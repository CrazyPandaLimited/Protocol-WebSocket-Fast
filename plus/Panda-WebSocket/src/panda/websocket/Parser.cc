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
    if (_buffer) _buffer += buf; // really not good situation, that means that user has not iterated previous call to get_frame till the end
    else         _buffer = buf;
    return FrameIteratorPair(FrameIterator(this, _get_frame()), FrameIterator(this, NULL));
}

FrameSP Parser::_get_frame () {
    if (!_buffer) return NULL;
    if (!_frame) _frame = new Frame(max_frame_size);

    if (!_frame->parse(_buffer)) {
        _buffer.clear();
        return NULL;
    }

    FrameSP ret(_frame);
    _frame = NULL;
    return ret;
}

}}
