#include <panda/protocol/websocket/ServerParser.h>
#include <exception>
#include <panda/protocol/websocket/ParserError.h>
#include <panda/protocol/http/RequestParser.h>
#include <sstream>

namespace panda { namespace protocol { namespace websocket {

struct RequestFactory : http::RequestFactory {
    http::RequestSP create() const override {
        return make_iptr<ConnectRequest>();
    }
};

ServerParser::ServerParser()
    : Parser(true)
    , _connect_parser(new http::RequestParser(new RequestFactory))
{
    _connect_parser->max_body_size = http::RequestParser::BODY_PROHIBITED;
}

ConnectRequestSP ServerParser::accept (string& buf) {
    if (_state[STATE_ACCEPT_PARSED]) throw ParserError("already parsed accept");

//    if (!_connect_request) {
//        _connect_request = new ConnectRequest();
//
//    }

//    if (!_connect_request->parse(buf)) return NULL;
    http::RequestParser::Result res = _connect_parser->parse_first(buf);
    _connect_request = dynamic_pointer_cast<ConnectRequest>(res.request);
    if (!res.state) {
        std::cerr << "http err" << std::endl;
        _connect_request->error = res.state.error().what();
        return _connect_request;
    } else if (res.state != http::RequestParser::State::done) {
        return nullptr;
    }
    _connect_request->process_headers();
//    _connect_request->max_headers_size = _max_handshake_size; // TODO: add support of max headers size

    _state.set(STATE_ACCEPT_PARSED);

    if (!_connect_request->error) {
        if (res.position != buf.size()) {
            _connect_request->error = "garbage found after http request";
        } else {
            _state.set(STATE_ACCEPTED);
        }
    }

    return _connect_request;
}

string ServerParser::accept_error () {
    if (!_state[STATE_ACCEPT_PARSED]) throw ParserError("accept not parsed yet");
    if (established()) throw ParserError("already established");
    if (!_connect_request->error) throw ParserError("no errors found");

    HTTPResponseSP res = new HTTPResponse();
    res->headers.add_field("Content-Type", "text/plain");

    if (!_connect_request->ws_version_supported()) {
        res->code    = 426;
        res->message = "Upgrade Required";
        res->body->parts.push_back("426 Upgrade Required");

        string svers(50);
        for (int v : supported_ws_versions) {
            svers += string::from_number(v);
            svers += ", ";
        }
        if (svers) svers.length(svers.length()-2);
        res->headers.add_field("Sec-WebSocket-Version", svers);
    }
    else {
        res->code    = 400;
        res->message = "Bad Request";
        res->body->parts.push_back("400 Bad Request\n");
        res->body->parts.push_back(_connect_request->error);
    }
    res->headers.set_field("Content-Length", panda::to_string(res->body->content_length()));

    std::ostringstream ss;
    ss << *res;
    ss.flush();
    return string(ss.str().data());
}

string ServerParser::accept_error (HTTPResponse* res) {
    if (!_state[STATE_ACCEPT_PARSED]) throw ParserError("accept not parsed yet");
    if (established()) throw ParserError("already established");
    if (_connect_request->error) return accept_error();

    if (!res->code) {
        res->code = 400;
        res->message = "Bad Request";
    }
    else if (!res->message) res->message = "Unknown";

    if (res->body->empty()) {
        res->body->parts.push_back(string::from_number(res->code) + ' ' + res->message);
    }

    if (!res->headers.has_field("Content-Type")) res->headers.add_field("Content-Type", "text/plain");
    if (!res->headers.has_field("Content-Length")) res->headers.add_field("Content-Length", panda::to_string(res->body->content_length()));

    std::ostringstream ss;
    ss << *res;
    ss.flush();
    return string(ss.str().data());
}

string ServerParser::accept_response (ConnectResponse* res) {
    if (!accepted()) throw ParserError("client has not been accepted");
    if (established()) throw ParserError("already established");

    res->_ws_key = _connect_request->ws_key;
    if (!res->ws_protocol) res->ws_protocol = _connect_request->ws_protocol;
    if (!res->ws_extensions_set()) res->ws_extensions(_connect_request->ws_extensions());

    const auto& exts = res->ws_extensions();
    HeaderValues used_extensions;
    if (_deflate_cfg && exts.size()) {
        // filter extensions
        auto role = DeflateExt::Role::SERVER;
        auto deflate_matches = DeflateExt::select(exts, *_deflate_cfg, role);
        if (deflate_matches) {
            _deflate_ext.reset(DeflateExt::uplift(deflate_matches, used_extensions, role));
        }
    }
    res->ws_extensions(std::move(used_extensions));

    _state.set(STATE_ESTABLISHED);
    _connect_request = NULL;
    return res->to_string();
}

void ServerParser::reset () {
    _connect_request = NULL;
    Parser::reset();
}

ServerParser::~ServerParser() {}

}}}
