#pragma once
#include <vector>
#include <memory>
#include <iostream>
#include <panda/refcnt.h>
#include <panda/string.h>
#include <panda/websocket/Parser.h>
#include <panda/websocket/ParserError.h>
#include <panda/websocket/HTTPResponse.h>
#include <panda/websocket/ConnectRequest.h>
#include <panda/websocket/ConnectResponse.h>

namespace panda { namespace websocket {

using panda::string;
using panda::shared_ptr;

class ServerParser : public Parser {
public:
    size_t max_accept_size;

    ServerParser () : Parser(true), max_accept_size(0), _accepted(false) {}

    bool accepted () const { return _accepted; }

    ConnectRequestSP accept (string& buf);

    string accept_error    ();
    string accept_error    (HTTPResponse* res);
    string accept_response (ConnectResponse* res);

    string accept_response () {
        ConnectResponse res;
        return accept_response(&res);
    }

    virtual void reset ();

    virtual ~ServerParser ();

private:
    ConnectRequestSP connect_request;
    bool             _accepted;

};

}}
