#pragma once
#include <panda/string.h>
#include <panda/iterator.h>

namespace panda { namespace websocket {

using panda::string;
using panda::IteratorPair;

class StringPairIterator : public std::iterator<std::random_access_iterator_tag, string> {
    string    s1;
    string    s2;
    ptrdiff_t i;

public:
    StringPairIterator ()                               : i(2) {}
    StringPairIterator (string str1, string str2)       : s1(str1), s2(str2), i(0) {}
    StringPairIterator (const StringPairIterator& oth)  : s1(oth.s1), s2(oth.s2), i(oth.i) {}

    StringPairIterator& operator= (const StringPairIterator& oth) {
        s1 = oth.s1;
        s2 = oth.s2;
        i  = oth.i;
        return *this;
    }

    bool operator== (const StringPairIterator& rhs) const { return i == rhs.i; }
    bool operator!= (const StringPairIterator& rhs) const { return i != rhs.i; }

    string& operator*  () { return i == 0 ? s1 : s2; }
    string& operator-> () { return i == 0 ? s1 : s2; }

    StringPairIterator& operator++ ()    { ++i; return *this; }
    StringPairIterator  operator++ (int) { StringPairIterator tmp(*this); operator++(); return tmp; }
    StringPairIterator& operator-- ()    { --i; return *this; }
    StringPairIterator  operator-- (int) { StringPairIterator tmp(*this); operator--(); return tmp; }

    StringPairIterator operator+ (ptrdiff_t n) const { auto ret = *this; ret.i += n; return ret; }
    StringPairIterator operator- (ptrdiff_t n) const { auto ret = *this; ret.i -= n; return ret; }

    ptrdiff_t operator- (const StringPairIterator& rhs) const { return i - rhs.i; }

    bool operator<  (const StringPairIterator& rhs) const { return i < rhs.i; }
    bool operator>  (const StringPairIterator& rhs) const { return i > rhs.i; }
    bool operator<= (const StringPairIterator& rhs) const { return i <= rhs.i; }
    bool operator>= (const StringPairIterator& rhs) const { return i >= rhs.i; }

    StringPairIterator& operator+= (ptrdiff_t n) { i += n; return *this; }
    StringPairIterator& operator-= (ptrdiff_t n) { i -= n; return *this; }

    string& operator[] (size_t idx) { size_t n = idx + i; return n == 0 ? s1 : s2; }

    void swap (StringPairIterator& rhs) { std::swap(i, rhs.i); }

};

inline StringPairIterator operator+ (ptrdiff_t n, const StringPairIterator& rhs) { auto ret = rhs; ret += n; return ret; }

typedef IteratorPair<StringPairIterator> StringPairIteratorPair;

inline StringPairIteratorPair make_iterator_pair (string str1, string str2) {
    return StringPairIteratorPair(StringPairIterator(str1, str2), StringPairIterator());
}

template <class It>
class StringChainIterator : public std::iterator<std::random_access_iterator_tag, string> {
    string    s;
    It        it;
    ptrdiff_t i;

public:
    StringChainIterator (It last, ptrdiff_t n)           : it(last), i(n+1) {}
    StringChainIterator (string str, It first)           : s(str), it(first), i(0) {}
    StringChainIterator (const StringChainIterator& oth) : s(oth.s), it(oth.it), i(oth.i) {}

    StringChainIterator& operator= (const StringChainIterator& oth) {
        s  = oth.s;
        it = oth.it;
        i  = oth.i;
        return *this;
    }

    bool operator== (const StringChainIterator& rhs) const { return i == rhs.i; }
    bool operator!= (const StringChainIterator& rhs) const { return i != rhs.i; }

    string& operator*  () { return i == 0 ? s : it[i-1]; }
    string& operator-> () { return i == 0 ? s : it[i-1]; }

    StringChainIterator& operator++ ()    { ++i; return *this; }
    StringChainIterator  operator++ (int) { StringChainIterator tmp(*this); operator++(); return tmp; }
    StringChainIterator& operator-- ()    { --i; return *this; }
    StringChainIterator  operator-- (int) { StringChainIterator tmp(*this); operator--(); return tmp; }

    StringChainIterator operator+ (ptrdiff_t n) const { auto ret = *this; ret.i += n; return ret; }
    StringChainIterator operator- (ptrdiff_t n) const { auto ret = *this; ret.i -= n; return ret; }

    ptrdiff_t operator- (const StringChainIterator& rhs) const { return i - rhs.i; }

    bool operator<  (const StringChainIterator& rhs) const { return i < rhs.i; }
    bool operator>  (const StringChainIterator& rhs) const { return i > rhs.i; }
    bool operator<= (const StringChainIterator& rhs) const { return i <= rhs.i; }
    bool operator>= (const StringChainIterator& rhs) const { return i >= rhs.i; }

    StringChainIterator& operator+= (ptrdiff_t n) { i += n; return *this; }
    StringChainIterator& operator-= (ptrdiff_t n) { i -= n; return *this; }

    string& operator[] (size_t idx) { size_t n = idx + i; return n == 0 ? s : it[n-1]; }

    void swap (StringChainIterator& rhs) { std::swap(i, rhs.i); }
};

template <class It>
inline StringChainIterator<It> operator+ (ptrdiff_t n, const StringChainIterator<It>& rhs) { auto ret = rhs; ret += n; return ret; }

template <class It>
using StringChainIteratorPair = IteratorPair<StringChainIterator<It>>;

template <class It>
inline StringChainIteratorPair<It> make_iterator_pair (string str, It first, It last) {
    return StringChainIteratorPair<It>(StringChainIterator<It>(str, first), StringChainIterator<It>(last, last - first));
}


}}
