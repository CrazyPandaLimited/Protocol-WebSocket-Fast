#include <panda/protocol/websocket/ConnectRequest.h>
#include <panda/encode/base64.h>
#include <panda/log.h>
#include "ConnectResponse.h"

namespace panda { namespace protocol { namespace websocket {

ConnectRequest::~ConnectRequest() {
}

void ConnectRequest::process_headers () {
    bool ok;

    if (method != Method::GET) {
        error = "method must be GET";
        return;
    }

    if (http_version != http::HttpVersion::v1_1) {
        error = "HTTP/1.1 or higher required";
        return;
    }

    if (!body.empty()) {
        error = "body must not present";
        return;
    }

    auto it = headers.find("Connection");
    if (it == headers.end() || !string_contains_ci(it->value, "upgrade")) {
        error = "Connection must be 'Upgrade'";
        return;
    }

    it = headers.find("Upgrade");
    if (it == headers.end() || !string_contains_ci(it->value, "websocket")) {
        error = "Upgrade must be 'websocket'";
        return;
    }

    ok = false;
    it = headers.find("Sec-WebSocket-Key");
    if (it != headers.end()) {
        ws_key = it->value;
        auto decoded = panda::encode::decode_base64(ws_key);
        if (decoded.length() == 16) ok = true;
    }
    if (!ok) {error = "Sec-WebSocket-Key missing or invalid"; return; }

    _ws_version_supported = false;
    it = headers.find("Sec-WebSocket-Version");
    if (it != headers.end()) {
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

//void ConnectRequest::_to_string (string& str) {
//    if (uri && uri->scheme() && uri->scheme() != "ws" && uri->scheme() != "wss")
//        throw std::logic_error("ConnectRequest[to_string] uri scheme must be 'ws' or 'wss'");
//    if (body.length()) throw std::logic_error("ConnectRequest[to_string] http body is not allowed for websocket handshake request");

//    method = "GET";

//    if (!ws_key) {
//        int32_t keybuf[] = {std::rand(), std::rand(), std::rand(), std::rand()};
//        ws_key = panda::encode::encode_base64(std::string_view((const char*)keybuf, sizeof(keybuf)), false, true);
//    }
//    replace_header("Sec-WebSocket-Key", ws_key);

//    if (ws_protocol) replace_header("Sec-WebSocket-Protocol", ws_protocol);

//    if (!ws_version) ws_version = 13;
//    replace_header("Sec-WebSocket-Version", string::from_number(ws_version));

//    if (_ws_extensions.size()) replace_header("Sec-WebSocket-Extensions", compile_header_value(_ws_extensions));

//    replace_header("Connection", "Upgrade");
//    replace_header("Upgrade", "websocket");

//    HTTPRequest::_to_string(str);
//}

void ConnectRequest::add_deflate(const DeflateExt::Config& cfg) {
    DeflateExt::request(_ws_extensions, cfg);
}

string ConnectRequest::to_string() {
    if (!uri || !uri->host()) {
        throw std::logic_error("HTTPRequest[to_string] uri with net location must be defined");
    }
    if (uri && uri->scheme() && uri->scheme() != "ws" && uri->scheme() != "wss") {
        throw std::logic_error("ConnectRequest[to_string] uri scheme must be 'ws' or 'wss'");
    }
    if (body.length()) {
        throw std::logic_error("ConnectRequest[to_string] http body is not allowed for websocket handshake request");
    }

    method = Request::Method::GET;

    if (!ws_key) {
        int32_t keybuf[] = {std::rand(), std::rand(), std::rand(), std::rand()};
        ws_key = panda::encode::encode_base64(string_view((const char*)keybuf, sizeof(keybuf)), false, true);
    }
    headers.set_field("Sec-WebSocket-Key", ws_key);

    if (ws_protocol) headers.set_field("Sec-WebSocket-Protocol", ws_protocol);

    if (!ws_version) ws_version = 13;
    headers.set_field("Sec-WebSocket-Version", string::from_number(ws_version));

    if (_ws_extensions.size()) headers.set_field("Sec-WebSocket-Extensions", compile_header_value(_ws_extensions));

    headers.set_field("Connection", "Upgrade");
    headers.set_field("Upgrade", "websocket");

    if (!headers.has_field("User-Agent")) headers.add_field("User-Agent", "Panda-WebSocket");
    if (!headers.has_field("Host"))       headers.add_field("Host", uri->host());

    return http::Request::to_string();
}

http::ResponseSP ConnectRequest::create_response() const{
    return new ConnectResponse();
}


}}}
