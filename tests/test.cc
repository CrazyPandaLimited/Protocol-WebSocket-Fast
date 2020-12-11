#include "test.h"
#include <regex>
#include <panda/util.h>
#include <panda/endian.h>

bool ReMatcher::match (const string& matchee) const {
    auto flags = std::regex::ECMAScript;
    if (case_sen) flags |= std::regex::icase;
    auto reg = std::regex((std::string)re, flags);
    return std::regex_search((std::string)matchee, reg);
}

std::string ReMatcher::describe () const {
    return "matches " + ::Catch::Detail::stringify(re) + (case_sen ? " case sensitively" : " case insensitively");
}

void regex_replace (string& str, const std::string& re, const std::string& fmt) {
    str = string(std::regex_replace((std::string)str, std::regex(re, std::regex::ECMAScript|std::regex::icase), fmt));
}

std::vector<string> FrameGenerator::vec () const {
    if (!_msg_mode) throw std::runtime_error("vector return is only for message mode");
    return _gen_message();
}

string FrameGenerator::str () const {
    string ret;
    if (_msg_mode) {
        auto vec = _gen_message();
        for (auto& s : vec) ret += s;
    }
    else ret = _gen_frame();
    return ret;
}

static void crypt_xor (string& str, string_view key) {
    auto buf  = (unsigned char*)str.buf();
    auto kbuf = (unsigned char*)key.data();
    auto slen = str.length();
    auto klen = key.length();
    for (size_t i = 0; i < slen; ++i) buf[i] ^= kbuf[i % klen];
}

string FrameGenerator::_gen_frame () const {
    uint8_t first = 0, second = 0;

    for (auto v : {_fin, _rsv1, _rsv2, _rsv3}) {
        first |= v ? 1 : 0;
        first <<= 1;
    }

    first <<= 3;
    first |= ((int)_opcode & 15);

    second |= _mask ? 1 : 0;
    second <<= 7;

    auto data = _data;

    if (_close_code) {
        auto net_cc = h2be16(_close_code);
        data = string((char*)&net_cc, 2) + data;
    }

    auto dlen = data.length();
    string extlen;
    if (dlen < 126) {
        second |= dlen;
    }
    else if (dlen < 65536) {
        second |= 126;
        auto net_len = h2be16(dlen);
        extlen.assign((char*)&net_len, 2);
    }
    else {
        second |= 127;
        auto net_len = h2be64(dlen);
        extlen.assign((char*)&net_len, 8);
    }

    auto mask = _mask;
    if (mask.length()) {
        if (mask.length() != 4) {
            auto rnd = h2be32(rand());
            mask.assign((char*)&rnd, 4);
        }
        crypt_xor(data, mask);
    }

    return string() + (char)first + (char)second + extlen + mask + data;
}

std::vector<string> FrameGenerator::_gen_message () const {
    FrameGenerator g = *this;
    auto nframes  = _nframes ? _nframes : 1;
    auto opcode   = _opcode;
    auto payload  = _data;
    std::vector<string> ret;

    FrameGenerator gen = *this;
    auto frames_left = nframes;
    while (frames_left) {
        auto curlen = payload.length() / frames_left--;
        auto chunk = payload.substr(0, curlen);
        if (chunk.length() >= curlen) payload.offset(curlen);
        else payload.clear();

        ret.push_back(
            gen.opcode(opcode)
               .data(chunk)
               .fin(payload.empty())
               .mask(_mask)
               ._gen_frame()
        );
        opcode = Opcode::CONTINUE;
    }

    return ret;
}
