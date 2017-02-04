#include <panda/websocket/Parser.h>
#include <iostream>
#include <assert.h>

namespace panda { namespace websocket {

using std::cout;
using std::endl;

void Parser::reset () {
    _established = false;
    _buffer.clear();
    _frame = NULL;
}

Parser::FrameIteratorPair Parser::get_frames (string& buf) {
    if (!_established) throw std::logic_error("Parser[get_frames] connection not established");
    if (_buffer) _buffer += buf; // really not good situation, that means that user has not iterated previous call to get_frames till the end
    else         _buffer = buf;
    return FrameIteratorPair(FrameIterator(this, _get_frame()), FrameIterator(this, NULL));
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

    if (_frame->error) _buffer.clear();
    else if (_frame->final() && !(_frame->is_control() && _frame_count)) _state = NONE;
    ++_frame_count;

    FrameSP ret(_frame);
    _frame = NULL;
    return ret;
}

Parser::MessageIteratorPair Parser::get_messages (string& buf) {
    if (!_established) throw std::logic_error("Parser[get_messages] connection not established");
    if (_buffer) _buffer += buf; // really not good situation, that means that user has not iterated previous call to get_messages till the end
    else         _buffer = buf;
    return MessageIteratorPair(MessageIterator(this, _get_message()), MessageIterator(this, NULL));
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

}}
