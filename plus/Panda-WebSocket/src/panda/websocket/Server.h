#pragma once
#include <vector>
#include <panda/refcnt.h>
#include <panda/websocket/inc.h>
#include <panda/websocket/error.h>

namespace panda { namespace websocket {

using panda::string;
using panda::event::Loop;

struct Location {
    string   host;
    uint16_t port;
    bool     secure;
};

typedef std::vector<Location> Locations;

struct ServerConfig {
    Loop*     loop;
    Locations locations;
};

class Server : public virtual RefCounted {
public:

    Server ();
    
    void init (ServerConfig config) throw(ConfigError);
    
    void run ();
    
    ~Server ();

private:
    
    Loop*     loop;
    Locations locations;

};

}}