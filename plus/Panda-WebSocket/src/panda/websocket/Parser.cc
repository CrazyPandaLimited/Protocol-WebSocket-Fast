#include <panda/websocket/Parser.h>
#include <iostream>
#include <cassert>

namespace panda { namespace websocket {

using std::cout;
using std::endl;

void Parser::reset () {
    _established = false;
    _buffer.clear();
    _state = NONE;
    _frame = NULL;
    _frame_count = 0;
    _message = NULL;
    _message_frame.reset();
}

FrameSP Parser::_get_frame () {
    if (_state == MESSAGE) throw std::logic_error(
        "Parser[_get_frame] can't get frame: message is being parsed. You can't get frames until you receive one next message."
    );
    if (!_buffer) return NULL;
    _state = FRAME;
    if (!_frame) _frame = new Frame(_mask_required, max_frame_size);

    if (!_frame->parse(_buffer)) {
        _buffer.clear();
        return NULL;
    }

    _frame->check(_frame_count);

    if (_frame->error) {
        _buffer.clear();
        _frame_count = 0;
        _state = NONE;
    }
    else if (_frame->is_control()) { // control frames can't be fragmented, no need to increment frame count
        if (!_frame_count) _state = NONE; // do not reset state if control frame arrives in the middle of message
    }
    else if (_frame->final()) {
        _state = NONE;
        _frame_count = 0;
    }
    else ++_frame_count;

    FrameSP ret(_frame);
    _frame = NULL;
    return ret;
}

MessageSP Parser::_get_message () {
    if (_state == FRAME) throw std::logic_error(
        "Parser[_get_message] can't get message: you are getting frames. "
        "You can't get messages until you receive all the frames of current message (till frame->final() is true, not counting control frames)"
    );
    if (!_buffer) return NULL;
    _state = MESSAGE;
    if (!_message) _message = new Message(max_message_size);

    while (1) {
        if (!_message_frame.parse(_buffer)) {
            _buffer.clear();
            return NULL;
        }

        // control frame arrived in the middle of fragmented message - wrap in new message and return (state remains MESSAGE)
        // because user can only switch to getting frames after receiving non-control message
        if (!_message_frame.error && _message_frame.is_control() && _message->frame_count) {
            auto cntl_msg = new Message(max_message_size);
            bool done = cntl_msg->add_frame(_message_frame);
            assert(done);
            _message_frame.reset();
            return cntl_msg;
        }

        _message_frame.check(_message->frame_count);
        bool done = _message->add_frame(_message_frame);
        _message_frame.reset();

        if (done) break;
        if (!_buffer) return NULL;
    }

    if (_message->error) _buffer.clear();

    _state = NONE;
    MessageSP ret(_message);
    _message = NULL;
    return ret;
}

void Parser::send_frame (bool final, std::deque<string>& payload, Frame::Opcode opcode) {
    if (Frame::is_control_opcode(opcode)) {
        if (!final) throw std::logic_error("Parser[send_frame] control frame must be final");
    } else {
        if (_sent_frame_count) opcode = Frame::CONTINUE;
        if (final) _sent_frame_count = 0;
        else ++_sent_frame_count;
    }

    for (auto& str : payload) {
        // TODO: change str with extensions
    }
    Frame::compile(final, 0, 0, 0, !_mask_required, opcode, payload);
}

}}
