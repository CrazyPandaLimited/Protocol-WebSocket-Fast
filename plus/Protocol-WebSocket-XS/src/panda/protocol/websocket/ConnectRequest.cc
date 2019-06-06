#include <panda/protocol/websocket/ConnectRequest.h>
#include <panda/encode/base64.h>

namespace panda { namespace protocol { namespace websocket {

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
    if (it == headers.fields.rend() || !string_contains_ci(it->value, "upgrade")) {
        error = "Connection must be 'Upgrade'";
        return;
    }

    it = headers.find("Upgrade");
    if (it == headers.fields.rend() || !string_contains_ci(it->value, "websocket")) {
        error = "Upgrade must be 'websocket'";
        return;
    }

    ok = false;
    it = headers.find("Sec-WebSocket-Key");
    if (it != headers.fields.rend()) {
        ws_key = it->value;
        auto decoded = panda::encode::decode_base64(ws_key);
        if (decoded.length() == 16) ok = true;
    }
    if (!ok) {error = "Sec-WebSocket-Key missing or invalid"; return; }

    _ws_version_supported = false;
    it = headers.find("Sec-WebSocket-Version");
    if (it != headers.fields.rend()) {
        it->value.to_number(ws_version);
        for (int v : supported_ws_versions) {
            if (ws_version != v) continue;
            _ws_version_supported = true;
            break;
        }
    }
    if (!_ws_version_supported) { error = "client's Sec-WebSocket-Version is not supported"; return; }

    auto ext_range = headers.equal_range("Sec-WebSocket-Extensions");
    for (auto& hv : ext_range) {
        parse_header_value(hv.value, _ws_extensions);
    }

    ws_protocol = headers.get_field("Sec-WebSocket-Protocol");
}

void ConnectRequest::_to_string (string& str) {
    if (uri && uri->scheme() && uri->scheme() != "ws" && uri->scheme() != "wss")
        throw std::logic_error("ConnectRequest[to_string] uri scheme must be 'ws' or 'wss'");
    if (body()->length()) throw std::logic_error("ConnectRequest[to_string] http body is not allowed for websocket handshake request");

    method = "GET";

    if (!ws_key) {
        int32_t keybuf[] = {std::rand(), std::rand(), std::rand(), std::rand()};
        ws_key = panda::encode::encode_base64(std::string_view((const char*)keybuf, sizeof(keybuf)), false, true);
    }
    replace_header("Sec-WebSocket-Key", ws_key);

    if (ws_protocol) replace_header("Sec-WebSocket-Protocol", ws_protocol);

    if (!ws_version) ws_version = 13;
    replace_header("Sec-WebSocket-Version", string::from_number(ws_version));

    if (_ws_extensions.size()) replace_header("Sec-WebSocket-Extensions", compile_header_value(_ws_extensions));

    replace_header("Connection", "Upgrade");
    replace_header("Upgrade", "websocket");

    HTTPRequest::_to_string(str);
}

void ConnectRequest::add_deflate(const DeflateExt::Config& cfg) {
    DeflateExt::request(_ws_extensions, cfg);
}


}}}
