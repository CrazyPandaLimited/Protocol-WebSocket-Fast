#include <panda/protocol/websocket/Message.h>
#include <cassert>

namespace panda { namespace protocol { namespace websocket {

bool Message::add_frame (const Frame& frame) {
    assert(_state != State::DONE);

    if (frame.error) {
        error = frame.error;
        _state = State::DONE;
        return true;
    }

    if (!frame_count++) {
        _opcode = frame.opcode();
        if (_opcode == Opcode::CLOSE) {
            _close_code    = frame.close_code();
            _close_message = frame.close_message();
        }
    }

    if (_max_size && _payload_length + frame.payload_length() > _max_size) {
        error = "max message size exceeded";
        _state = State::DONE;
        return true;
    }

    for (const auto& s : frame.payload) payload.push_back(s);
    _payload_length += frame.payload_length();

    if (frame.final()) _state = State::DONE;

    return _state == State::DONE;
}

}}}
