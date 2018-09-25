#pragma once
#include <panda/string.h>
#include <panda/protocol/websocket/Parser.h>
#include <panda/protocol/websocket/HTTPResponse.h>
#include <panda/protocol/websocket/ConnectRequest.h>
#include <panda/protocol/websocket/ConnectResponse.h>

namespace panda { namespace protocol { namespace websocket {

using panda::string;

class ServerParser : public Parser {
public:
    size_t max_handshake_size;

    ServerParser () : Parser(true), max_handshake_size(0) {}

    void use_deflate (const DeflateExt::ServerConfig& conf);

    bool accept_parsed () const { return _state[STATE_ACCEPT_PARSED]; }
    bool accepted      () const { return _state[STATE_ACCEPTED]; }

    ConnectRequestSP accept (string& buf);

    string accept_error    ();
    string accept_error    (HTTPResponse* res);
    string accept_response (ConnectResponse* res);

    string accept_response () {
        ConnectResponse res;
        return accept_response(&res);
    }

    virtual void reset ();

    virtual ~ServerParser () {}

private:
    static const int STATE_ACCEPT_PARSED = STATE_LAST + 1;
    static const int STATE_ACCEPTED      = STATE_ACCEPT_PARSED + 1;

    ConnectRequestSP _connect_request;
};

}}}
