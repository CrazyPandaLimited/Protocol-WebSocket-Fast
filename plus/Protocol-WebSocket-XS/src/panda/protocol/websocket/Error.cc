#include "Error.h"

#include <map>

#include <panda/log.h>

namespace panda { namespace protocol { namespace websocket {

log::Module pwslog("Protocol""WebSocket", log::Warning);

const std::error_category& protocol_error_categoty = ProtocolErrorCategoty();

static const std::map<errc, std::string> error_map = {
    {errc::GARBAGE_AFTER_CONNECT,      "garbage found after http request"},

    {errc::RESPONSE_CODE_101,          "handshake response code must be 101"},
    {errc::VERSION_UPGRADE_REQUIRED,   "version upgrade required"},
    {errc::UNSUPPORTED_VERSION,        "client's Sec-WebSocket-Version is not supported"},
    {errc::CONNECTION_MUSTBE_UPGRADE,   "Connection must be 'Upgrade'"},
    {errc::UPGRADE_MUSTBE_WEBSOCKET,    "Upgrade must be 'websocket'"},
    {errc::SEC_ACCEPT_MISSING,         "Sec-WebSocket-Accept missing or invalid"},

    {errc::METHOD_MUSTBE_GET,  "method must be GET"},
    {errc::HTTP_1_1_REQUIRED, "HTTP/1.1 or higher required"},
    {errc::BODY_PROHIBITED,   "body must not present"},

    {errc::INVALID_OPCODE,          "invalid opcode received"},
    {errc::CONTROL_FRAGMENTED,      "control frame can't be fragmented"},
    {errc::CONTROL_PAYLOAD_TOO_BIG, "control frame payload is too big"},
    {errc::NOT_MASKED,              "frame is not masked"},
    {errc::MAX_FRAME_SIZE,          "max frame size exceeded"},
    {errc::CLOSE_FRAME_INVALID_DATA,"control frame CLOSE contains invalid data"},
    {errc::INITIAL_CONTINUE,        "initial frame can't have opcode CONTINUE"},
    {errc::FRAGMENT_NO_CONTINUE,    "fragment frame must have opcode CONTINUE"},

    {errc::MAX_MESSAGE_SIZE,        "max message size exceeded"},
};

std::string ProtocolErrorCategoty::message(int ev) const {
    panda_debug_v(ev);
    auto iter = error_map.find(errc(ev));
    if (iter != error_map.end()) {
        return iter->second;
    } else if (ev){
        return "Unknown websocket error";
    } else {
        return "";
    }
}

}}}

