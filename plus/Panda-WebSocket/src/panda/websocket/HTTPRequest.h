#pragma once
#include <panda/uri.h>
#include <panda/refcnt.h>
#include <panda/websocket/HTTPPacket.h>

namespace panda { namespace websocket {

using panda::uri::URI;
using panda::shared_ptr;

class HTTPRequest : public HTTPPacket {
public:
    string          method;
    shared_ptr<URI> uri;

    virtual void clear ();

protected:
    virtual void _parse_header (StringRange range);

};

}}
