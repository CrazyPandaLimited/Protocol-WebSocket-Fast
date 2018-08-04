#pragma once
#include <panda/websocket.h>
#include <xs/xs.h>

namespace xs { namespace websocket {

using namespace panda::websocket;

void  av_to_header_values (pTHX_ const Array& av, HTTPPacket::HeaderValues* vals);
Array header_values_to_av (pTHX_ const HTTPPacket::HeaderValues& vals);

void http_packet_set_headers (pTHX_ HTTPPacket* p, const Hash& headers);
void http_packet_set_body    (pTHX_ HTTPPacket* p, const Simple& body);

void av_to_vstring (pTHX_ const Array& av, std::vector<string>& v);

template <class T>
Simple strings_to_sv (pTHX_ const T& v) {
    size_t len = 0;
    for (const string& s : v) len += s.length();
    if (!len) return Simple::undef;

    auto ret = Simple::create(len);
    char* dest = ret;
    for (const string& s : v) {
        memcpy(dest, s.data(), s.length());
        dest += s.length();
    }
    *dest = 0;
    ret.length(len);
    return ret;
}

Simple strings_to_sv (pTHX_ const string& s1, const string& s2);


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

namespace xs {

    template <class TYPE>
    struct Typemap<panda::websocket::Parser*, TYPE> : TypemapObject<panda::websocket::Parser*, TYPE, ObjectTypeRefcntPtr, ObjectStorageMG> {};

    template <class TYPE>
    struct Typemap<panda::websocket::ClientParser*, TYPE> : Typemap<panda::websocket::Parser*, TYPE> {};

    template <class TYPE>
    struct Typemap<panda::websocket::ServerParser*, TYPE> : Typemap<panda::websocket::Parser*, TYPE> {};

    template <class TYPE>
    struct Typemap<panda::websocket::HTTPPacketSP, TYPE> : TypemapObject<panda::websocket::HTTPPacketSP, TYPE, ObjectTypeIPtr, ObjectStorageMG> {};

    template <class TYPE>
    struct Typemap<panda::websocket::HTTPRequestSP, TYPE> : Typemap<panda::websocket::HTTPPacketSP, TYPE> {};

    template <class TYPE>
    struct Typemap<panda::websocket::HTTPResponseSP, TYPE> : Typemap<panda::websocket::HTTPPacketSP, TYPE> {};

    template <class TYPE>
    struct Typemap<panda::websocket::ConnectRequestSP, TYPE> : Typemap<panda::websocket::HTTPRequestSP, TYPE> {
        std::string package () { return "Panda::WebSocket::ConnectRequest"; }
    };

    template <class TYPE>
    struct Typemap<panda::websocket::ConnectResponseSP, TYPE> : Typemap<panda::websocket::HTTPResponseSP, TYPE> {
        std::string package () { return "Panda::WebSocket::ConnectResponse"; }
    };

    template <class TYPE>
    struct Typemap<panda::websocket::FrameSP, TYPE> : TypemapObject<panda::websocket::FrameSP, TYPE, ObjectTypeIPtr, ObjectStorageMG> {
        std::string package () { return "Panda::WebSocket::Frame"; }
    };

    template <class TYPE>
    struct Typemap<panda::websocket::MessageSP, TYPE> : TypemapObject<panda::websocket::MessageSP, TYPE, ObjectTypeIPtr, ObjectStorageMG> {
        std::string package () { return "Panda::WebSocket::Message"; }
    };

    template <class TYPE>
    struct Typemap<xs::websocket::XSFrameIterator*, TYPE> : TypemapObject<xs::websocket::XSFrameIterator*, TYPE, ObjectTypePtr, ObjectStorageMG> {
        std::string package () { return "Panda::WebSocket::FrameIterator"; }
    };

    template <class TYPE>
    struct Typemap<xs::websocket::XSMessageIterator*, TYPE> : TypemapObject<xs::websocket::XSMessageIterator*, TYPE, ObjectTypePtr, ObjectStorageMG> {
        std::string package () { return "Panda::WebSocket::MessageIterator"; }
    };

}
