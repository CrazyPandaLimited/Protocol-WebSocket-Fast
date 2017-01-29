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
