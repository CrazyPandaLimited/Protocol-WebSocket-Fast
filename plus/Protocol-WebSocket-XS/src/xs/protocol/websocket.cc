#include <xs/protocol/websocket.h>

namespace xs { namespace protocol { namespace websocket {

void av_to_header_values (const Array& av, HTTPPacket::HeaderValues* vals) {
    if (!av.size()) return;
    vals->reserve(av.size());
    for (const auto& sv : av) {
        const Array subav(sv);
        if (!subav) continue;
        auto namesv = subav.fetch(0);
        if (!namesv) continue;
        HTTPPacket::HeaderValue elem;
        elem.name = xs::in<string>(namesv);
        Hash args = subav.fetch(1);
        if (args) for (const auto& row : args) elem.params.emplace(string(row.key()), xs::in<string>(row.value()));
        vals->push_back(std::move(elem));
    }
}

Array header_values_to_av (const HTTPPacket::HeaderValues& vals) {
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

void http_packet_set_headers (HTTPPacket* p, const Hash& hv) {
    p->headers.clear();
    for (const auto& row : hv) p->headers.emplace(string(row.key()), xs::in<string>(row.value()));
}

void http_packet_set_body (HTTPPacket* p, const Simple& sv) {
    p->body.clear();
    auto newbody = xs::in<string>(sv);
    if (newbody.length()) p->body.push_back(newbody);
}

Simple strings_to_sv (const string& s1, const string& s2) {
    auto len = s1.length() + s2.length();
    if (!len) return Simple::undef;

    auto ret = Simple::create(len);
    char* dest = SvPVX(ret);
    memcpy(dest, s1.data(), s1.length());
    memcpy(dest + s1.length(), s2.data(), s2.length());
    dest[len] = 0;
    ret.length(len);
    return ret;
}

void av_to_vstring (const Array& av, std::vector<string>& v) {
    for (const auto& elem : av) {
        if (!elem.defined()) continue;
        v.push_back(xs::in<string>(elem));
    }
}

}}}
