#include <panda/protocol/websocket/ClientParser.h>
#include <ctime>
#include <cstdlib>
#include <exception>
#include <panda/protocol/websocket/ParserError.h>

namespace panda { namespace protocol { namespace websocket {

static bool _init () {
    std::srand(std::time(NULL));
    return true;
}

static const bool _inited = _init();

string ClientParser::connect_request (const ConnectRequestSP& req) {
    if (_state[STATE_CONNECTION_REQUESTED]) {
        throw ParserError("already requested connection");
    }
    _state.set(STATE_CONNECTION_REQUESTED);
    _connect_request = req;
    _connect_response_parser->append_request(req);
    if (_deflate_cfg) req->add_deflate(*_deflate_cfg);
    return req->to_string();
}

ConnectResponseSP ClientParser::connect (string& buf) {
    if (!_state[STATE_CONNECTION_REQUESTED]) throw ParserError("has not requested connection");
    if (_state[STATE_CONNECTION_RESPONSE_PARSED]) throw ParserError("already parsed connect response");

//    if (!_connect_response) {
//        _connect_response = new ConnectResponse();
        //_connect_response->max_headers_size = _max_handshake_size; // TODO: add support of max
//    }

    http::ResponseParser::Result res = _connect_response_parser->parse_first(buf);
    _connect_response = dynamic_pointer_cast<ConnectResponse>(res.response);

    if (!res.state) {
        _connect_response->error = res.state.error().what();
        ConnectResponseSP ret(_connect_response);
        _connect_request = NULL;
        _connect_response = NULL;
        return ret;
    }
    else if (res.state != http::ResponseParser::State::done) {
        return nullptr;
    }
    _connect_response->_ws_key = _connect_request->ws_key;
    _connect_response->process_headers();

    _state.set(STATE_CONNECTION_RESPONSE_PARSED);

    if (!_connect_response->error && _deflate_cfg) {
        using result_t = DeflateExt::EffectiveConfig::NegotiationsResult;
        auto& exts = _connect_response->ws_extensions();
        HeaderValues used_extensions;
        auto role = DeflateExt::Role::CLIENT;
        auto deflate_matches = DeflateExt::select(exts, *_deflate_cfg, role);
        switch (deflate_matches.result) {
        case result_t::SUCCESS:
            _deflate_ext.reset(DeflateExt::uplift(deflate_matches, used_extensions, role));
            _connect_response->ws_extensions(used_extensions);
            break;
        case result_t::NOT_FOUND:
            /* NOOP */
            break;
        case result_t::ERROR:
            _connect_response->error = "deflate paramenters negotiation error";
        }
    }

    if (!_connect_response->error) {
        _buffer = buf.substr(res.position);// if something remains in buf, user can get it via get_frames() or get_messages() without buf param.
        _state.set(STATE_ESTABLISHED);
    }

    ConnectResponseSP ret(_connect_response);
    _connect_request = NULL;
    _connect_response = NULL;
    return ret;
}

void ClientParser::reset () {
    _connect_request = NULL;
    Parser::reset();
}

}}}
