#pragma once
#include <xs.h>
#include <panda/protocol/websocket.h>

namespace xs { namespace protocol { namespace websocket {

using namespace panda::protocol::websocket;

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
    char* dest = ret.get<char*>();
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

class XSFrameBuilder: public FrameBuilder {
public:
    XSFrameBuilder(FrameBuilder&& fb): FrameBuilder(std::move(fb)) {
        // keep link to make XSFrameBuilder perl-safe
        _parser.retain();
    }
    ~XSFrameBuilder() { _parser.release(); }
};

}}}

namespace xs {

    template <class TYPE>
    struct Typemap<panda::protocol::websocket::Parser*, TYPE> : TypemapObject<panda::protocol::websocket::Parser*, TYPE, ObjectTypeRefcntPtr, ObjectStorageMG> {};

    template <class TYPE>
    struct Typemap<panda::protocol::websocket::ClientParser*, TYPE> : Typemap<panda::protocol::websocket::Parser*, TYPE> {
        std::string package () { return "Protocol::WebSocket::XS::ClientParser"; }
    };

    template <class TYPE>
    struct Typemap<panda::protocol::websocket::ServerParser*, TYPE> : Typemap<panda::protocol::websocket::Parser*, TYPE> {
        std::string package () { return "Protocol::WebSocket::XS::ServerParser"; }
    };

    template <class TYPE>
    struct Typemap<panda::protocol::websocket::HTTPPacket*, TYPE> : TypemapObject<panda::protocol::websocket::HTTPPacket*, TYPE, ObjectTypeRefcntPtr, ObjectStorageMG> {};

    template <class TYPE>
    struct Typemap<panda::protocol::websocket::HTTPRequest*, TYPE> : Typemap<panda::protocol::websocket::HTTPPacket*, TYPE> {};

    template <class TYPE>
    struct Typemap<panda::protocol::websocket::HTTPResponse*, TYPE> : Typemap<panda::protocol::websocket::HTTPPacket*, TYPE> {
        std::string package () { return "Protocol::WebSocket::XS::HTTPResponse"; }
    };

    template <class TYPE>
    struct Typemap<panda::protocol::websocket::HTTPResponseSP, panda::iptr<TYPE>> : Typemap<TYPE*> {
        using Super = Typemap<TYPE*>;
        panda::iptr<TYPE> in (pTHX_ SV* arg) {
            Object obj = arg;
            if (!obj && SvOK(arg)) obj = Super::default_stash().call("new", arg);
            return Super::in(aTHX_ obj);
        }
    };

    template <class TYPE>
    struct Typemap<panda::protocol::websocket::ConnectRequest*, TYPE> : Typemap<panda::protocol::websocket::HTTPRequest*, TYPE> {
        std::string package () { return "Protocol::WebSocket::XS::ConnectRequest"; }
    };

    template <class TYPE>
    struct Typemap<panda::protocol::websocket::ConnectRequestSP, panda::iptr<TYPE>> : Typemap<TYPE*> {
        using Super = Typemap<TYPE*>;
        panda::iptr<TYPE> in (pTHX_ SV* arg) {
            Object obj = arg;
            if (!obj && SvOK(arg)) obj = Super::default_stash().call("new", arg);
            return Super::in(aTHX_ obj);
        }
    };

    template <class TYPE>
    struct Typemap<panda::protocol::websocket::ConnectResponse*, TYPE> : Typemap<panda::protocol::websocket::HTTPResponse*, TYPE> {
        std::string package () { return "Protocol::WebSocket::XS::ConnectResponse"; }
    };

    template <class TYPE>
    struct Typemap<panda::protocol::websocket::ConnectResponseSP, panda::iptr<TYPE>> : Typemap<TYPE*> {
        using Super = Typemap<TYPE*>;
        panda::iptr<TYPE> in (pTHX_ SV* arg) {
            Object obj = arg;
            if (!obj && SvOK(arg)) obj = Super::default_stash().call("new", arg);
            return Super::in(aTHX_ obj);
        }
    };

    template <class TYPE>
    struct Typemap<panda::protocol::websocket::Frame*, TYPE> : TypemapObject<panda::protocol::websocket::Frame*, TYPE, ObjectTypeRefcntPtr, ObjectStorageMG> {
        std::string package () { return "Protocol::WebSocket::XS::Frame"; }
    };

    template <class TYPE>
    struct Typemap<panda::protocol::websocket::Message*, TYPE> : TypemapObject<panda::protocol::websocket::Message*, TYPE, ObjectTypeRefcntPtr, ObjectStorageMG> {
        std::string package () { return "Protocol::WebSocket::XS::Message"; }
    };

    template <class TYPE>
    struct Typemap<xs::protocol::websocket::XSFrameIterator*, TYPE> : TypemapObject<xs::protocol::websocket::XSFrameIterator*, TYPE, ObjectTypePtr, ObjectStorageMG> {
        std::string package () { return "Protocol::WebSocket::XS::FrameIterator"; }
    };

    template <class TYPE>
    struct Typemap<xs::protocol::websocket::XSMessageIterator*, TYPE> : TypemapObject<xs::protocol::websocket::XSMessageIterator*, TYPE, ObjectTypePtr, ObjectStorageMG> {
        std::string package () { return "Protocol::WebSocket::XS::MessageIterator"; }
    };

    template <class TYPE>
    struct Typemap<xs::protocol::websocket::XSFrameBuilder*, TYPE> : TypemapObject<xs::protocol::websocket::XSFrameBuilder*, TYPE, ObjectTypePtr, ObjectStorageMG> {
        std::string package () { return "Protocol::WebSocket::XS::FrameBuilder"; }
    };


}
