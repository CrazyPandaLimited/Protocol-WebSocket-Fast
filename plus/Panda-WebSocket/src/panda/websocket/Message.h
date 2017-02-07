#pragma once
#include <vector>
#include <panda/refcnt.h>
#include <panda/string.h>
#include <panda/websocket/Frame.h>

namespace panda { namespace websocket {

using panda::string;

class Message : public virtual panda::RefCounted {
public:
    typedef Frame::Opcode Opcode;

    string              error;
    std::vector<string> payload;
    uint32_t            frame_count;

    Message (size_t max_size) : frame_count(0), _max_size(max_size), _state(PENDING), _payload_length(0) {}

    Opcode   opcode         () const { return _opcode; }
    bool     is_control     () const { return Frame::is_control_opcode(_opcode); }
    uint16_t close_code     () const { return _close_code; }
    string   close_message  () const { return _close_message; }
    size_t   payload_length () const { return _payload_length; }

    bool add_frame (const Frame& frame);

private:
    enum State { PENDING, DONE };

    size_t   _max_size;
    State    _state;
    Opcode   _opcode;
    uint16_t _close_code;
    string   _close_message;
    size_t   _payload_length;

};

typedef panda::shared_ptr<Message> MessageSP;

}}
