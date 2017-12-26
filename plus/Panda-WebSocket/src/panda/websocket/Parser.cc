#include <panda/websocket/Parser.h>
#include <cstdlib>
#include <cassert>
#include <iostream>
#include <panda/websocket/ParserError.h>

namespace panda { namespace websocket {

using std::cout;
using std::endl;

void Parser::reset () {
    _buffer.clear();
    _state.reset();
    _frame = NULL;
    _frame_count = 0;
    _message = NULL;
    _message_frame.reset();
}

FrameSP Parser::_get_frame () {
    if (!_state[STATE_ESTABLISHED]) {
        throw ParserError("not established");
    }
    if (_state[STATE_RECV_MESSAGE]) throw ParserError("message is being parsed");
    if (_state[STATE_RECV_CLOSED]) { _buffer.clear(); return NULL; }
    if (!_buffer) return NULL;

    _state.set(STATE_RECV_FRAME);
    if (!_frame) _frame = new Frame(_recv_mask_required, max_frame_size);

    if (!_frame->parse(_buffer)) {
        _buffer.clear();
        return NULL;
    }

    _frame->check(_frame_count);

    if (_frame->error) {
        _buffer.clear();
        _frame_count = 0;
    }
    else if (_frame->is_control()) { // control frames can't be fragmented, no need to increment frame count
        if (!_frame_count) _state.reset(STATE_RECV_FRAME); // do not reset state if control frame arrives in the middle of message
        if (_frame->opcode() == Opcode::CLOSE) {
            _buffer.clear();
            _state.set(STATE_RECV_CLOSED);
        }
    }
    else if (_frame->final()) {
        _state.reset(STATE_RECV_FRAME);
        _frame_count = 0;
    }
    else ++_frame_count;

    FrameSP ret(_frame);
    _frame = NULL;
    return ret;
}

MessageSP Parser::_get_message () {
    if (!_state[STATE_ESTABLISHED]) {
        throw ParserError("not established");
    }
    if (_state[STATE_RECV_FRAME]) throw ParserError("frame mode active");
    if (_state[STATE_RECV_CLOSED]) { _buffer.clear(); return NULL; }
    if (!_buffer) return NULL;

    _state.set(STATE_RECV_MESSAGE);
    if (!_message) _message = new Message(max_message_size);

    while (1) {
        if (!_message_frame.parse(_buffer)) {
            _buffer.clear();
            return NULL;
        }

        // control frame arrived in the middle of fragmented message - wrap in new message and return (state remains MESSAGE)
        // because user can only switch to getting frames after receiving non-control message
        if (!_message_frame.error && _message_frame.is_control()) {
            if (_message_frame.opcode() == Opcode::CLOSE) {
                _buffer.clear();
                _state.set(STATE_RECV_CLOSED);
            }
            if (_message->frame_count) {
                auto cntl_msg = new Message(max_message_size);
                bool done = cntl_msg->add_frame(_message_frame);
                assert(done);
                _message_frame.reset();
                return cntl_msg;
            }
        }

        _message_frame.check(_message->frame_count);
        bool done = _message->add_frame(_message_frame);
        _message_frame.reset();

        if (done) break;
        if (!_buffer) return NULL;
    }

    if (_message->error) _buffer.clear();

    _state.reset(STATE_RECV_MESSAGE);
    MessageSP ret(_message);
    _message = NULL;
    return ret;
}

FrameHeader Parser::_prepare_frame_header (bool final, Opcode opcode) {
    if (!_state[STATE_ESTABLISHED]) {
        throw ParserError("not established");
    }
    if (_state[STATE_SEND_MESSAGE]) throw ParserError("message is being sent");
    if (_state[STATE_SEND_CLOSED])  {
        throw ParserError("close sent, can't send anymore");
    }

    _state.set(STATE_SEND_FRAME);

    if (FrameHeader::is_control_opcode(opcode)) {
        if (!final) throw std::logic_error("control frame must be final");
        if (!_sent_frame_count) _state.reset(STATE_SEND_FRAME);
        if (opcode == Opcode::CLOSE) _state.set(STATE_SEND_CLOSED);
    } else {
        if (_sent_frame_count) opcode = Opcode::CONTINUE;
        if (final) {
            _sent_frame_count = 0;
            _state.reset(STATE_SEND_FRAME);
        }
        else ++_sent_frame_count;
    }

    return FrameHeader(opcode, final, 0, 0, 0, !_recv_mask_required, _recv_mask_required ? 0 : (uint32_t)std::rand());
}

}}
