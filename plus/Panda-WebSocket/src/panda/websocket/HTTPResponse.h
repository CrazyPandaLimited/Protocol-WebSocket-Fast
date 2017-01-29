#pragma once
#include <panda/uri.h>
#include <panda/refcnt.h>
#include <panda/websocket/HTTPPacket.h>

namespace panda { namespace websocket {

class HTTPResponse : public HTTPPacket {
public:
    int    code;
    string message;

    HTTPResponse () : code(0) {}

    //virtual void parse (StringRange range);

protected:
    virtual void _to_string (string& str);

};

}}
