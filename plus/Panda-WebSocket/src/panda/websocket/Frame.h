#pragma once
#include <vector>
#include <cassert>
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
        PONG     = 0x0A,
    };


    string              error;
    std::vector<string> payload;

    Frame (bool mask_required, size_t max_size) :
        _mask_required(mask_required), _max_size(max_size), _state(FIRST), _len16(0), _length(0), _mask(0) {}

    bool     is_control     () const { return _opcode >= CLOSE; }
    Opcode   opcode         () const { return _opcode; }
    bool     final          () const { return _fin; }
    bool     has_mask       () const { return _has_mask; }
    size_t   payload_length () const { return _length; }
    uint16_t close_code     () const { return _close_code; }
    string   close_message  () const { return _close_message; }

    bool parse (string& buf);
    void reset ();

    void check (bool fragment_in_message) {
        assert(_state == DONE);
        if (is_control() || error) return;
        if (!fragment_in_message) {
            if (_opcode == Frame::CONTINUE) error = "initial frame can't have opcode CONTINUE";
        }
        else if (_opcode != Frame::CONTINUE) error = "fragment frame must have opcode CONTINUE";
    }

private:
    enum State { FIRST, SECOND, LENGTH, MASK, PAYLOAD, DONE };

    bool     _mask_required;
    size_t   _max_size;
    State    _state;
    bool     _fin;
    bool     _rsv1;
    bool     _rsv2;
    bool     _rsv3;
    bool     _has_mask;
    uint8_t  _slen;
    uint16_t _len16;
    uint64_t _length;
    Opcode   _opcode;
    uint32_t _mask;
    uint64_t _payload_bytes_left;
    uint16_t _close_code;
    string   _close_message;

#   pragma pack(push,1)
    struct BinaryFirst {
        uint8_t opcode : 4;
        bool    rsv3   : 1;
        bool    rsv2   : 1;
        bool    rsv1   : 1;
        bool    fin    : 1;
    };
    struct BinarySecond {
        uint8_t slen : 7;
        bool    mask : 1;
    };
#   pragma pack(pop)

public:
    static const uint16_t CLOSE_NORMAL           = 1000;
    static const uint16_t CLOSE_AWAY             = 1001;
    static const uint16_t CLOSE_PROTOCOL_ERROR   = 1002;
    static const uint16_t CLOSE_INVALID_DATA     = 1003;
    static const uint16_t CLOSE_UNKNOWN          = 1005; // NOT FOR SENDING
    static const uint16_t CLOSE_ABNORMALLY       = 1006; // NOT FOR SENDING
    static const uint16_t CLOSE_INVALID_TEXT     = 1007;
    static const uint16_t CLOSE_BAD_REQUEST      = 1008;
    static const uint16_t CLOSE_MAX_SIZE         = 1009;
    static const uint16_t CLOSE_EXTENSION_NEEDED = 1010; // FOR SENDING BY CLIENT ONLY
    static const uint16_t CLOSE_INTERNAL_ERROR   = 1011;
    static const uint16_t CLOSE_TLS              = 1015; // NOT FOR SENDING
};

typedef panda::shared_ptr<Frame> FrameSP;

}}
