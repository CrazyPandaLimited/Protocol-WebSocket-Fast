#pragma once
#include <deque>
#include <map>
#include <unordered_map>
#include <panda/string.h>

namespace panda { namespace websocket {

using panda::string;

typedef std::multimap<const string, const string> HTTPHeaders;

class Parser {
public:
    size_t max_buffers_size;
    size_t max_message_size;

    bool add_buffer (const char* data, size_t len) { return add_buffer(string(data, len, string::COPY)); }
    bool add_buffer (const string& buf);

    //void parse (const char* data, size_t len) { parse(string(data, len, string::COPY)); }
    //void parse (string data);

    virtual void reset ();

    virtual ~Parser ();

protected:
    typedef std::deque<string> Bufs;

    size_t bufs_bytes;
    Bufs   bufs;
    bool   established;
    string key;

    Parser ();

private:

};

}}
