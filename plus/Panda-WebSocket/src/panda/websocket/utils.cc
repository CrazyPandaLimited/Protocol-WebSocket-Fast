#include <panda/websocket/utils.h>

namespace panda { namespace websocket {

union _check_endianess { unsigned x; unsigned char c; };
static const bool am_i_little = (_check_endianess{1}).c;

// case-insensitive jenkins_one_at_a_time_hash
uint32_t string_hash32_ci (const char *key, size_t len) {
    uint32_t hash, i;
    for (hash = i = 0; i < len; ++i) {
        hash += std::tolower(key[i]);
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}

static inline uint32_t rotate_shift (uint32_t x, unsigned shift) {
    return am_i_little ? ((x >> shift) | (x << (sizeof(x)*8 - shift))) :
                         ((x << shift) | (x >> (sizeof(x)*8 - shift)));
}

void crypt_mask (char* str, size_t len, uint32_t mask, uint64_t bytes_received) {
    mask = rotate_shift(mask, (bytes_received & 3)*8);
    const uint64_t mask64 = ((uint64_t)mask << 32) | mask;
    auto str64 = (uint64_t*)str;
    auto end64 = str64 + (len / 8);

    while (str64 != end64) *str64++ ^= mask64;

    auto cstr  = (unsigned char*)str64;
    auto cmask = (const unsigned char*)&mask64;
    switch (len & 7) {
        case 7: *cstr++ ^= *cmask++;
        case 6: *cstr++ ^= *cmask++;
        case 5: *cstr++ ^= *cmask++;
        case 4: *cstr++ ^= *cmask++;
        case 3: *cstr++ ^= *cmask++;
        case 2: *cstr++ ^= *cmask++;
        case 1: *cstr++ ^= *cmask++;
    };
}

}}
