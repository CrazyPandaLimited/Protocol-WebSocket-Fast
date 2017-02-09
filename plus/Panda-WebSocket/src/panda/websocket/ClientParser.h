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
    static const int STATE_CONNECTION_REQUESTED       = STATE_LAST + 1;
    static const int STATE_CONNECTION_RESPONSE_PARSED = STATE_CONNECTION_REQUESTED + 1;

    ConnectRequestSP  _connect_request;
    ConnectResponseSP _connect_response;

};

}}
