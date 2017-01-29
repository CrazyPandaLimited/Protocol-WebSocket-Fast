#include <panda/websocket/HTTPResponse.h>
#include <iostream>
#include <panda/lib/lib.h>

namespace panda { namespace websocket {

void HTTPResponse::_to_string (string& str) {
    size_t stlen = 30 + message.length();
    str.reserve(stlen);

    str += "HTTP/1.1 ";
    str += panda::lib::itoa(code);
    str += ' ';
    str += message;
    str += "\r\n";

    if (headers.find("Server") == headers.end()) headers.emplace("Server", "Panda-WebSocket");

    HTTPPacket::_to_string(str);
}

}}
