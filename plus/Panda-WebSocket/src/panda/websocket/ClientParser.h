#pragma once
#include <panda/string.h>
#include <panda/websocket/Parser.h>
#include <panda/websocket/ConnectRequest.h>
#include <panda/websocket/ConnectResponse.h>

namespace panda { namespace websocket {

using panda::string;

class ClientParser : public Parser {
public:
    size_t max_handshake_size;

    ClientParser () : Parser(false), max_handshake_size(0) {}

    string connect_request (ConnectRequestSP& req);

    ConnectResponseSP connect (string& buf);

    virtual void reset ();

    virtual ~ClientParser () {}

private:
    ConnectRequestSP  _connect_request;
    ConnectResponseSP _connect_response;

};

}}
