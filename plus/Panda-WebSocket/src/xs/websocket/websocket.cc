#include <xs/websocket/websocket.h>
#include <xs/lib.h>

namespace xs { namespace websocket {

using xs::lib::sv2string;

void av_to_header_values (pTHX_ AV* av, HTTPPacket::HeaderValues* vals) {
    if (!av) return;
    auto len = AvFILLp(av)+1;
    if (!len) return;
    vals->reserve(len);
    XS_AV_ITER_NU(av, {
        if (!SvROK(elem) || SvTYPE(SvRV(elem)) != SVt_PVAV) continue;
        auto elemav = (AV*)SvRV(elem);
        SV** ref = av_fetch(elemav, 0, 0);
        if (!ref) continue;
        vals->push_back(HTTPPacket::HeaderValue{});
        auto& elem = vals->back();
        elem.name = sv2string(aTHX_ *ref);
        ref = av_fetch(elemav, 1, 0);
        if (!ref || !SvROK(*ref) || SvTYPE(SvRV(*ref)) != SVt_PVHV) continue;
        auto arghv = (HV*)SvRV(*ref);
        if (arghv) XS_HV_ITER(arghv, {
            STRLEN klen;
            char* key = HePV(he, klen);
            elem.params.emplace(string(key, klen), sv2string(aTHX_ HeVAL(he)));
        });
    });
}

AV* header_values_to_av (pTHX_ const HTTPPacket::HeaderValues& vals) {
    if (!vals.size()) return NULL;
    auto ret = newAV();
    for (const auto& elem : vals) {
        auto elemav = newAV();
        av_push(elemav, newSVpvn(elem.name.data(), elem.name.length()));
        if (elem.params.size()) {
            auto phv = newHV();
            for (const auto& param : elem.params)
                hv_store(phv, param.first.data(), param.first.length(), newSVpvn(param.second.data(), param.second.length()), 0);
            av_push(elemav, newRV_noinc((SV*)phv));
        }
        av_push(ret, newRV_noinc((SV*)elemav));
    }
    return ret;
}

void http_packet_set_headers (pTHX_ HTTPPacket* p, HV* hv) {
    p->headers.clear();
    if (!hv) return;
    XS_HV_ITER(hv, {
        STRLEN klen;
        char* key = HePV(he, klen);
        p->headers.emplace(string(key, klen), sv2string(aTHX_ HeVAL(he)));
    });
}

void http_packet_set_body (pTHX_ HTTPPacket* p, SV* sv) {
    p->body.clear();
    string newbody = sv2string(aTHX_ sv);
    if (newbody.length()) p->body.push_back(newbody);
}

SV* vector_string_to_sv (pTHX_ const std::vector<string>& v) {
    size_t len = 0;
    for (const auto& s : v) len += s.length();
    if (!len) return &PL_sv_undef;

    SV* ret = newSV(len+1);
    SvPOK_on(ret);
    char* dest = SvPVX(ret);
    for (const auto& s : v) {
        memcpy(dest, s.data(), s.length());
        dest += s.length();
    }
    SvCUR_set(ret, len);

    return ret;
}

}}
