#include <panda/websocket/HTTPRequest.h>
#include <iostream>

namespace panda { namespace websocket {

static const int MAX_METHOD = 20;
static const int MAX_URI = 16*1024;

void HTTPRequest::_parse_header (StringRange range) {
    auto cur = std::begin(range);
    auto end = std::end(range);
    bool ok;

    // find method
    method.reserve(MAX_METHOD);
    char* mptr = method.buf();
    int mlen = 0;
    for (; cur != end; ++cur) {
        char c = *cur;
        if (c == ' ') {
            method.resize(mlen);
            break;
        }
        if (++mlen > MAX_METHOD) break;
        *mptr++ = c;
    }
    if (!method) { error = "bad request method"; return; }
    ++cur;

    // find URI
    ok = true;
    char  uristr[MAX_URI+1];
    char* uriptr = uristr;
    for (; cur != end; ++cur) {
        char c = *cur;
        if (c == ' ') break;
        if (c == '\r' || c == '\n'|| (uriptr-uristr) == MAX_URI) { ok = false; break; }
        *uriptr++ = c;
    }
    if (!ok || *cur++ != ' ')  {error = "couldn't find uri"; return; }
    uri = new URI(string(uristr, uriptr-uristr));

    // skip till next line
    for (; cur != end && *cur != '\n'; ++cur) {}
    if (*cur++ != '\n')  {error = "cannot find end of request line"; return; }

    HTTPPacket::_parse_header(StringRange{cur, end});
}

void HTTPRequest::clear () {
    method.clear();
    uri = NULL;
    HTTPPacket::clear();
}

}}
