#pragma once

#include <panda/protocol/websocket/iterator.h>
#include <panda/protocol/websocket/inc.h>

namespace panda { namespace protocol { namespace websocket {

struct Parser;

struct FrameBuilder {
    FrameBuilder(FrameBuilder&& other);
    virtual ~FrameBuilder();
    FrameBuilder(FrameBuilder&) = delete;

    Opcode opcode() const noexcept;
    FrameBuilder& opcode(Opcode value) noexcept { _opcode = value; return *this; }

    FrameBuilder& final(bool value) noexcept { _final = value; return *this; }
    bool final() const noexcept { return _final;}

    FrameBuilder& deflate(bool value) noexcept;
    bool deflate() const noexcept;

    string send();

    StringPair send(string& payload);

    template<class Begin, class End>
    StringChain<Begin, End> send(Begin payload_begin, End payload_end);

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

    friend struct Parser;
};

}}}
