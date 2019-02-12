#include <xs/protocol/websocket.h>

namespace xs { namespace protocol { namespace websocket {

void av_to_header_values (pTHX_ const Array& av, HTTPPacket::HeaderValues* vals) {
    if (!av.size()) return;
    vals->reserve(av.size());
    for (const auto& sv : av) {
        const Array subav(sv);
        if (!subav) continue;
        auto namesv = subav.fetch(0);
        if (!namesv) continue;
        HTTPPacket::HeaderValue elem;
        elem.name = xs::in<string>(aTHX_ namesv);
        Hash args = subav.fetch(1);
        if (args) for (const auto& row : args) elem.params.emplace(string(row.key()), xs::in<string>(aTHX_ row.value()));
        vals->push_back(std::move(elem));
    }
}

Array header_values_to_av (pTHX_ const HTTPPacket::HeaderValues& vals) {
    if (!vals.size()) return Array();
    auto ret = Array::create(vals.size());
    for (const auto& elem : vals) {
        auto elemav = Array::create(2);
        elemav.push(xs::out(elem.name));
        if (elem.params.size()) {
            auto args = Hash::create(elem.params.size());
            for (const auto& param : elem.params) {
                args.store(param.first, xs::out(param.second));
            }
            elemav.push(Ref::create(args));
        }
        ret.push(Ref::create(elemav));
    }
    return ret;
}

void http_packet_set_headers (pTHX_ HTTPPacket* p, const Hash& hv) {
    p->headers.clear();
    for (const auto& row : hv) p->headers.emplace(string(row.key()), xs::in<string>(aTHX_ row.value()));
}

void http_packet_set_body (pTHX_ HTTPPacket* p, const Simple& sv) {
    p->body()->parts.clear();
    auto newbody = xs::in<string>(aTHX_ sv);
    if (newbody.length()) p->body()->parts.push_back(newbody);
}

Simple strings_to_sv (pTHX_ const string& s1, const string& s2) {
    auto len = s1.length() + s2.length();
    if (!len) return Simple::undef;

    auto ret = Simple::create(len);
    char* dest = SvPVX(ret);
    std::memcpy(dest, s1.data(), s1.length());
    std::memcpy(dest + s1.length(), s2.data(), s2.length());
    dest[len] = 0;
    ret.length(len);
    return ret;
}

void av_to_vstring (pTHX_ const Array& av, std::vector<string>& v) {
    for (const auto& elem : av) {
        if (!elem.defined()) continue;
        v.push_back(xs::in<string>(aTHX_ elem));
    }
}

}}}
