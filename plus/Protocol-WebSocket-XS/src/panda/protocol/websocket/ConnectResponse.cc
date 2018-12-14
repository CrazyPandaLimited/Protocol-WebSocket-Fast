#include <panda/protocol/websocket/ConnectResponse.h>
#include <openssl/sha.h>
#include <panda/encode/base64.h>

namespace panda { namespace protocol { namespace websocket {

void ConnectResponse::_parse_header (StringRange range) {
    HTTPResponse::_parse_header(range);
    if (error) return;

    if (code == 426) {
        auto it = headers.find("Sec-WebSocket-Version");
        if (it != headers.end()) _ws_versions = it->second;
        error = "websocket version upgrade required";
        return;
    }

    if (code != 101) {
        error = "websocket handshake response code must be 101";
        return;
    }

    auto it = headers.find("Connection");
    if (it == headers.end() || it->second.find("Upgrade") == string::npos) {
        error = "Connection must be 'Upgrade'";
        return;
    }

    it = headers.find("Upgrade");
    if (it == headers.end() || !string_contains_ci(it->second, "websocket")) {
        error = "Upgrade must be 'websocket'";
        return;
    }

    it = headers.find("Sec-WebSocket-Accept");
    if (it == headers.end() || it->second != _calc_accept_key(_ws_key)) {
        error = "Sec-WebSocket-Accept missing or invalid";
        return;
    }
    else _ws_accept_key = it->second;

    auto ext_range = headers.equal_range("Sec-WebSocket-Extensions");
    for (auto it = ext_range.first; it != ext_range.second; ++it) {
        parse_header_value(it->second, _ws_extensions);
    }

    it = headers.find("Sec-WebSocket-Protocol");
    if (it != headers.end()) ws_protocol = it->second;
}

void ConnectResponse::_to_string (string& str) {
    code    = 101;
    message = "Switching Protocols";
    headers.emplace("Upgrade", "websocket");
    headers.emplace("Connection", "Upgrade");

    if (ws_protocol) headers.emplace("Sec-WebSocket-Protocol", ws_protocol);

    headers.emplace("Sec-WebSocket-Accept", _calc_accept_key(_ws_key));

    if (_ws_extensions.size()) headers.emplace("Sec-WebSocket-Extensions", compile_header_value(_ws_extensions));

    body.clear(); // body not supported in WS responses

    HTTPResponse::_to_string(str);
}


string ConnectResponse::_calc_accept_key (string ws_key) {
    auto key_base = ws_key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    unsigned char sha1bin[21];
    SHA1((const unsigned char*)key_base.data(), key_base.length(), sha1bin);
    return panda::encode::encode_base64(std::string_view((const char*)sha1bin, 20), false, true);
}

}}}
