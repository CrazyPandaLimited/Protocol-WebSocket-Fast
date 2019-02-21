#include <panda/protocol/websocket/HTTPPacket.h>
#include <iostream>
#include <cassert>

namespace panda { namespace protocol { namespace websocket {

using std::cout;
using std::endl;

static const int MAX_HEADER_NAME  = 256;
static const int MAX_HEADER_VALUE = 8*1024;


bool HTTPPacket::parse (string& buf) {
    if (_parsed) clear();

    if (_header_ok) { // waiting for body
        assert(_content_length);
        assert(_buf_size < _content_length);
        size_t left = _content_length - _buf_size;
        if (buf.length() < left) {
            _body->parts.push_back(buf);
            _buf_size += buf.length();
            return false;
        }
        if (buf.length() == left) {
            _body->parts.push_back(buf);
            buf.clear();
        } else {
            _body->parts.push_back(buf.substr(0, left));
            buf.offset(left);
        }
        _parsed = true;
        return true;
    }

    auto end = _header_finder.find(buf);
    if (end == buf.cbegin()) return !add_header_chunk(buf);

    string last_chunk;
    if (end != buf.cend()) { // have data in buffer after headers
        size_t pos = end - buf.cbegin();
        last_chunk = buf.substr(0, pos);
        buf.offset(pos);
    } else {
        last_chunk = buf;
        buf.clear();
    }

    if (!add_header_chunk(last_chunk)) return true;
    _parse_header(panda::ranges::joiner(_body->parts.cbegin(), _body->parts.cend()));
    _body->parts.clear();
    _buf_size = 0;

    if (_header_ok && _content_length) {
        if (max_body_size) {
            if (max_body_size == (size_t)-1) {
                error = "http body is disallowed for this request";
                _parsed = true;
                return true;
            }
            else if (_content_length > max_body_size) {
                error = "max_body_size exceeded";
                _parsed = true;
                return true;
            }
        }
        return parse(buf);
    }

    _parsed = true;
    return true;
}

void HTTPPacket::_parse_header (StringRange range) {
    auto cur = std::begin(range);
    auto end = std::end(range);
    bool ok;

    // parse headers
    ok = false;
    char keyacc[MAX_HEADER_NAME];
    char valacc[MAX_HEADER_VALUE];
    enum { PARSE_MODE_KEY, PARSE_MODE_VAL } mode = PARSE_MODE_KEY;
    size_t keylen = 0;
    char* curacc = keyacc;

    for (; cur != end; ++cur) {
        char c = *cur;
        if (mode == PARSE_MODE_KEY) {
            if (curacc - keyacc == MAX_HEADER_NAME) {error = "max header name length reached"; return; }
        }
        else {
            if (curacc - valacc == MAX_HEADER_VALUE) {error = "max header value length reached"; return; }
        }

        if (c == ':' && mode == PARSE_MODE_KEY) {
            mode = PARSE_MODE_VAL;
            keylen = curacc - keyacc;
            curacc = valacc;
        }
        else if (c == '\n') {
            if (mode == PARSE_MODE_KEY) {
                if (curacc == keyacc || (curacc == keyacc + 1 && *keyacc == '\r')) { ok = true; break; } // headers end (empty line)
                else {error = "header without value found"; return; };
            }

            string key;
            if (keylen) key.assign(keyacc, keylen);

            string value;
            const char* valptr = valacc;
            if (curacc != valptr && *valptr == ' ') valptr++;
            if (curacc != valptr && *(curacc-1) == '\r') curacc--;
            if (curacc != valptr) value.assign(valptr, curacc - valptr);

            headers.add_field(key, value);
            //std::cout << "found key='" << key << "' value='" << value << "'\n";

            mode = PARSE_MODE_KEY;
            curacc = keyacc;
        }
        else *curacc++ = c;
    }
    if (!ok)  {error = "headers did not end with empty line"; return; }

    _header_ok = true;

    auto cont_len_str = headers.get_field("Content-Length", "0");
    auto res = cont_len_str.to_number(_content_length);
    assert(!res.ec);
}

void HTTPPacket::clear () {
    error.clear();
    headers.clear();
    _content_length = _buf_size = 0;
    _body->parts.clear();
    _header_finder.reset();
    _header_ok = _parsed = false;
}

void HTTPPacket::_to_string (string& str) {
    size_t blen = _body->length();
    headers.remove_field("Content-Length");

    size_t hlen = 0;
    for (const auto& elem : headers.fields) {
        hlen += elem.name.length() + elem.value.length() + 4;
    }

    str.reserve(str.length() + hlen + (blen ? (blen+30) : 0) + 10); // +30 for content-length: xxxxxxx

    for (const auto& elem : headers.fields) {
        str += elem.name;
        str += ": ";
        str += elem.value;
        str += "\r\n";
    }
    if (blen) {
        str += "Content-Length: ";
        str += string::from_number(blen);
        str += "\r\n";
    }
    str += "\r\n";

    if (blen) for (const auto& s : _body->parts) str += s;
}

}}}
