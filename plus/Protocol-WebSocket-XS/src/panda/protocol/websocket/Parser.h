#pragma once
#include <deque>
#include <bitset>
#include <iterator>
#include <panda/log.h>
#include <panda/refcnt.h>
#include <panda/string.h>
#include <panda/optional.h>
#include <panda/protocol/websocket/Frame.h>
#include <panda/protocol/websocket/Message.h>
#include <panda/protocol/websocket/iterator.h>
#include <panda/protocol/websocket/DeflateExt.h>
#include <panda/protocol/websocket/FrameBuilder.h>
#include <panda/protocol/websocket/ParserError.h>

namespace panda { namespace protocol { namespace websocket {

using panda::string;
using panda::IteratorPair;

class MessageBuilder;

struct Parser : virtual panda::Refcnt {
public:
    struct MessageIterator : std::iterator<std::input_iterator_tag, MessageSP> {
    public:
        typedef IteratorPair<MessageIterator> MessageIteratorPair;

        struct FrameIterator : std::iterator<std::input_iterator_tag, FrameSP> {
            FrameIterator (Parser* parser, const FrameSP& start_frame) : parser(parser), cur(start_frame) {}
            FrameIterator (const FrameIterator& oth)                   : parser(oth.parser), cur(oth.cur) {}

            FrameIterator& operator++ ()                               { if (cur) cur = parser->_get_frame(); return *this; }
            FrameIterator  operator++ (int)                            { FrameIterator tmp(*this); operator++(); return tmp; }
            bool           operator== (const FrameIterator& rhs) const { return parser == rhs.parser && cur.get() == rhs.cur.get(); }
            bool           operator!= (const FrameIterator& rhs) const { return parser != rhs.parser || cur.get() != rhs.cur.get();}
            FrameSP        operator*  ()                               { return cur; }
            FrameSP        operator-> ()                               { return cur; }

            MessageIteratorPair get_messages () {
                cur = NULL; // invalidate current iterator
                return MessageIteratorPair(MessageIterator(parser, parser->_get_message()), MessageIterator(parser, NULL));
            }
        protected:
            Parser* parser;
            FrameSP cur;
        };
        typedef IteratorPair<FrameIterator> FrameIteratorPair;

        MessageIterator (Parser* parser, const MessageSP& start_message) : parser(parser), cur(start_message) {}
        MessageIterator (const MessageIterator& oth)                     : parser(oth.parser), cur(oth.cur) {}

        MessageIterator& operator++ ()                                 { if (cur) cur = parser->_get_message(); return *this; }
        MessageIterator  operator++ (int)                              { MessageIterator tmp(*this); operator++(); return tmp; }
        bool             operator== (const MessageIterator& rhs) const { return parser == rhs.parser && cur.get() == rhs.cur.get(); }
        bool             operator!= (const MessageIterator& rhs) const { return parser != rhs.parser || cur.get() != rhs.cur.get();}
        MessageSP        operator*  ()                                 { return cur; }
        MessageSP        operator-> ()                                 { return cur; }

        FrameIteratorPair get_frames () {
            cur = NULL; // invalidate current iterator
            return FrameIteratorPair(FrameIterator(parser, parser->_get_frame()), FrameIterator(parser, NULL));
        }
    protected:
        Parser*   parser;
        MessageSP cur;
    };
    typedef MessageIterator::FrameIterator       FrameIterator;
    typedef MessageIterator::MessageIteratorPair MessageIteratorPair;
    typedef MessageIterator::FrameIteratorPair   FrameIteratorPair;
    typedef panda::optional<DeflateExt::Config>  DeflateConfigOption;

    struct Config {
        Config():max_frame_size{0}, max_message_size{0}, max_handshake_size{0}, deflate_config{ DeflateExt::Config() } {}
        size_t max_frame_size;
        size_t max_message_size;
        size_t max_handshake_size;
        DeflateConfigOption deflate_config;
    };

    bool established () const { return _state[STATE_ESTABLISHED]; }
    bool recv_closed () const { return _state[STATE_RECV_CLOSED]; }
    bool send_closed () const { return _state[STATE_SEND_CLOSED]; }

    FrameIteratorPair get_frames () {
        return FrameIteratorPair(FrameIterator(this, _get_frame()), FrameIterator(this, NULL));
    }

    MessageIteratorPair get_messages () {
        return MessageIteratorPair(MessageIterator(this, _get_message()), MessageIterator(this, NULL));
    }

    FrameIteratorPair get_frames (string& buf) {
        if (_buffer) _buffer += buf; // user has not iterated previous call to get_frames till the end or remainder after handshake on client side
        else         _buffer = buf;
        return get_frames();
    }

    MessageIteratorPair get_messages (string& buf) {
        if (_buffer) _buffer += buf; // user has not iterated previous call to get_frames till the end or remainder after handshake on client side
        else         _buffer = buf;
        return get_messages();
    }

