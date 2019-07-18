#pragma once
#include <panda/uri.h>
#include <panda/refcnt.h>
#include <panda/protocol/websocket/HTTPPacket.h>

namespace panda { namespace protocol { namespace websocket {

using panda::uri::URI;

struct HTTPRequest : HTTPPacket {
    string    method;
    iptr<URI> uri;

    virtual void clear ();

protected:
    virtual void _parse_header (StringRange range);
    virtual void _to_string    (string& str);
};

using HTTPRequestSP = panda::iptr<HTTPRequest>;

}}}
