#include <panda/protocol/websocket/HTTPRequest.h>
#include <iostream>

namespace panda { namespace protocol { namespace websocket {

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
            method.length(mlen);
            break;
        }
        if (++mlen > MAX_METHOD || c == '\r' || c == '\n') break;
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
        if (c == '\r' || c == '\n' || (uriptr-uristr) == MAX_URI) { ok = false; break; }
        *uriptr++ = c;
    }
    if (!ok || *cur++ != ' ' || uriptr == uristr)  {error = "couldn't find uri"; return; }
    uri = new URI(string(uristr, uriptr-uristr));

    // skip till next line
    for (; cur != end && *cur != '\n'; ++cur) {}
    if (*cur++ != '\n')  {error = "cannot find end of request line"; return; }

    HTTPPacket::_parse_header(StringRange{cur, end});
}

void HTTPRequest::_to_string (string& str) {
    if (!uri || !uri->host()) throw std::logic_error("HTTPRequest[to_string] uri with net location must be defined");
    string uristr = uri->relative();
    if (!uristr) uristr = '/';

    size_t rslen = (method ? method.length() : 3) + uristr.length() + 20;
    str.reserve(rslen);

    if (method) {
        str += method;
        str += ' ';
    }
    else str += "GET ";

    str += uristr;
    str += ' ';
    str += "HTTP/1.1\r\n";


    if (!headers.has_field("User-Agent")) headers.add_field("User-Agent", "Panda-WebSocket");
    if (!headers.has_field("Host"))       headers.add_field("Host", uri->host());

    HTTPPacket::_to_string(str);
}

void HTTPRequest::clear () {
    method.clear();
    uri = NULL;
    HTTPPacket::clear();
}

}}}
