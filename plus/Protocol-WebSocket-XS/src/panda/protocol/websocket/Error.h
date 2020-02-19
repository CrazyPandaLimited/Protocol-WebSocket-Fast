#pragma once

#include <system_error>
#include <panda/log.h>

namespace panda { namespace protocol { namespace websocket {

enum class errc {
    GARBAGE_AFTER_CONNECT = 1,

    VERSION_UPGRADE_REQUIRED,
    UNSUPPORTED_VERSION,
    RESPONSE_CODE_101,
    CONNECTION_MUSTBE_UPGRADE,
    UPGRADE_MUSTBE_WEBSOCKET,
    SEC_ACCEPT_MISSING,

    METHOD_MUSTBE_GET,
    HTTP_1_1_REQUIRED,
    BODY_PROHIBITED,

    INVALID_OPCODE,
    CONTROL_FRAGMENTED,
    CONTROL_PAYLOAD_TOO_BIG,
    NOT_MASKED,
    MAX_FRAME_SIZE,
    CLOSE_FRAME_INVALID_DATA,
    INITIAL_CONTINUE,
    FRAGMENT_NO_CONTINUE,

    MAX_MESSAGE_SIZE
};

class ProtocolErrorCategoty : public std::error_category
{
public:
    const char * name() const noexcept override {return "websocket::ProtocolError";}
    std::string message(int ev) const override;
};

extern const std::error_category& protocol_error_categoty;


inline std::error_code make_error_code(errc err) noexcept {
    return std::error_code(int(err), protocol_error_categoty);
}

extern log::Module pwslog;

}}}

namespace std {
template <> struct is_error_code_enum<panda::protocol::websocket::errc> : std::true_type {};
}
