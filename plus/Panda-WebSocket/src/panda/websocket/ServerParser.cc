#include <panda/websocket/ServerParser.h>
#include <panda/ranges/Joiner.h>
#include <panda/encode/base64.h>
#include <cctype>
#include <iostream>
#include <exception>

namespace panda { namespace websocket {

using std::cout;
using std::endl;

static const int MAX_URI          = 16*1024;
static const int MAX_HEADER_NAME  = 256;
static const int MAX_HEADER_VALUE = 8*1024;

ServerParser::ServerParser () : Parser(), accept_finder_next_buf(0), accept_finder(fndr) {
}

shared_ptr<AcceptRequest> ServerParser::accept () {
    if (established) throw std::logic_error("can't accept - already established");

    AcceptRequest* req = NULL;
    for (; accept_finder_next_buf < bufs.size(); ++accept_finder_next_buf) {
        const string& s = bufs[accept_finder_next_buf];
        auto end = accept_finder.find(s);
        if (end == s.cbegin()) continue;

        if (end != s.cend()) { // have data in buffer after http packet
            string rest = s.substr(end - s.cbegin());
            bufs[accept_finder_next_buf].resize(end - s.cbegin());
            req = parse_accept(bufs.cbegin(), bufs.cbegin() + accept_finder_next_buf + 1);
            bufs.erase(bufs.cbegin(), bufs.cbegin() + accept_finder_next_buf);
            bufs[0] = rest;
        } else {
            req = parse_accept(bufs.cbegin(), bufs.cbegin() + accept_finder_next_buf + 1);
            bufs.erase(bufs.cbegin(), bufs.cbegin() + accept_finder_next_buf + 1);
        }
        break;
    }

//    cout << "rest in buf after accept: '";
//    for (auto& s : bufs) {
//        cout << s;
//    }
//    cout << "'" << endl;

    if (req && !req->error) {
        //cout << "established!!";
        established = true;
    }

    return req;
}

AcceptRequest* ServerParser::parse_accept (Bufs::const_iterator buf_begin, Bufs::const_iterator buf_end) {
    //cout << "ServerParser[parse_http]\n";
    auto range = panda::ranges::joiner(buf_begin, buf_end);
    auto cur   = std::begin(range);
    auto end   = std::end(range);
    auto req   = new AcceptRequest();

    bool ok = true;

    // check "GET "
    const char  _lead[] = "GET ";
    const char* lead = _lead;
    for (int i = 0; i < sizeof(_lead)-1; ++i) {
        if (cur == end || *cur++ != *lead++) { ok = false; break; }
    }
    if (!ok) {req->error = "http error: only GET requests are supported"; return req; }
    //cout << "GET ok\n";

    // find URI
    char  uristr[MAX_URI+1];
    char* uriptr = uristr;
    for (; cur != end; ++cur) {
        char c = *cur;
        if (c == '\r' || c == '\n'|| (uriptr-uristr) == MAX_URI) { ok = false; break; }
        if (c == ' ') break;
        *uriptr++ = c;
    }
    if (!ok || *cur++ != ' ')  {req->error = "http error: couldn't find uri"; return req; }
    uriptr = 0;
    req->uri = new URI(string(uristr, uriptr-uristr));
    //cout << "URI ok\n";

    // skip till next line
    for (; cur != end && *cur != '\n'; ++cur) {}
    if (*cur++ != '\n')  {req->error = "http error: cannot find end of request line"; return req; }

    // parse headers
    ok = false;
    HTTPHeaders& headers = req->headers;
    char keyacc[MAX_HEADER_NAME];
    char valacc[MAX_HEADER_VALUE];
    enum { PARSE_MODE_KEY, PARSE_MODE_VAL } mode = PARSE_MODE_KEY;
    size_t keylen;
    char* curacc = keyacc;

    for (; cur != end; ++cur) {
        char c = *cur;
        if (mode == PARSE_MODE_KEY) {
            if (curacc - keyacc == MAX_HEADER_NAME) {req->error = "http error: max header name length reached"; return req; }
        }
        else {
            if (curacc - valacc == MAX_HEADER_VALUE) {req->error = "http error: max header value length reached"; return req; }
        }

        if (c == ':' && mode == 0) {
            mode = PARSE_MODE_VAL;
            keylen = curacc - keyacc;
            curacc = valacc;
        }
        else if (c == '\n') {
            if (mode == PARSE_MODE_KEY) {
                if (curacc == keyacc || (curacc == keyacc + 1 && *keyacc == '\r')) { ok = true; break; } // headers end (empty line)
                else {req->error = "http error: header without value found"; return req; };
            }

            string key;
            if (keylen) key.assign(keyacc, keylen, string::COPY);

            string value;
            const char* valptr = valacc;
            if (curacc != valptr && *valptr == ' ') valptr++;
            if (curacc != valptr && *(curacc-1) == '\r') curacc--;
            if (curacc != valptr) value.assign(valptr, curacc - valptr, string::COPY);

            headers.insert(std::pair<string,string>(key, value));
            //cout << "found key='" << key << "' value='" << value << "'\n";

            mode = PARSE_MODE_KEY;
            curacc = keyacc;
        }
        else *curacc++ = c;
    }
    if (!ok)  {req->error = "http error: headers did not end with empty line"; return req; }

    auto it = headers.find("Connection");
    if (it == headers.end() || it->second.find("Upgrade", 0, 7) == string::npos) {
        req->error = "http error: Connection must be 'Upgrade'";
        return req;
    }

    it = headers.find("Upgrade");
    if (it == headers.end() || it->second.find("websocket", 0, 9) == string::npos) {
        req->error = "http error: Upgrade must be 'websocket'";
        return req;
    }

    ok = false;
    it = headers.find("Sec-WebSocket-Key");
    if (it != headers.end()) {
        const string& b64key = it->second;
        key = panda::encode::decode_base64(it->second);
        if (key.length() == 16) ok = true;
    }
    if (!ok) {req->error = "http error: Sec-WebSocket-Key missing or invalid"; return req; }

    return req;
}

void ServerParser::reset () {
    accept_finder_next_buf = 0;
    accept_finder.reset();
    Parser::reset();
}

ServerParser::~ServerParser () {
    cout << "~ServerParser\n";
}

}}
