#include <panda/websocket/HTTPResponse.h>
#include <cstdlib>
#include <panda/lib/lib.h>

namespace panda { namespace websocket {

static const int MAX_CODE    = 4;
static const int MAX_MESSAGE = 512;

void HTTPResponse::_parse_header (StringRange range) {
    auto cur = std::begin(range);
    auto end = std::end(range);
    bool ok;

    // skip until space (http version)
    int httpv_len = 0;
    for (; cur != end; ++cur) {
        char c = *cur;
        if (c == ' ') break;
        if (c == '\r' || c == '\n') { httpv_len = 0; break; }
        ++httpv_len;
    }
    if (!httpv_len) { error = "bad http version token"; return; }
    ++cur;

    // find status code
    char  cstr[MAX_CODE+1];
    char* cptr = cstr;
    char* cend = cptr + MAX_CODE;
    for (; cur != end; ++cur) {
        char c = *cur;
        if (c == ' ') break;
        if (c == '\r' || c == '\n' || cptr == cend) { cptr = cstr; break; }
        *cptr++ = c;
    }
    if (cptr == cstr || *cur++ != ' ')  {error = "couldn't find valid status code"; return; }
    *cptr = 0;
    code = std::atoi(cstr);

    // find status message
    char  mstr[MAX_MESSAGE];
    char* mptr = mstr;
    char* mend = mptr + MAX_MESSAGE;
    for (; cur != end && *cur != '\n'; ++cur) {
        char c = *cur;
        if (c == '\r') continue;
        if (mptr == mend) { mptr = mstr; break; }
        *mptr++ = c;
    }
    if (*cur++ != '\n' || mptr == mstr)  {error = "cannot find valid status message"; return; }
    message.assign(mstr, mptr-mstr, string::COPY);

    HTTPPacket::_parse_header(StringRange{cur, end});
}

void HTTPResponse::_to_string (string& str) {
    size_t stlen = 30 + message.length();
    str.reserve(stlen);

    str += "HTTP/1.1 ";
    str += panda::lib::itoa(code);
    str += ' ';
    str += message;
    str += "\r\n";

    if (headers.find("Server") == headers.end()) headers.emplace("Server", "Panda-WebSocket");

    HTTPPacket::_to_string(str);
}

}}
