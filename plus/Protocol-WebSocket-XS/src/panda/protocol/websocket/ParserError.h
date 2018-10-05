#pragma once
#include <exception>
#include <panda/string.h>

namespace panda { namespace protocol { namespace websocket {

class ParserError : public std::exception {
public:
    ParserError () {}
    ParserError (const string& what) : _what(what) {}
    ParserError (const char* what)   : _what(string(what)) {}

    virtual const char* what () const noexcept {
        if (!_what) return "";
        if (_what.shared_capacity() <= _what.length() || _what[_what.length()] != 0) {
            _what.reserve(_what.length() + 1);
            _what[_what.length()] = 0;
        }
        return _what.data();
    }

    explicit
    operator bool () const { return _what.length() > 0; }

private:
    mutable string _what;
};

}}}
