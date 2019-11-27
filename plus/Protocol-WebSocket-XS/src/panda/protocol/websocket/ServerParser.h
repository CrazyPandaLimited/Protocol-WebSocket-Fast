#pragma once
#include <panda/string.h>
#include <panda/optional.h>
#include <panda/protocol/websocket/Parser.h>
#include <panda/protocol/websocket/ConnectRequest.h>
#include <panda/protocol/websocket/ConnectResponse.h>
#include <panda/protocol/http/RequestParser.h>

namespace panda { namespace protocol { namespace websocket {

using panda::string;

struct ServerParser : Parser {
    ServerParser ();

    bool accept_parsed () const { return _state[STATE_ACCEPT_PARSED]; }
    bool accepted      () const { return _state[STATE_ACCEPTED]; }

    ConnectRequestSP accept (string& buf);

    string accept_error    ();
    string accept_error    (http::Response* res);
    string accept_response (ConnectResponse* res);

    string accept_response () {
        ConnectResponse res;
        return accept_response(&res);
    }

    virtual void reset ();

    virtual ~ServerParser ();

private:
    static const int STATE_ACCEPT_PARSED = STATE_LAST + 1;
    static const int STATE_ACCEPTED      = STATE_ACCEPT_PARSED + 1;

    http::RequestParser _connect_parser;
    ConnectRequestSP _connect_request;
};

}}}
