#include "Error.h"

#include <map>

#include <panda/log.h>

namespace panda { namespace protocol { namespace websocket {

const std::error_category& protocol_error_categoty = ProtocolErrorCategoty();

static const std::map<ProtocolError, std::string> error_map = {
    {ProtocolError::GARBAGE_AFTER_CONNECT,      "Websocket: garbage found after http request"},
    {ProtocolError::DEFLATE_NEGOTIATION_FAILED, "Websocket: deflate paramenters negotiation error"},

    {ProtocolError::RESPONSE_CODE_101,          "Websocket: handshake response code must be 101"},
    {ProtocolError::VERSION_UPGRADE_REQUIRED,   "Websocket: version upgrade required"},
    {ProtocolError::UNSUPPORTED_VERSION,        "Websocket: client's Sec-WebSocket-Version is not supported"},
    {ProtocolError::CONNECTION_ISNOT_UPGRADE,   "Websocket: Connection must be 'Upgrade'"},
    {ProtocolError::UPGRADE_ISNOT_WEBSOCKET,    "Websocket: Upgrade must be 'websocket'"},
    {ProtocolError::SEC_ACCEPT_MISSING,         "Websocket: Sec-WebSocket-Accept missing or invalid"},

    {ProtocolError::METHOD_ISNOT_GET,  "Websocket: method must be GET"},
    {ProtocolError::HTTP_1_1_REQUIRED, "Websocket: HTTP/1.1 or higher required"},
    {ProtocolError::BODY_PROHIBITED,   "Websocket: body must not present"},
};

std::string ProtocolErrorCategoty::message(int ev) const {
    panda_debug_v(ev);
    auto iter = error_map.find(ProtocolError(ev));
    if (iter != error_map.end()) {
        return iter->second;
    } else if (ev){
        return "Unknown websocket error";
    } else {
        return "";
    }
}

}}}

