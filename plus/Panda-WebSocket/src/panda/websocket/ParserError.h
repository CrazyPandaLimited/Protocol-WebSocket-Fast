#pragma once
#include <exception>
#include <panda/string.h>

namespace panda { namespace websocket {

class ParserError : public std::exception {
public:
    ParserError (const string& what) : _what(what) {}
    ParserError (const char* what)   : _what(string(what)) {}

    virtual const char* what () const noexcept {
        if (_what.shared_capacity() <= _what.length() || _what[_what.length()] != 0) {
            _what.reserve(_what.length() + 1);
            _what[_what.length] = 0;
        }
        return _what.data();
    }
private:
    string _what;
};

}}