    FrameBuilder start_message() {
        return FrameBuilder(*this);
    }

    MessageBuilder message();

    string     send_control (Opcode opcode)                  { return send_control_frame(opcode); }
    StringPair send_control (Opcode opcode, string& payload) {
        if (payload.length() > Frame::MAX_CONTROL_PAYLOAD) {
            panda_log_critical("control frame payload is too long");
            payload.offset(0, Frame::MAX_CONTROL_PAYLOAD);
        }
        return send_control_frame(payload, opcode);
    }

    string     send_ping  ()                { return send_control(Opcode::PING); }
    StringPair send_ping  (string& payload) { return send_control(Opcode::PING, payload); }
    string     send_pong  ()                { return send_control(Opcode::PONG); }
    StringPair send_pong  (string& payload) { return send_control(Opcode::PONG, payload); }
    string     send_close ()                { return send_control(Opcode::CLOSE); }

    StringPair send_close (uint16_t code, const string& payload = string()) {
        string frpld = FrameHeader::compile_close_payload(code, payload);
        return send_control(Opcode::CLOSE, frpld);
    }

    void configure(const Config& cfg) {
        _max_frame_size     = cfg.max_frame_size;
        _max_message_size   = cfg.max_message_size;
        _max_handshake_size = cfg.max_handshake_size;

        if (!_state[STATE_ESTABLISHED]) {
            _deflate_cfg = cfg.deflate_config;
            if (_deflate_cfg) _deflate_cfg->max_message_size = _max_message_size;
        }
    }

    size_t max_frame_size()     const { return  _max_frame_size; }
    size_t max_message_size()   const { return  _max_message_size; }
    size_t max_handshake_size() const { return  _max_handshake_size; }

    const DeflateConfigOption& deflate_config() const { return _deflate_cfg; }
    void no_deflate() {
        if (!_state[STATE_ESTABLISHED]) _deflate_cfg = DeflateConfigOption();
    }

    virtual void reset ();

    bool is_deflate_active() const { return (bool)_deflate_ext; }
    panda::optional<DeflateExt::Config> effective_deflate_config() const {
        using result_t = panda::optional<DeflateExt::Config>;
        if (!_deflate_ext) return result_t{};
        return result_t{ _deflate_ext->effective_config() };
    }

    virtual ~Parser () {}

protected:
    static const int STATE_ESTABLISHED  = 1;
    static const int STATE_RECV_FRAME   = 2;
    static const int STATE_RECV_MESSAGE = 3;
    static const int STATE_RECV_CLOSED  = 4;
    static const int STATE_SEND_FRAME   = 5;
    static const int STATE_SEND_MESSAGE = 6;
    static const int STATE_SEND_CLOSED  = 7;
    static const int STATE_LAST         = STATE_SEND_CLOSED;

    size_t _max_frame_size;
    size_t _max_message_size;
    size_t _max_handshake_size;

    std::bitset<32> _state;
    string          _buffer;
    std::unique_ptr<DeflateExt> _deflate_ext;

    Parser (bool recv_mask_required, Config cfg = Config()) :
        _max_frame_size{cfg.max_frame_size},
        _max_message_size{cfg.max_message_size},
        _max_handshake_size{cfg.max_handshake_size},
        _state(0),
        _deflate_cfg{cfg.deflate_config},
        _recv_mask_required(recv_mask_required),
        _frame_count(0),
        _message_frame(_recv_mask_required, _max_frame_size),
        _sent_frame_count(0)
    {}

    DeflateConfigOption _deflate_cfg;

private:
    string send_control_frame (Opcode opcode = Opcode::BINARY) {
        auto header = _prepare_frame_header(true, false, opcode);
        return Frame::compile(header);
    }

    StringPair send_control_frame (string& payload, Opcode opcode = Opcode::BINARY) {
        auto header = _prepare_frame_header(true, false, opcode);
        string hbin = Frame::compile(header, payload);
        return make_string_pair(hbin, payload);
    }

    string send_frame(const FrameBuilder& fb) {
        bool is_final = fb.final();
        auto header = _prepare_frame_header(is_final, false, fb.opcode());
        return Frame::compile(header);
    }

    StringPair send_frame(string& payload, const FrameBuilder& fb) {
        bool use_deflate = fb.deflate() && _deflate_ext;
        bool is_final = fb.final();
        auto header = _prepare_frame_header(is_final, use_deflate, fb.opcode());
        string& frame_payload = use_deflate ? _deflate_ext->compress(payload, is_final) : payload;
        string hbin = Frame::compile(header, frame_payload);
        return make_string_pair(hbin, frame_payload);
    }

