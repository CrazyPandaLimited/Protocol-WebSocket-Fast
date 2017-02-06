#include <panda/websocket/ClientParser.h>
#include <exception>

namespace panda { namespace websocket {

string ClientParser::connect_request (ConnectRequestSP& req) {
    if (_connect_request) throw std::logic_error("ClientParser[connect_request] already requested connection");
    _connect_request = req;
    return req->to_string();
}

ConnectResponseSP ClientParser::connect (string& buf) {
    if (!_connect_request) throw std::logic_error("ClientParser[connect] has not requested connection");
    if (_established || (_connect_response && _connect_response->parsed())) throw std::logic_error("ClientParser[connect] already parsed response");

    if (!_connect_response) {
        _connect_response = new ConnectResponse();
        _connect_response->max_headers_size = max_handshake_size;
        _connect_response->_ws_key = _connect_request->ws_key;
    }

    if (!_connect_response->parse(buf)) return NULL;

    if (!_connect_response->error) {
        _buffer = buf;       // if something remains in buf, user can get it via get_frames() or get_messages() without param.
        _established = true;
    }

    return _connect_response;
}

void ClientParser::reset () {
    _connect_request = NULL;
    Parser::reset();
}

}}
