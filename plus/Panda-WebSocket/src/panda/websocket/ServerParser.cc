#include <panda/websocket/ServerParser.h>
#include <exception>
#include <panda/websocket/ParserError.h>

namespace panda { namespace websocket {

ConnectRequestSP ServerParser::accept (string& buf) {
    if (_state[STATE_ACCEPT_PARSED]) throw ParserError("already parsed accept");

    if (!_connect_request) {
        _connect_request = new ConnectRequest();
        _connect_request->max_headers_size = max_handshake_size;
    }

    if (!_connect_request->parse(buf)) return NULL;

    _state.set(STATE_ACCEPT_PARSED);

    if (!_connect_request->error) {
        if (buf) _connect_request->error = "garbage found after http request";
        else _state.set(STATE_ACCEPTED);
    }

    return _connect_request;
}

string ServerParser::accept_error () {
    if (!_state[STATE_ACCEPT_PARSED]) throw ParserError("accept not parsed yet");
    if (established()) throw ParserError("already established");
    if (!_connect_request->error) throw ParserError("no errors found");

    HTTPResponse res;
    res.headers.emplace("Content-Type", "text/plain");

    if (!_connect_request->ws_version_supported()) {
        res.code    = 426;
        res.message = "Upgrade Required";
        res.body.push_back("426 Upgrade Required");

        string svers(50);
        for (int v : supported_ws_versions) {
            svers += string::from_number(v);
            svers += ", ";
        }
        if (svers) svers.length(svers.length()-2);
        res.headers.emplace("Sec-WebSocket-Version", svers);
    }
    else {
        res.code    = 400;
        res.message = "Bad Request";
        res.body.push_back("400 Bad Request\n");
        res.body.push_back(_connect_request->error);
    }

    return res.to_string();
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

    if (!res->body.size()) {
        res->body.push_back(string::from_number(res->code) + ' ' + res->message);
    }

    if (res->headers.find("Content-Type") == res->headers.end()) res->headers.emplace("Content-Type", "text/plain");

    return res->to_string();
}

string ServerParser::accept_response (ConnectResponse* res) {
    if (!accepted()) throw ParserError("client has not been accepted");
    if (established()) throw ParserError("already established");

    res->_ws_key = _connect_request->ws_key;
    if (!res->ws_protocol) res->ws_protocol = _connect_request->ws_protocol;
    if (!res->ws_extensions_set()) res->ws_extensions(_connect_request->ws_extensions());

    const auto& exts = res->ws_extensions();

    if (exts.size()) {
        // filter extensions
        res->ws_extensions(HTTPPacket::HeaderValues()); // for now no extensions supported
    }

    _state.set(STATE_ESTABLISHED);
    _connect_request = NULL;
    return res->to_string();
}

void ServerParser::reset () {
    _connect_request = NULL;
    Parser::reset();
}

}}