    template<typename It>
    StringChain<It> send_frame(It payload_begin, It payload_end, const FrameBuilder& fb) {
        bool non_empty = payload_begin != payload_end;
        bool use_deflate = fb.deflate() && _deflate_ext && non_empty;
        bool is_final = fb.final();
        auto header = _prepare_frame_header(is_final, use_deflate, fb.opcode());
        It frame_payload_end = use_deflate ? _deflate_ext->compress(payload_begin, payload_end, is_final) : payload_end;
        string hbin = Frame::compile(header, payload_begin, frame_payload_end);
        return make_string_pair(hbin, payload_begin, frame_payload_end);
    }

    bool      _recv_mask_required;
    FrameSP   _frame;            // current frame being received (frame mode)
    int       _frame_count;      // frame count for current message being received (frame mode)
    MessageSP _message;          // current message being received (message mode)
    Frame     _message_frame;    // current frame being received (message mode)
    int       _sent_frame_count; // frame count for current message being sent (frame mode)

    std::deque<string> _simple_payload_tmp;

    FrameSP   _get_frame ();
    MessageSP _get_message ();

    FrameHeader _prepare_frame_header (bool final, bool deflate, Opcode opcode);
    friend class FrameBuilder;
    friend class MessageBuilder;

};

template<typename It>
StringChain<It> FrameBuilder::send(It payload_begin, It payload_end) {
    if (_finished) throw std::runtime_error("messsage is already finished");
    _finished = _final;
    return _parser.send_frame(payload_begin, payload_end, *this);
}

class MessageBuilder {
public:

    MessageBuilder(MessageBuilder&&) = default;
    Opcode opcode() const noexcept;
    MessageBuilder& opcode(Opcode value) noexcept { _opcode = value; return *this; }

    MessageBuilder& deflate(bool value) noexcept {
        _deflate = value ? apply_deflate_t::YES : apply_deflate_t::NO;
        return *this;
    }
    bool deflate() const noexcept { return _deflate == apply_deflate_t::YES; }

    StringPair send(string& payload) {
        bool apply_deflate = maybe_deflate(payload.length());
        return _parser.start_message().final(true).opcode(_opcode).deflate(apply_deflate).send(payload);
    }

    template <class It, typename = typename std::enable_if<std::is_same<typename It::value_type, string>::value>::type>
    StringChain<It> send(It payload_begin, It payload_end) {
        bool apply_deflate = maybe_deflate(std::distance(payload_begin, payload_end));
        return _parser.start_message().final(true).opcode(_opcode).deflate(apply_deflate).send(payload_begin, payload_end);
    }

    template <class ContIt, typename = typename std::enable_if<std::is_same<decltype(*((*ContIt()).begin())), string&>::value>::type>
    std::vector<string> send(ContIt cont_begin, ContIt cont_end) {
        std::vector<string> ret;

        size_t sz = 0, idx = 0, payload_sz = 0, last_nonempty = 0;
        auto cont_range = IteratorPair<ContIt>(cont_begin, cont_end);
        for (const auto& range : cont_range) {
            auto piece_size = range.end() - range.begin();
            sz += piece_size;
            for(const auto& it: range) payload_sz += it.length();
            if (payload_sz) last_nonempty = idx++;
        };

        auto fb = _parser.start_message();
        fb.opcode(_opcode);

        if (!payload_sz) {
            ret.reserve(1);
            ret.push_back(fb.deflate(false).final(true).send());
            return ret;
        }

        ret.reserve(sz);

        fb.deflate(maybe_deflate(payload_sz));
        for (size_t i = 0; i <= last_nonempty; ++i) {
            auto& range = cont_begin[i];
            auto frame_range = fb.final(i == last_nonempty).send(range.begin(), range.end());
            for (const auto& s : frame_range) ret.push_back(s);
        }

        return ret;
    }

private:
    enum class apply_deflate_t { YES, NO, TEXT_BY_THRESHOLD };

    bool maybe_deflate(size_t payload_length) {
        bool r = false;
        switch (_deflate) {
        case apply_deflate_t::NO:  r = false; break;
        case apply_deflate_t::YES: r = true; break;
        case apply_deflate_t::TEXT_BY_THRESHOLD: r
                =  _opcode == Opcode::TEXT
                && _parser._deflate_cfg
                && _parser._deflate_cfg->compression_threshold <= payload_length;
            break;
        }
        return r;
    }

    MessageBuilder(Parser& parser):_parser{parser}{}
    MessageBuilder(MessageBuilder&) = delete;
    Parser& _parser;
    apply_deflate_t _deflate = apply_deflate_t::TEXT_BY_THRESHOLD;
    Opcode _opcode = Opcode::BINARY;
    friend struct Parser;
};

using FrameIteratorPair   = Parser::FrameIteratorPair;
using FrameIterator       = Parser::FrameIterator;
using MessageIteratorPair = Parser::MessageIteratorPair;
using MessageIterator     = Parser::MessageIterator;
using ParserSP            = iptr<Parser>;

}}}
