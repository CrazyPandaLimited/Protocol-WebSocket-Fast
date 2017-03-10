#pragma once
#include <cstdint>
#include <strings.h> // strncasecmp
#include <panda/string.h>

namespace panda { namespace websocket {

uint32_t string_hash32_ci (const char *key, size_t len);

struct string_hash_ci {
    size_t operator() (const string& s) const {
        return string_hash32_ci(s.data(), s.length());
    }
};

struct string_equal_ci {
    bool operator() (const string& lhs, const string& rhs) const {
        return lhs.length() == rhs.length() && strncasecmp(lhs.data(), rhs.data(), lhs.length()) == 0;
    }
};

template <typename T>
inline bool parse_binary_number (T& num, const char*& src, size_t len) {
    if (!num && len >= sizeof(T)) { // common case - have whole number in src
        num = *((T*)src);
        src += sizeof(T);
        return true;
    }

    uint8_t bcnt = ((char*)&num)[sizeof(T)-1];
    char* dst = (char*)&num + bcnt;

    if (bcnt + len >= sizeof(T)) { // have everything left in src
        char* end = (char*)&num + sizeof(T);
        while (dst != end) *dst++ = *src++;
        return true;
    }

    // still not enough
    ((char*)&num)[sizeof(T)-1] = bcnt+len;
    char* end = dst + len;
    while (dst != end) *dst++ = *src++;
    return false;
}

void crypt_mask (char* str, size_t len, uint32_t mask, uint64_t bytes_received);

}}
