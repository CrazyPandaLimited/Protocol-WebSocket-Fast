#pragma once
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

}}
