#pragma once
#include <panda/uri.h>
#include <panda/refcnt.h>
#include <panda/protocol/websocket/HTTPPacket.h>
#include <panda/protocol/http/Request.h>

namespace panda { namespace protocol { namespace websocket {

using panda::uri::URI;

//class HTTPRequest : public HTTPPacket {
//public:
//    string    method;
//    iptr<URI> uri;

//    virtual void clear ();

//protected:
//    virtual void _parse_header (StringRange range);
//    virtual void _to_string    (string& str);
//};

using HTTPRequest = http::Request;
using HTTPRequestSP = panda::iptr<HTTPRequest>;

}}}
