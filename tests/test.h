#pragma once
#include <catch2/catch.hpp>
#include <panda/protocol/websocket.h>

namespace test {

using namespace panda;
using namespace panda::protocol::websocket;
using panda::protocol::http::Headers;

struct ReMatcher : Catch::MatcherBase<string> {
    ReMatcher (const string& regex, bool case_sen = false) : re(regex), case_sen(case_sen) {}
    bool match (const string& matchee) const override;
    std::string describe () const override;
private:
    string re;
    bool   case_sen;
};

inline ReMatcher MatchesRe (const string& regex, bool case_sen = false) { return ReMatcher(regex, case_sen); }

void regex_replace (string&, const std::string&, const std::string&);

std::vector<string> accept_packet ();

struct FrameGenerator {
    FrameGenerator& opcode     (Opcode v) { _opcode = v; return *this; }
    FrameGenerator& mask       (string v) { _mask = v; return *this; }
    FrameGenerator& mask       ()         { return mask("autogen"); }
    FrameGenerator& fin        (bool v)   { _fin = v; return *this; }
    FrameGenerator& fin        ()         { return fin(true); }
    FrameGenerator& rsv1       ()         { _rsv1 = true; return *this; }
    FrameGenerator& rsv2       ()         { _rsv2 = true; return *this; }
    FrameGenerator& rsv3       ()         { _rsv3 = true; return *this; }
    FrameGenerator& data       (string v) { _data = v; return *this; }
    FrameGenerator& nframes    (int v)    { _nframes = v; return *this; }
    FrameGenerator& close_code (int v)    { _close_code = v; return *this; }

    operator string () const { return str(); }

    string              str() const;
    std::vector<string> vec() const;

    FrameGenerator& msg_mode () { _msg_mode = true; return *this; }

private:
    Opcode _opcode = Opcode::TEXT;
    string _mask;
    bool   _fin  = false;
    bool   _rsv1 = false;
    bool   _rsv2 = false;
    bool   _rsv3 = false;
    string _data;
    int    _close_code = 0;
    int    _nframes = 1;
    bool   _msg_mode = false;

    string              _gen_frame   () const;
    std::vector<string> _gen_message () const;
};

inline FrameGenerator gen_frame   () { return {}; }
inline FrameGenerator gen_message () { return FrameGenerator().msg_mode(); }

template <class T>
inline MessageSP get_message (const T& parser) {
    auto msgs = parser->get_messages();
    if (msgs.begin() == msgs.end()) return {};
    return *(msgs.begin());
}

}

using namespace test;

namespace panda { namespace protocol { namespace websocket {
    inline bool operator== (const HeaderValue& lhs, const HeaderValue& rhs) {
        return lhs.name == rhs.name && lhs.params == rhs.params;
    }
}}}
