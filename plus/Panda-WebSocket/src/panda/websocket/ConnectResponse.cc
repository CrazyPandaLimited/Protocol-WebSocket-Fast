#include <panda/websocket/ConnectResponse.h>
#include <openssl/sha.h>
#include <panda/encode/base64.h>
#include <iostream>

namespace panda { namespace websocket {

void ConnectResponse::_to_string (string& str) {

    code    = 101;
    message = "Switching Protocols";
    headers.emplace("Upgrade", "websocket");
    headers.emplace("Connection", "Upgrade");

    if (ws_protocol) headers.emplace("Sec-WebSocket-Protocol", ws_protocol);

    auto key_base = _ws_key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    unsigned char sha1bin[21];
    SHA1((const unsigned char*)key_base.data(), key_base.length(), sha1bin);
    string accept_key = panda::encode::encode_base64((const char*)sha1bin, 20, false, true);
    headers.emplace("Sec-WebSocket-Accept", accept_key);

    if (_ws_extensions.size()) headers.emplace("Sec-WebSocket-Extensions", compile_header_value(_ws_extensions));

    body.clear(); // body not supported in WS responses

    HTTPResponse::_to_string(str);
}

}}
