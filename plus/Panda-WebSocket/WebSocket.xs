#include <xs/xs.h>
#include <xs/lib.h>
#include <xs/uri.h>
#include <xs/export.h>
#include <algorithm_perlsafe> // early include algorithm to avoid collisions with fucking perl source code
#include <panda/websocket.h>
#include <iostream>

using std::cout;
using xs::lib::sv2string;

using xs::exp::constant_t;
using xs::exp::create_constant;
using xs::exp::create_constants;

using panda::uri::URI;
using xs::uri::XSURIWrapper;
using xs::uri::URIx;

using namespace panda::websocket;


MODULE = Panda::WebSocket                PACKAGE = Panda::WebSocket
PROTOTYPES: DISABLE

INCLUDE: Parser.xsi

INCLUDE: ServerParser.xsi
