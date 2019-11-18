#include <panda/protocol/websocket/ConnectRequest.h>
#include <panda/encode/base64.h>
#include <panda/log.h>
#include "ConnectResponse.h"
#include "Error.h"

namespace panda { namespace protocol { namespace websocket {

ConnectRequest::~ConnectRequest() {
}

void ConnectRequest::process_headers () {
    bool ok;

    if (method != Method::GET) {
        error = errc::METHOD_MUSTBE_GET;
        return;
    }

    if (http_version != 11) {
        error = errc::HTTP_1_1_REQUIRED;
        return;
    }

    if (!body.empty()) {
        error = errc::BODY_PROHIBITED;
        return;
    }

    auto it = headers.find("Connection");
    if (it == headers.end() || !string_contains_ci(it->value, "upgrade")) {
        error = errc::CONNECTION_MUSTBE_UPGRADE;
        return;
    }

    it = headers.find("Upgrade");
    if (it == headers.end() || !string_contains_ci(it->value, "websocket")) {
        error = errc::UPGRADE_MUSTBE_WEBSOCKET;
        return;
    }

    ok = false;
    it = headers.find("Sec-WebSocket-Key");
    if (it != headers.end()) {
        ws_key = it->value;
        auto decoded = panda::encode::decode_base64(ws_key);
        if (decoded.length() == 16) ok = true;
    }
    if (!ok) {error = errc::SEC_ACCEPT_MISSING; return; }

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
    if (!_ws_version_supported) { error = errc::UNSUPPORTED_VERSION; return; }

    auto ext_range = headers.get_multi("Sec-WebSocket-Extensions");
    for (auto& val : ext_range) {
        parse_header_value(val, _ws_extensions);
    }

    ws_protocol = headers.get("Sec-WebSocket-Protocol");
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
    headers.set("Sec-WebSocket-Key", ws_key);

    if (ws_protocol) headers.set("Sec-WebSocket-Protocol", ws_protocol);

    if (!ws_version) ws_version = 13;
    headers.set("Sec-WebSocket-Version", string::from_number(ws_version));

    if (_ws_extensions.size()) headers.set("Sec-WebSocket-Extensions", compile_header_value(_ws_extensions));

    headers.set("Connection", "Upgrade");
    headers.set("Upgrade", "websocket");

    if (!headers.has("User-Agent")) headers.add("User-Agent", "Panda-WebSocket");
    if (!headers.has("Host"))       headers.add("Host", uri->host());

    return http::Request::to_string();
}

http::ResponseSP ConnectRequest::new_response() const{
    return new ConnectResponse();
}


}}}
