#pragma once

#include <panda/protocol/websocket/iterator.h>
#include <panda/protocol/websocket/inc.h>
#include <vector>

namespace panda { namespace protocol { namespace websocket {

class Parser;

/*
class MessageBuilder {
public:

    Opcode opcode() const noexcept;
    MessageBuilder& opcode(Opcode value) noexcept { _opcode = value; return *this; }

    MessageBuilder& deflate(bool value) noexcept { _deflate = value; return *this; }
    bool deflate() const noexcept { return _deflate;}

    StringPair send(string& payload);

    template <class It, typename = typename std::enable_if<std::is_same<typename It::value_type, string>::value>::type>
    StringChain<It> send(It payload_begin, It payload_end);

    template <class ContIt, typename = typename std::enable_if<std::is_same<decltype(*((*ContIt()).begin())), string&>::value>::type>
    std::vector<string> send(ContIt cont_begin, ContIt cont_end);

private:
    MessageBuilder(Parser& parser):_parser{parser}{}
    Parser& _parser;
    bool _deflate = true;
    Opcode _opcode = Opcode::BINARY;
    friend class Parser;
};
*/

}}}
