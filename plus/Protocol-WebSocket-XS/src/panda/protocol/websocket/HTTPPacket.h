#pragma once
#include <vector>
#include <unordered_map>
#include <panda/refcnt.h>
#include <panda/string.h>
#include <panda/ranges/Joiner.h>
#include <panda/ranges/KmpFinder.h>
#include <panda/unordered_string_map.h>
#include <panda/protocol/websocket/utils.h>
#include <panda/protocol/http/Body.h>
#include <panda/protocol/http/HeaderValueParamsParser.h>

namespace panda { namespace protocol { namespace websocket {

using panda::string;

static auto fndr = panda::ranges::make_kmp_finder("\r\n\r\n");

class HTTPPacket : public virtual panda::Refcnt {
public:
    typedef panda::unordered_string_multimap<string, string, string_hash_ci, string_equal_ci> Headers;

    using HeaderValue = http::HeaderValue;
    using HeaderValues = http::HeaderValues;

    using BodySP = http::BodySP;

    size_t  max_headers_size;
    size_t  max_body_size;
    string  error;
    Headers headers;

    BodySP body           () const { return _body; }
    bool   header_ok      () const { return _header_ok; }
    bool   parsed         () const { return _parsed; }
    size_t content_length () const { return _content_length; }

    virtual bool parse (string& buf);

    void replace_header (const string& name, const string& value) {
        auto it = headers.find(name);
        if (it != headers.end()) it->second = value;
        else headers.emplace(name, value);
    }

    virtual void clear ();

    string to_string () {
        string ret;
        _to_string(ret);
        return ret;
    }

    virtual ~HTTPPacket () {}

protected:
    typedef decltype(panda::ranges::joiner(BodySP()->parts.cbegin(), BodySP()->parts.cend())) StringRange;

    HTTPPacket () :
        max_headers_size(0), max_body_size(0), headers(Headers(16)), _body(new http::Body), _header_finder(fndr), _header_ok(false), _parsed(false),
        _buf_size(0), _content_length(0) {
    }

    virtual void _parse_header (StringRange range);

    virtual void _to_string (string& str);

private:
    BodySP         _body;
    decltype(fndr) _header_finder;
    bool           _header_ok;
    bool           _parsed;
    size_t         _buf_size;
    size_t         _content_length;

    bool add_header_chunk (string& buf) {
        if (max_headers_size && _buf_size + buf.length() > max_headers_size) {
            error = "max_headers_size exceeded";
            _parsed = true;
            return false;
        }
        _body->parts.push_back(buf);
        _buf_size += buf.length();
        return true;
    }
};

using HTTPPacketSP = panda::iptr<HTTPPacket>;

}}}
