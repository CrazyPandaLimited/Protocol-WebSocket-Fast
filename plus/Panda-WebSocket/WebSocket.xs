#include <algorithm_perlsafe> // early include algorithm to avoid collisions with fucking perl source code
#include <xs/xs.h>
#include <xs/lib.h>
#include <xs/uri.h>
#include <xs/export.h>
#include <xs/websocket.h>

#include <iostream>

using std::cout;
using std::endl;

using xs::exp::constant_t;
using xs::exp::create_constant;
using xs::exp::create_constants;

using panda::string;
using xs::lib::sv2string;
using xs::my_perl;

using namespace xs::websocket;
using namespace panda::websocket;

static inline SV* new_sv_shared (const char* str) { return newSVpvn_share(str, strlen(str), 0); }

static SV*const http_response_class    = new_sv_shared("Panda::WebSocket::HTTPResponse");
static SV*const connect_response_class = new_sv_shared("Panda::WebSocket::ConnectResponse");

MODULE = Panda::WebSocket                PACKAGE = Panda::WebSocket
PROTOTYPES: DISABLE

BOOT {
    constant_t clist[] = {
        {"OPCODE_CONTINUE", Frame::CONTINUE},
        {"OPCODE_TEXT",     Frame::TEXT},
        {"OPCODE_BINARY",   Frame::BINARY},
        {"OPCODE_CLOSE",    Frame::CLOSE},
        {"OPCODE_PING",     Frame::PING},
        {"OPCODE_PONG",     Frame::PONG},

        {"CLOSE_NORMAL",           Frame::CLOSE_NORMAL},
        {"CLOSE_AWAY",             Frame::CLOSE_AWAY},
        {"CLOSE_PROTOCOL_ERROR",   Frame::CLOSE_PROTOCOL_ERROR},
        {"CLOSE_INVALID_DATA",     Frame::CLOSE_INVALID_DATA},
        {"CLOSE_UNKNOWN",          Frame::CLOSE_UNKNOWN},
        {"CLOSE_ABNORMALLY",       Frame::CLOSE_ABNORMALLY},
        {"CLOSE_INVALID_TEXT",     Frame::CLOSE_INVALID_TEXT},
        {"CLOSE_BAD_REQUEST",      Frame::CLOSE_BAD_REQUEST},
        {"CLOSE_MAX_SIZE",         Frame::CLOSE_MAX_SIZE},
        {"CLOSE_EXTENSION_NEEDED", Frame::CLOSE_EXTENSION_NEEDED},
        {"CLOSE_INTERNAL_ERROR",   Frame::CLOSE_INTERNAL_ERROR},
        {"CLOSE_TLS",              Frame::CLOSE_TLS},

        {NULL}
    };
    HV* stash = gv_stashpvs("Panda::WebSocket", GV_ADD);
    create_constants(aTHX_ stash, clist);
}

INCLUDE: Parser.xsi

INCLUDE: ServerParser.xsi

INCLUDE: HTTPPacket.xsi

INCLUDE: HTTPRequest.xsi

INCLUDE: HTTPResponse.xsi

INCLUDE: ConnectRequest.xsi

INCLUDE: ConnectResponse.xsi

INCLUDE: Frame.xsi

INCLUDE: FrameIterator.xsi

INCLUDE: Message.xsi

INCLUDE: MessageIterator.xsi
