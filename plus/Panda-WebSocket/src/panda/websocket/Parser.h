#pragma once
#include <deque>
#include <iterator>
#include <panda/refcnt.h>
#include <panda/string.h>
#include <panda/iterator.h>
#include <panda/websocket/Frame.h>

namespace panda { namespace websocket {

using panda::string;
using panda::shared_ptr;

class Parser {
public:
    class FrameIterator : public std::iterator<std::input_iterator_tag, FrameSP> {
    public:
        FrameIterator (Parser* parser, const FrameSP& start_frame) : parser(parser), cur(start_frame) {}
        FrameIterator (const FrameIterator& oth)                   : parser(oth.parser), cur(oth.cur) {}

        FrameIterator& operator++ ()                               { cur = parser->_get_frame(); return *this; }
        FrameIterator  operator++ (int)                            { FrameIterator tmp(*this); operator++(); return tmp; }
        bool           operator== (const FrameIterator& rhs) const { return parser == rhs.parser && cur.get() == rhs.cur.get(); }
        bool           operator!= (const FrameIterator& rhs) const { return parser != rhs.parser || cur.get() != rhs.cur.get();}
        FrameSP        operator*  ()                               { return cur; }
        FrameSP        operator-> ()                               { return cur; }
    private:
        Parser* parser;
        FrameSP cur;
    };

    typedef panda::IteratorPair<FrameIterator> FrameIteratorPair;

    size_t max_frame_size;
    size_t max_message_size;

    bool established () const { return _established; }

    FrameIteratorPair get_frames (string& buf);

    virtual void reset ();

    virtual ~Parser () {}

protected:
    bool _established;

    Parser () : _established(false), max_frame_size(0), max_message_size(0) {}

    virtual FrameSP _get_frame ();

private:
    string  _buffer;
    FrameSP _frame;

};

}}
