#pragma once
#include <panda/uri.h>
#include <panda/refcnt.h>
#include <panda/websocket/Parser.h>

namespace panda { namespace websocket {

using panda::uri::URI;
using panda::shared_ptr;

class AcceptRequest : public virtual panda::RefCounted {
public:
    string error;

    shared_ptr<URI> uri;
    HTTPHeaders     headers;

    AcceptRequest () {}

    ~AcceptRequest () {}
};

}}
