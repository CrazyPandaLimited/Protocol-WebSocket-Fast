#pragma once
#include <deque>
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

    bool established () const { return _established; }

    FrameIteratorPair   get_frames   (string& buf);
    MessageIteratorPair get_messages (string& buf);

    virtual void reset ();

    virtual ~Parser () {}

protected:
    bool _established;

    Parser (bool mask_required) :
        max_frame_size(0), max_message_size(0), _established(false), _mask_required(mask_required), _state(NONE), _frame_count(0),
        _message_frame(mask_required, max_frame_size) {}

private:
    enum State { NONE, FRAME, MESSAGE };

    bool      _mask_required;
    string    _buffer;
    State     _state;
    FrameSP   _frame;         // current frame (frame mode)
    int       _frame_count;   // frame count for current message (frame mode)
    MessageSP _message;       // current message (message mode)
    Frame     _message_frame; // current frame (message mode)

    FrameSP   _get_frame ();
    MessageSP _get_message ();

};

typedef Parser::FrameIteratorPair   FrameIteratorPair;
typedef Parser::FrameIterator       FrameIterator;
typedef Parser::MessageIteratorPair MessageIteratorPair;
typedef Parser::MessageIterator     MessageIterator;
typedef shared_ptr<Parser>          ParserSP;

}}
