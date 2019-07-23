#include <xs/protocol/websocket.h>
#include <xs/uri.h>
#include <xs/export.h>

#include <iostream>

using namespace xs;
using xs::exp::autoexport;
using xs::exp::create_constant;
using xs::exp::create_constants;

using panda::string;
using panda::string_view;
using xs::my_perl;

using namespace xs::protocol::websocket;
using namespace panda::protocol::websocket;

MODULE = Protocol::WebSocket::XS                PACKAGE = Protocol::WebSocket::XS
PROTOTYPES: DISABLE

BOOT {
    Stash me(__PACKAGE__);
    create_constants(me, {
        {"OPCODE_CONTINUE", (int)Opcode::CONTINUE},
        {"OPCODE_TEXT",     (int)Opcode::TEXT    },
        {"OPCODE_BINARY",   (int)Opcode::BINARY  },
        {"OPCODE_CLOSE",    (int)Opcode::CLOSE   },
        {"OPCODE_PING",     (int)Opcode::PING    },
        {"OPCODE_PONG",     (int)Opcode::PONG    },

        {"CLOSE_DONE",             (int)CloseCode::DONE            },
        {"CLOSE_AWAY",             (int)CloseCode::AWAY            },
        {"CLOSE_PROTOCOL_ERROR",   (int)CloseCode::PROTOCOL_ERROR  },
        {"CLOSE_INVALID_DATA",     (int)CloseCode::INVALID_DATA    },
        {"CLOSE_UNKNOWN",          (int)CloseCode::UNKNOWN         },
        {"CLOSE_ABNORMALLY",       (int)CloseCode::ABNORMALLY      },
        {"CLOSE_INVALID_TEXT",     (int)CloseCode::INVALID_TEXT    },
        {"CLOSE_BAD_REQUEST",      (int)CloseCode::BAD_REQUEST     },
        {"CLOSE_MAX_SIZE",         (int)CloseCode::MAX_SIZE        },
        {"CLOSE_EXTENSION_NEEDED", (int)CloseCode::EXTENSION_NEEDED},
        {"CLOSE_INTERNAL_ERROR",   (int)CloseCode::INTERNAL_ERROR  },
        {"CLOSE_TLS",              (int)CloseCode::TLS             }
    });
    autoexport(me);
    
    auto err = panda::protocol::websocket::DeflateExt::bootstrap();
    if (err) throw *err;
}

INCLUDE: Parser.xsi

INCLUDE: ServerParser.xsi

INCLUDE: ClientParser.xsi

INCLUDE: HTTPPacket.xsi

INCLUDE: HTTPRequest.xsi

INCLUDE: HTTPResponse.xsi

INCLUDE: ConnectRequest.xsi

INCLUDE: ConnectResponse.xsi

INCLUDE: Frame.xsi

INCLUDE: FrameBuilder.xsi

INCLUDE: FrameIterator.xsi

INCLUDE: Message.xsi

INCLUDE: MessageIterator.xsi
