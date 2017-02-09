#pragma once
#include <deque>
#include <bitset>
#include <iterator>
#include <panda/refcnt.h>
#include <panda/string.h>
#include <panda/iterator.h>
#include <panda/websocket/Frame.h>
#include <panda/websocket/Message.h>

namespace panda { namespace websocket {

using panda::string;
using panda::shared_ptr;

class Parser : public virtual panda::RefCounted {
public:

    class MessageIterator : public std::iterator<std::input_iterator_tag, MessageSP> {
    public:
        typedef panda::IteratorPair<MessageIterator> MessageIteratorPair;

        class FrameIterator : public std::iterator<std::input_iterator_tag, FrameSP> {
        public:
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
        typedef panda::IteratorPair<FrameIterator> FrameIteratorPair;

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

    size_t max_frame_size;
    size_t max_message_size;

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

    void send_frame (bool final, std::deque<string>& payload, Frame::Opcode opcode = Frame::BINARY);

    string send_control (Frame::Opcode opcode);
    string send_control (Frame::Opcode opcode, const string& payload);

    string send_ping ()                      { return send_control(Frame::PING); }
    string send_ping (const string& payload) { return send_control(Frame::PING, payload); }

    string send_pong ()                      { return send_control(Frame::PONG); }
    string send_pong (const string& payload) { return send_control(Frame::PONG, payload); }

    string send_close ()                                     { return send_control(Frame::CLOSE); }
    string send_close (uint16_t code)                        { return send_control(Frame::CLOSE, Frame::compile_close_payload(code, string())); }
    string send_close (uint16_t code, const string& payload) { return send_control(Frame::CLOSE, Frame::compile_close_payload(code, payload)); }

    virtual void reset ();

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

    std::bitset<32> _state;
    string          _buffer;

    Parser (bool recv_mask_required) :
        max_frame_size(0),
        max_message_size(0),
        _state(0),
        _recv_mask_required(recv_mask_required),
        _frame_count(0),
        _message_frame(_recv_mask_required, max_frame_size),
        _sent_frame_count(0)
    {}

private:
    bool      _recv_mask_required;
    FrameSP   _frame;            // current frame being received (frame mode)
    int       _frame_count;      // frame count for current message being received (frame mode)
    MessageSP _message;          // current message being received (message mode)
    Frame     _message_frame;    // current frame being received (message mode)
    int       _sent_frame_count; // frame count for current message being sent (frame mode)

    std::deque<string> _simple_payload_tmp;

    FrameSP   _get_frame ();
    MessageSP _get_message ();

};

typedef Parser::FrameIteratorPair   FrameIteratorPair;
typedef Parser::FrameIterator       FrameIterator;
typedef Parser::MessageIteratorPair MessageIteratorPair;
typedef Parser::MessageIterator     MessageIterator;
typedef shared_ptr<Parser>          ParserSP;

}}
