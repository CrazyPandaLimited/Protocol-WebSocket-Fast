#include <panda/websocket/ClientParser.h>
#include <ctime>
#include <cstdlib>
#include <exception>

namespace panda { namespace websocket {

static bool _init () {
    std::srand(std::time(NULL));
    return true;
}

static const bool _inited = _init();

string ClientParser::connect_request (ConnectRequestSP& req) {
    if (_state[STATE_CONNECTION_REQUESTED]) throw std::logic_error("already requested connection");
    _state.set(STATE_CONNECTION_REQUESTED);
    _connect_request = req;
    return req->to_string();
}

ConnectResponseSP ClientParser::connect (string& buf) {
    if (!_state[STATE_CONNECTION_REQUESTED]) throw std::logic_error("has not requested connection");
    if (_state[STATE_CONNECTION_RESPONSE_PARSED]) throw std::logic_error("already parsed connect response");

    if (!_connect_response) {
        _connect_response = new ConnectResponse();
        _connect_response->max_headers_size = max_handshake_size;
        _connect_response->_ws_key = _connect_request->ws_key;
    }

    if (!_connect_response->parse(buf)) return NULL;

    _state.set(STATE_CONNECTION_RESPONSE_PARSED);

    if (!_connect_response->error) {
        _buffer = buf;       // if something remains in buf, user can get it via get_frames() or get_messages() without buf param.
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

}}
