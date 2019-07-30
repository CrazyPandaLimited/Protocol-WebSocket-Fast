#include <panda/protocol/websocket/ConnectResponse.h>
#include <openssl/sha.h>
#include <panda/encode/base64.h>

namespace panda { namespace protocol { namespace websocket {

//void ConnectResponse::_parse_header (StringRange range) {
//    HTTPResponse::_parse_header(range);
//    if (error) return;

//    if (code == 426) {
//        _ws_versions = headers.get_field("Sec-WebSocket-Version");
//        error = "websocket version upgrade required";
//        return;
//    }

//    if (code != 101) {
//        error = "websocket handshake response code must be 101";
//        return;
//    }

//    auto it = headers.find("Connection");
//    if (it == headers.fields.rend() || !string_contains_ci(it->value, "upgrade")) {
//        error = "Connection must be 'Upgrade'";
//        return;
//    }

//    it = headers.find("Upgrade");
//    if (it == headers.end() || !string_contains_ci(it->value, "websocket")) {
//        error = "Upgrade must be 'websocket'";
//        return;
//    }

//    it = headers.find("Sec-WebSocket-Accept");
//    if (it == headers.end() || it->value != _calc_accept_key(_ws_key)) {
//        error = "Sec-WebSocket-Accept missing or invalid";
//        return;
//    }
//    else _ws_accept_key = it->value;

//    auto ext_range = headers.equal_range("Sec-WebSocket-Extensions");
//    for (auto& hv : ext_range) {
//        http::parse_header_value(hv.value, _ws_extensions);
//    }

//    ws_protocol = headers.get_field("Sec-WebSocket-Protocol");
//}

//void ConnectResponse::_to_string (string& str) {
//    code    = 101;
//    message = "Switching Protocols";
//    headers.add_field("Upgrade", "websocket");
//    headers.add_field("Connection", "Upgrade");

//    if (ws_protocol) headers.add_field("Sec-WebSocket-Protocol", ws_protocol);

//    headers.add_field("Sec-WebSocket-Accept", _calc_accept_key(_ws_key));

//    if (_ws_extensions.size()) headers.add_field("Sec-WebSocket-Extensions", compile_header_value(_ws_extensions));

//    body->parts.clear(); // body not supported in WS responses

//    HTTPResponse::_to_string(str);
//}


string ConnectResponse::_calc_accept_key (string ws_key) {
    auto key_base = ws_key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    unsigned char sha1bin[21];
    SHA1((const unsigned char*)key_base.data(), key_base.length(), sha1bin);
    return panda::encode::encode_base64(string_view((const char*)sha1bin, 20), false, true);
}

string ConnectResponse::to_string() {
    code    = 101;
    message = "Switching Protocols";
    headers.add_field("Upgrade", "websocket");
    headers.add_field("Connection", "Upgrade");

    if (ws_protocol) headers.add_field("Sec-WebSocket-Protocol", ws_protocol);

    headers.add_field("Sec-WebSocket-Accept", _calc_accept_key(_ws_key));

    if (_ws_extensions.size()) headers.add_field("Sec-WebSocket-Extensions", compile_header_value(_ws_extensions));

    body->parts.clear(); // body not supported in WS responses

    string res;
    for (const auto& s : to_vector(this)) {
        res += s;
    }
    return res;
}

}}}
