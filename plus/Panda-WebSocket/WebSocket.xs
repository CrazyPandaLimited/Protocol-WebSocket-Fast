#include <xs/websocket.h>
#include <xs/lib.h>
#include <xs/uri.h>
#include <xs/export.h>

#include <iostream>

using std::cout;
using std::endl;

using xs::exp::constant_t;
using xs::exp::create_constant;
using xs::exp::create_constants;

using panda::string;
using std::string_view;
using xs::lib::sv2string;
using xs::my_perl;

using namespace xs::websocket;
using namespace panda::websocket;

static inline SV* new_sv_shared (const char* str) { return newSVpvn_share(str, strlen(str), 0); }

static SV*const http_response_class    = new_sv_shared("Panda::WebSocket::HTTPResponse");
static SV*const connect_response_class = new_sv_shared("Panda::WebSocket::ConnectResponse");
static SV*const connect_request_class  = new_sv_shared("Panda::WebSocket::ConnectRequest");

MODULE = Panda::WebSocket                PACKAGE = Panda::WebSocket
PROTOTYPES: DISABLE

BOOT {
    constant_t clist[] = {
        {"OPCODE_CONTINUE", (int)Opcode::CONTINUE},
        {"OPCODE_TEXT",     (int)Opcode::TEXT},
        {"OPCODE_BINARY",   (int)Opcode::BINARY},
        {"OPCODE_CLOSE",    (int)Opcode::CLOSE},
        {"OPCODE_PING",     (int)Opcode::PING},
        {"OPCODE_PONG",     (int)Opcode::PONG},

        {"CLOSE_DONE",             (int)CloseCode::DONE},
        {"CLOSE_AWAY",             (int)CloseCode::AWAY},
        {"CLOSE_PROTOCOL_ERROR",   (int)CloseCode::PROTOCOL_ERROR},
        {"CLOSE_INVALID_DATA",     (int)CloseCode::INVALID_DATA},
        {"CLOSE_UNKNOWN",          (int)CloseCode::UNKNOWN},
        {"CLOSE_ABNORMALLY",       (int)CloseCode::ABNORMALLY},
        {"CLOSE_INVALID_TEXT",     (int)CloseCode::INVALID_TEXT},
        {"CLOSE_BAD_REQUEST",      (int)CloseCode::BAD_REQUEST},
        {"CLOSE_MAX_SIZE",         (int)CloseCode::MAX_SIZE},
        {"CLOSE_EXTENSION_NEEDED", (int)CloseCode::EXTENSION_NEEDED},
        {"CLOSE_INTERNAL_ERROR",   (int)CloseCode::INTERNAL_ERROR},
        {"CLOSE_TLS",              (int)CloseCode::TLS},

        {NULL}
    };
    HV* stash = gv_stashpvs("Panda::WebSocket", GV_ADD);
    create_constants(aTHX_ stash, clist);
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

INCLUDE: FrameIterator.xsi

INCLUDE: Message.xsi

INCLUDE: MessageIterator.xsi
