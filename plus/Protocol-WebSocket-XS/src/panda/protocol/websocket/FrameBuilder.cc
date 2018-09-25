#include <panda/protocol/websocket/FrameBuilder.h>
#include <panda/protocol/websocket/Parser.h>
#include <panda/protocol/websocket/ParserError.h>

namespace panda { namespace protocol { namespace websocket {

FrameBuilder::FrameBuilder(Parser& parser): _parser{parser}, _detached{false} {
    if (_parser._state[Parser::STATE_SEND_FRAME]) throw ParserError("previous message wasn't finished");
    _parser._state.set(Parser::STATE_SEND_FRAME);
}

FrameBuilder::FrameBuilder(FrameBuilder&& other):_parser{other._parser}, _detached{false},
    _finished{other._finished}, _final{other._final}, _deflate{other._deflate},
    _frame_index{other._frame_index}, _opcode{other._opcode} {
    other._detached = true;
}

FrameBuilder::~FrameBuilder() {
    if(!_detached && !_finished && _parser._deflate_ext) {
        _parser._deflate_ext->reset_tx();
    }
}

Opcode FrameBuilder::opcode() const noexcept {
    return _frame_index == 0 ? _opcode : Opcode::CONTINUE;
}


string FrameBuilder::send() {
    if (_finished) throw std::runtime_error("messsage is already finished");
    _finished = _final;
    return _parser.send_frame(*this);
}

StringPair FrameBuilder::send(string& payload) {
    if (_finished) throw std::runtime_error("messsage is already finished");
    _finished = _final;
    return _parser.send_frame(payload, *this);
}

}}}
