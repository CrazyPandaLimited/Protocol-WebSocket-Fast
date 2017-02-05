#include <panda/websocket/Message.h>
#include <assert.h>

namespace panda { namespace websocket {

bool Message::add_frame (const Frame& frame) {
    assert(_state != DONE);

    if (frame.error) {
        error = frame.error;
        _state = DONE;
        return true;
    }

    if (!frame_count++) {
        _opcode = frame.opcode();
        _is_control = frame.is_control();
        if (_opcode == Frame::CLOSE) {
            _close_code    = frame.close_code();
            _close_message = frame.close_message();
        }
    }

    if (_max_size && _payload_length + frame.payload_length() > _max_size) {
        error = "max message size exceeded";
        _state = DONE;
        return true;
    }

    for (const auto& s : frame.payload) payload.push_back(s);
    _payload_length += frame.payload_length();

    if (frame.final()) _state = DONE;

    return _state == DONE;
}

}}
