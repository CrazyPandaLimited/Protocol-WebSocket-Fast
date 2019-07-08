#include <xs/protocol/websocket.h>

namespace xs { namespace protocol { namespace websocket {

void av_to_header_values (pTHX_ const Array& av, HeaderValues* vals) {
    if (!av.size()) return;
    vals->reserve(av.size());
    for (const auto& sv : av) {
        const Array subav(sv);
        if (!subav) continue;
        auto namesv = subav.fetch(0);
        if (!namesv) continue;
        HeaderValue elem;
        elem.name = xs::in<string>(aTHX_ namesv);
        Hash args = subav.fetch(1);
        if (args) for (const auto& row : args) elem.params.emplace(string(row.key()), xs::in<string>(aTHX_ row.value()));
        vals->push_back(std::move(elem));
    }
}

Array header_values_to_av (pTHX_ const HeaderValues& vals) {
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

void av_to_vstring (pTHX_ const Array& av, std::vector<string>& v) {
    for (const auto& elem : av) {
        if (!elem.defined()) continue;
        v.push_back(xs::in<string>(aTHX_ elem));
    }
}

}}}
