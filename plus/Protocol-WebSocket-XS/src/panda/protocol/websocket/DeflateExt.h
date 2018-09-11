#pragma once

#include <panda/refcnt.h>
#include <panda/protocol/websocket/HTTPRequest.h>

namespace panda { namespace protocol { namespace websocket {

class DeflateExt;

class DeflateExt : public panda::Refcnt {
public:

    struct config_t {
        bool client_no_context_takeover = false;
        bool server_no_context_takeover = false;
        std::uint8_t server_max_window_bits = 15;
        std::uint8_t client_max_window_bits = 15;
    };

    static const char* extension_name;

    static const HTTPPacket::HeaderValue* select(const HTTPPacket::HeaderValues& values);
    static void request(HTTPPacket::HeaderValues& ws_extensions, const config_t& cfg);
    static DeflateExt* uplift(const HTTPPacket::HeaderValue& deflate_extension, HTTPPacket::HeaderValues& extensions);

private:
    DeflateExt(const config_t& cfg):_cfg{cfg}{}
    config_t _cfg;
};

using DeflateExtSP = panda::iptr<DeflateExt>;

}}}
