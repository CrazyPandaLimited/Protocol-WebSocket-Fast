#pragma once
#include <panda/string.h>
#include <panda/protocol/websocket/Parser.h>
#include <panda/protocol/websocket/ConnectRequest.h>
#include <panda/protocol/websocket/ConnectResponse.h>
#include <panda/protocol/http/ResponseParser.h>

namespace panda { namespace protocol { namespace websocket {

using panda::string;

struct ClientParser : Parser {

    ClientParser () : Parser(false), _connect_response_parser(new http::ResponseParser) {}

    string connect_request (ConnectRequestSP& req);

    ConnectResponseSP connect (string& buf);

    virtual void reset ();

    virtual ~ClientParser () {}

private:
    static const int STATE_CONNECTION_REQUESTED       = STATE_LAST + 1;
    static const int STATE_CONNECTION_RESPONSE_PARSED = STATE_CONNECTION_REQUESTED + 1;

    ConnectRequestSP  _connect_request;
    ConnectResponseSP _connect_response;
    http::ResponseParserSP _connect_response_parser;
};

}}}
