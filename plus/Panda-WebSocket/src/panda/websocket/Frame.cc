#include <panda/websocket/Frame.h>
#include <cassert>
#include <iostream>
#include <panda/lib/endian.h>
#include <panda/websocket/utils.h>

namespace panda { namespace websocket {

using std::cout;
using std::endl;

union _check_endianess { unsigned x; unsigned char c; };
static const bool am_i_little = (_check_endianess{1}).c;
static const int MAX_CONTROL_FRAME_LEN = 125;
static const int MAX_HEADER_SIZE = 14; // 2 bytes required + 8-byte length + 4-byte mask

static inline uint32_t rotate_shift (uint32_t x, unsigned shift) {
    return am_i_little ? ((x >> shift) | (x << (sizeof(x)*8 - shift))) :
                         ((x << shift) | (x >> (sizeof(x)*8 - shift)));
}

static inline void crypt_mask (char* str, size_t len, uint32_t mask, uint64_t bytes_received) {
    mask = rotate_shift(mask, (bytes_received & 3)*8);
    const uint64_t mask64 = ((uint64_t)mask << 32) | mask;
    auto str64 = (uint64_t*)str;
    auto end64 = str64 + (len / 8);

    while (str64 != end64) *str64++ ^= mask64;

    auto cstr  = (unsigned char*)str64;
    auto cmask = (const unsigned char*)&mask64;
    switch (len & 7) {
        case 7: *cstr++ ^= *cmask++;
        case 6: *cstr++ ^= *cmask++;
        case 5: *cstr++ ^= *cmask++;
        case 4: *cstr++ ^= *cmask++;
        case 3: *cstr++ ^= *cmask++;
        case 2: *cstr++ ^= *cmask++;
        case 1: *cstr++ ^= *cmask++;
    };
}

bool Frame::parse (string& buf) {
    assert(_state != DONE);

    auto data = buf.data();
    auto end  = data + buf.length();

    if (_state == FIRST) {
        if (data == end) return false;
        auto first = *((BinaryFirst*)data++);
        _header.fin    = first.fin;
        _header.rsv1   = first.rsv1;
        _header.rsv2   = first.rsv2;
        _header.rsv3   = first.rsv3;
        _header.opcode = (Opcode)first.opcode;
        _state = SECOND;
        //cout << "Frame[parse]: first: FIN=" << final() << ", RSV1=" << _header.rsv1 << ", RSV2=" << _header.rsv2 << ", RSV3=" << _header.rsv3 << ", OPCODE=" << opcode() << endl;
        if (opcode() > PONG) {
            //cout << "Frame[parse]: invalid opcode=" << opcode() << endl;
            error = "invalid opcode received";
            _state = DONE;
        }
    }

    if (_state == SECOND) {
        if (data == end) return false;
        auto second = *((BinarySecond*)data++);
        _header.has_mask = second.mask;
        _slen            = second.slen;
        //cout << "Frame[parse]: second: HASMASK=" << _header.has_mask << ", SLEN=" << (int)_slen << endl;
        if (is_control()) {
            if (!final()) {
                error = "control frame can't be fragmented";
                _state = DONE;
            }
            else if (_slen > MAX_CONTROL_FRAME_LEN) {
                error = "control frame payload is too big";
                _state = DONE;
            }
        }
        _state = LENGTH;
        if (!_header.has_mask && _mask_required && _slen) {
            error = "frame is not masked";
            _state = DONE;
        }
    }

    if (_state == LENGTH) {
        if (_slen < 126) {
            _length = _slen;
            //cout << "Frame[parse]: LENGTH(in)=" << _length << endl;
            _state = MASK;
        }
        else if (data == end) return false;
        else if (_slen == 126) {
            if (!parse_binary_number(_len16, data, end - data)) return false;
            _length = panda::lib::be2h16(_len16);
            //cout << "Frame[parse]: LENGTH(ex16)=" << _length << endl;
            _state = MASK;
        }
        else { // 127
            if (!parse_binary_number(_length, data, end - data)) return false;
            _length = panda::lib::be2h64(_length);
            //cout << "Frame[parse]: LENGTH(ex64)=" << _length << endl;
            _state = MASK;
        }
        _payload_bytes_left = _length;
    }

    if (_state == MASK) {
        if (!_header.has_mask) _state = _length ? PAYLOAD : DONE;
        else if (data == end) return false;
        else {
            if (!parse_binary_number(_header.mask, data, end - data)) return false;
            //cout << "Frame[parse]: MASK=" << _header.mask << endl;
            _state = _length ? PAYLOAD : DONE;
        }

        if (_max_size && _length > _max_size) {
            error = "max frame size exceeded";
            _state = DONE;
        }
    }

    if (_state == PAYLOAD) {
        if (data == end) return false;
        auto shift  = data - buf.data();
        auto buflen = end - data;
        //cout << "Frame[parse]: have buflen=" << buflen << ", payloadleft=" << _payload_bytes_left << endl;
        if (buflen >= _payload_bytes_left) { // last needed buffer
            if (_header.has_mask) crypt_mask((char*)data, _payload_bytes_left, _header.mask, _length - _payload_bytes_left); // TODO: remove (char*)cast when unique_string is done
            data += _payload_bytes_left;
            if (data == end && !shift) payload.push_back(buf); // payload is the whole buf
            else payload.push_back(buf.substr(shift, _payload_bytes_left));
            _state = DONE;
            //cout << "Frame[parse]: PAYLOAD=";
            //for (const auto& s : payload) cout << s; cout << endl;
        }
        else { // not last buffer
            if (_header.has_mask) crypt_mask((char*)data, buflen, _header.mask, _length - _payload_bytes_left); // TODO: remove (char*)cast when unique_string is done
            _payload_bytes_left -= buflen;
            if (!shift) payload.push_back(buf);        // payload is the whole buf
            else payload.push_back(buf.substr(shift)); // payload is not in the beginning - first packet
            return false;
        }
    }

    if (data == end) buf.clear(); // no extra data after the end of frame
    else buf = buf.substr(data - buf.data());

    if (opcode() == CLOSE) {
        if (error || !_length) _close_code = CLOSE_UNKNOWN;
        else if (_length < sizeof(_close_code)) {
            _close_code = CLOSE_UNKNOWN;
            error = "control frame CLOSE contains invalid data";
        } else {
            char tmp[MAX_CONTROL_FRAME_LEN];
            char* ptr = tmp;
            for (const auto& s : payload) {
                memcpy(ptr, s.data(), s.length());
                ptr += s.length();
            }
            _close_code = panda::lib::be2h16(*((uint16_t*)tmp));
            _close_message.assign(tmp + sizeof(_close_code), _length - sizeof(_close_code), string::COPY);
            payload.clear();
            payload.push_back(_close_message);
            _length -= sizeof(uint16_t);
        }
        //cout << "Frame[parse]: CLOSE CODE=" << _close_code << " MSG=" << _close_message << endl;
    }

    return true;
}

void Frame::compile (Header header, std::deque<string>& payload) {
    size_t plen = 0;
    if (payload.size()) {
        if (header.has_mask) {
            for (auto& str : payload) {
                auto slen = str.length();
                crypt_mask((char*)str.data(), slen, header.mask, plen); // TODO: remove (char*)cast when unique_string is done
                plen += slen;
            }
        }
        else
            for (auto& str : payload) plen += str.length();
    }

    string header_str(MAX_HEADER_SIZE);
    char* hptr = header_str.buf();
    *((BinaryFirst*)hptr++) = BinaryFirst{header.opcode, header.rsv3, header.rsv2, header.rsv1, header.fin};

    if (plen < 126) {
        *((BinarySecond*)hptr++) = BinarySecond{(uint8_t)plen, header.has_mask};
    } else if (plen < 65536) {
        *((BinarySecond*)hptr++) = BinarySecond{126, header.has_mask};
        *((uint16_t*)hptr) = panda::lib::h2be16(plen);
        hptr += sizeof(uint16_t);
    } else {
        *((BinarySecond*)hptr++) = BinarySecond{127, header.has_mask};
        *((uint64_t*)hptr) = panda::lib::h2be64(plen);
        hptr += sizeof(uint64_t);
    }

    if (header.has_mask) {
        *((uint32_t*)hptr) = header.mask;
        hptr += sizeof(uint32_t);
    }

    header_str.resize(hptr - header_str.data());

    payload.push_front(header_str);
}

string Frame::compile_close_payload (uint16_t code, const string& message) {
    size_t sz = sizeof(code) + message.length();
    string ret(sz);
    char* buf = ret.buf();
    *((uint16_t*)buf) = panda::lib::h2be16(code);
    buf += sizeof(code);
    if (message.length()) std::memcpy(buf, message.data(), message.length());
    ret.resize(sz);
    return ret;
}

void Frame::reset () {
    error.clear();
    payload.clear();
    _state       = FIRST;
    _len16       = 0;
    _length      = 0;
    _header.mask = 0;
}

}}
