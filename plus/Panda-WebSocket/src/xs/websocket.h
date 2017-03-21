#pragma once
#include <panda/websocket.h>
#include <xs/xs.h>

namespace xs { namespace websocket {

using namespace panda::websocket;

void av_to_header_values (pTHX_ AV* av, HTTPPacket::HeaderValues* vals);
AV*  header_values_to_av (pTHX_ const HTTPPacket::HeaderValues& vals);

void http_packet_set_headers (pTHX_ HTTPPacket* p, HV* headers);
void http_packet_set_body    (pTHX_ HTTPPacket* p, SV* body);

void av_to_vstring (pTHX_ AV* av, std::vector<string>& v);

template <class T>
SV* strings_to_sv (pTHX_ const T& v) {
    size_t len = 0;
    for (const string& s : v) len += s.length();
    if (!len) return &PL_sv_undef;

    SV* ret = newSV(len+1);
    SvPOK_on(ret);
    char* dest = SvPVX(ret);
    for (const string& s : v) {
        memcpy(dest, s.data(), s.length());
        dest += s.length();
    }
    *dest = 0;
    SvCUR_set(ret, len);

    return ret;
}

SV* strings_to_sv (pTHX_ const string& s1, const string& s2);


class XSFrameIterator : public FrameIterator {
public:
    XSFrameIterator (Parser* parser, const FrameSP& start_frame) : FrameIterator(parser, start_frame), nexted(false) { parser->retain(); }
    XSFrameIterator (const XSFrameIterator& oth)                 : FrameIterator(oth), nexted(oth.nexted)            { parser->retain(); }
    XSFrameIterator (const FrameIterator& oth)                   : FrameIterator(oth), nexted(false)                 { parser->retain(); }

    FrameSP next () {
        if (nexted) operator++();
        else nexted = true;
        return cur;
    }

    ~XSFrameIterator () { parser->release(); }
private:
    bool nexted;
};

class XSMessageIterator : public MessageIterator {
public:
    XSMessageIterator (Parser* parser, const MessageSP& start_msg) : MessageIterator(parser, start_msg), nexted(false) { parser->retain(); }
    XSMessageIterator (const XSMessageIterator& oth)               : MessageIterator(oth), nexted(oth.nexted)          { parser->retain(); }
    XSMessageIterator (const MessageIterator& oth)                 : MessageIterator(oth), nexted(false)               { parser->retain(); }

    MessageSP next () {
        if (nexted) operator++();
        else nexted = true;
        return cur;
    }

    ~XSMessageIterator () { parser->release(); }
private:
    bool nexted;
};

}}
