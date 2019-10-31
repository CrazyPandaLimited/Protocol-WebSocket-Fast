#pragma once

#include <system_error>

namespace panda { namespace protocol { namespace websocket {

enum class ProtocolError {
    GARBAGE_AFTER_CONNECT = 1,
    DEFLATE_NEGOTIATION_FAILED,

    VERSION_UPGRADE_REQUIRED,
    UNSUPPORTED_VERSION,
    RESPONSE_CODE_101,
    CONNECTION_ISNOT_UPGRADE,
    UPGRADE_ISNOT_WEBSOCKET,
    SEC_ACCEPT_MISSING,

    METHOD_ISNOT_GET,
    HTTP_1_1_REQUIRED,
    BODY_PROHIBITED,


};

class ProtocolErrorCategoty : public std::error_category
{
public:
    const char * name() const noexcept override {return "websocket::ProtocolError";}
    std::string message(int ev) const override;
};

extern const std::error_category& protocol_error_categoty;


inline std::error_code make_error_code(ProtocolError err) noexcept {
    return std::error_code(int(err), protocol_error_categoty);
}

}}}

namespace std {
template <> struct is_error_code_enum<panda::protocol::websocket::ProtocolError> : std::true_type {};
}
