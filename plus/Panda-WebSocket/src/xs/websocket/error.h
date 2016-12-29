#pragma once
#include <xs/xs.h>
#include <panda/websocket/error.h>

namespace xs { namespace websocket {

using panda::websocket::Error;

SV* error_sv (const Error& err, bool with_mess = false);

}}
