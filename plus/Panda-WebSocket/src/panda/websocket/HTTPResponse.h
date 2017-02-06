#pragma once
#include <panda/refcnt.h>
#include <panda/websocket/HTTPPacket.h>

namespace panda { namespace websocket {

class HTTPResponse : public HTTPPacket {
public:
    int    code;
    string message;

    HTTPResponse () : code(0) {}

protected:
    virtual void _parse_header (StringRange range);
    virtual void _to_string    (string& str);

};

typedef panda::shared_ptr<HTTPResponse> HTTPResponseSP;

}}
