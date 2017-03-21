#include <xs/websocket.h>
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
        HTTPPacket::HeaderValue elem;
        elem.name = sv2string(aTHX_ *ref);
        ref = av_fetch(elemav, 1, 0);
        if (ref && SvROK(*ref) && SvTYPE(SvRV(*ref)) == SVt_PVHV) {
            auto arghv = (HV*)SvRV(*ref);
            if (arghv) XS_HV_ITER(arghv, {
                STRLEN klen;
                char* key = HePV(he, klen);
                elem.params.emplace(string(key, klen), sv2string(aTHX_ HeVAL(he)));
            });
        }
        vals->push_back(std::move(elem));
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

SV* strings_to_sv (pTHX_ const string& s1, const string& s2) {
    auto len = s1.length() + s2.length();
    if (!len) return &PL_sv_undef;

    SV* ret = newSV(len+1);
    SvPOK_on(ret);
    char* dest = SvPVX(ret);
    std::memcpy(dest, s1.data(), s1.length());
    std::memcpy(dest + s1.length(), s2.data(), s2.length());
    dest[len] = 0;
    SvCUR_set(ret, len);

    return ret;
}

void av_to_vstring (pTHX_ AV* av, std::vector<string>& v) {
    XS_AV_ITER_NU(av, {
        STRLEN len;
        char* ptr = SvPV(elem, len);
        v.push_back(string(ptr, len));
    });
}

}}
