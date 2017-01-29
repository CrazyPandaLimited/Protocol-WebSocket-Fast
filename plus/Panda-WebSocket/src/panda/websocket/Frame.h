#pragma once
#include <vector>
#include <panda/refcnt.h>
#include <panda/string.h>

namespace panda { namespace websocket {

using panda::string;

class Frame : public virtual panda::RefCounted {
public:
    enum Opcode {
        CONTINUE = 0x00,
        TEXT     = 0x01,
        BINARY   = 0x02,
        CLOSE    = 0x08,
        PING     = 0x09,
        PONG     = 0xA0,
    };

    string              error;
    std::vector<string> payload;

    Frame (size_t max_size = 0) : _header_parsed(false), _max_size(max_size) {}

    Opcode opcode () const { return _opcode; }

    bool parse (string& buf);

private:
    size_t   _max_size;
    bool     _header_parsed;
    bool     _fin;
    bool     _rsv1;
    bool     _rsv2;
    bool     _rsv3;
    uint8_t  _slen;
    Opcode   _opcode;
    uint64_t _length;
    char     _mask[4];


    //size_t
};

typedef panda::shared_ptr<Frame> FrameSP;

}}
