#include <panda/websocket/ConnectRequest.h>
#include <panda/encode/base64.h>
#include <cstdlib>
#include <iostream>

namespace panda { namespace websocket {

void ConnectRequest::_parse_header (StringRange range) {
    HTTPRequest::_parse_header(range);
    if (error) return;
    bool ok;

    if (method != "GET") {
        error = "method must be GET";
        return;
    }

    if (content_length()) {
        error = "body must not present";
        return;
    }

    auto it = headers.find("Connection");
    if (it == headers.end() || it->second.find("Upgrade", 0, 7) == string::npos) {
        error = "Connection must be 'Upgrade'";
        return;
    }

    it = headers.find("Upgrade");
    if (it == headers.end() || it->second.find("websocket", 0, 9) == string::npos) {
        error = "Upgrade must be 'websocket'";
        return;
    }

    ok = false;
    it = headers.find("Sec-WebSocket-Key");
    if (it != headers.end()) {
        const string& b64key = it->second;
        ws_key = it->second;
        auto decoded = panda::encode::decode_base64(ws_key);
        if (decoded.length() == 16) ok = true;
    }
    if (!ok) {error = "Sec-WebSocket-Key missing or invalid"; return; }

    _ws_version_supported = false;
    it = headers.find("Sec-WebSocket-Version");
    if (it != headers.end()) {
        ws_version = atoi(it->second.data());
        for (int v : supported_ws_versions) {
            if (ws_version != v) continue;
            _ws_version_supported = true;
            break;
        }
    }
    if (!_ws_version_supported) { error = "client's Sec-WebSocket-Version is not supported"; return; }

    auto ext_range = headers.equal_range("Sec-WebSocket-Extensions");
    for (auto it = ext_range.first; it != ext_range.second; ++it) {
        parse_header_value(it->second, _ws_extensions);
    }

    it = headers.find("Sec-WebSocket-Protocol");
    if (it != headers.end()) ws_protocol = it->second;
}

void ConnectRequest::_to_string (string& str) {
    if (uri && uri->scheme() != "ws" && uri->scheme() != "wss") throw std::logic_error("ConnectRequest[to_string] uri scheme must be 'ws' or 'wss'");
    if (body.size()) throw std::logic_error("ConnectRequest[to_string] http body is not allowed for websocket handshake request");

    method = "GET";

    if (!ws_key) {
        decltype(std::rand()) keybuf[] = {std::rand(), std::rand(), std::rand(), std::rand()};
        ws_key = panda::encode::encode_base64((const char*)keybuf, sizeof(keybuf), false, true);
    }
    replace_header("Sec-WebSocket-Key", ws_key);

    if (ws_protocol) replace_header("Sec-WebSocket-Protocol", ws_protocol);

    if (!ws_version) ws_version = 13;
    replace_header("Sec-WebSocket-Version", panda::lib::itoa(ws_version));

    if (_ws_extensions.size()) replace_header("Sec-WebSocket-Extensions", compile_header_value(_ws_extensions));

    replace_header("Connection", "Upgrade");
    replace_header("Upgrade", "websocket");

    HTTPRequest::_to_string(str);
}

}}
