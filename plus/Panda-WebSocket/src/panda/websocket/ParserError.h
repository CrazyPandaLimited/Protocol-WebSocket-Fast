#pragma once
#include <exception>
#include <panda/string.h>

namespace panda { namespace websocket {

class ParserError : public std::exception {
public:
    ParserError (string what) : _what(what) {}
    ParserError (const char* what) : _what(string(what, string::COPY)) {}

    virtual const char* what () const noexcept { return _what.c_str(); }
private:
    string _what;
};

}}
