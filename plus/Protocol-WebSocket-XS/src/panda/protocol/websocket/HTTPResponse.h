#pragma once
#include <panda/refcnt.h>
#include <panda/protocol/websocket/HTTPPacket.h>
#include <panda/protocol/http/Response.h>

namespace panda { namespace protocol { namespace websocket {

//class HTTPResponse : public HTTPPacket {
//public:
//    int    code;
//    string message;

//    HTTPResponse () : code(0) {}

//protected:
//    virtual void _parse_header (StringRange range);
//    virtual void _to_string    (string& str);

//};

using HTTPResponse = http::Response;
using HTTPResponseSP = panda::iptr<HTTPResponse>;

}}}
