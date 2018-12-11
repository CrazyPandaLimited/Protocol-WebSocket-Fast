#pragma once

#include <panda/protocol/websocket/iterator.h>
#include <panda/protocol/websocket/inc.h>

namespace panda { namespace protocol { namespace websocket {

struct Parser;

class FrameBuilder {
public:

    FrameBuilder(FrameBuilder&& other);
    virtual ~FrameBuilder();
    FrameBuilder(FrameBuilder&) = delete;

    Opcode opcode() const noexcept;
    FrameBuilder& opcode(Opcode value) noexcept { _opcode = value; return *this; }

    FrameBuilder& final(bool value) noexcept { _final = value; return *this; }
    bool final() const noexcept { return _final;}

    FrameBuilder& deflate(bool value) noexcept { _deflate = value; return *this; }
    bool deflate() const noexcept { return _deflate;}

    string send();

    StringPair send(string& payload);

    template<typename It>
    StringChain<It> send(It payload_begin, It payload_end);

protected:
    FrameBuilder(Parser& parser);

    Parser& _parser;
    bool _detached;
    bool _finished = false;

private:
    bool _final = false;
    bool _deflate = true;
    std::uint32_t _frame_index = 0;
    Opcode _opcode = Opcode::BINARY;

    friend class Parser;
};

}}}
