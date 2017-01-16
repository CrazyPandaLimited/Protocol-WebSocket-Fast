#pragma once
#include <vector>
#include <memory>
#include <iostream>
#include <panda/refcnt.h>
#include <panda/string.h>
#include <panda/websocket/Parser.h>
#include <panda/websocket/ParserError.h>
#include <panda/websocket/AcceptRequest.h>
#include <panda/ranges/KmpFinder.h>

namespace panda { namespace websocket {

using panda::string;
using panda::shared_ptr;

static auto fndr = panda::ranges::make_kmp_finder("\r\n\r\n");

class ServerParser : public Parser {
public:
    ServerParser ();

    shared_ptr<AcceptRequest> accept ();

    //void parse (const char* data, size_t len) { parse(string(data, len, string::COPY)); }
    //void parse (string data);

    virtual void reset ();

    virtual ~ServerParser ();

private:
    size_t         accept_finder_next_buf;
    decltype(fndr) accept_finder;

    AcceptRequest* parse_accept (Bufs::const_iterator begin, Bufs::const_iterator end);
};

}}
