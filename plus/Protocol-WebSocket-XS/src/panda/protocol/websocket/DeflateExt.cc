#include "DeflateExt.h"
#include <charconv>

namespace panda { namespace protocol { namespace websocket {

const char* DeflateExt::extension_name = "permessage-deflate";

panda::optional<panda::string> DeflateExt::bootstrap() {
    using result_t = panda::optional<panda::string>;
    panda::string compiled_verison{ZLIB_VERSION};
    panda::string loaded_version{zlibVersion()};

    if (compiled_verison != loaded_version) {
        panda::string err = "zlib version mismatch, loaded: " + loaded_version + ", compiled" + compiled_verison;
        return result_t{err};
    }
    return  result_t{}; // all OK
}

void DeflateExt::request(HTTPPacket::HeaderValues& ws_extensions, const Config& cfg) {
    HTTPPacket::HeaderValueParams params;
    params.emplace("server_max_window_bits", panda::to_string(cfg.server_max_window_bits));
    params.emplace("client_max_window_bits", panda::to_string(cfg.client_max_window_bits));
    if(cfg.server_no_context_takeover) params.emplace("server_no_context_takeover", "");
    if(cfg.client_no_context_takeover) params.emplace("client_no_context_takeover", "");

    string name{extension_name};
    HTTPPacket::HeaderValue hv {name, std::move(params)};
    ws_extensions.emplace_back(std::move(hv));
}


static bool get_window_bits(const string& value, std::int8_t& bits) {
    auto res = std::from_chars(value.data(), value.data() + value.size(), bits, 10);
    return !res.ec && (bits >= 8) && (bits <= 15);
}

const HTTPPacket::HeaderValue* DeflateExt::select(const HTTPPacket::HeaderValues& values, const Config& cfg, Role role) {
    for(auto& header: values) {
        if (header.name == extension_name) {
            bool params_correct = true;
            for(auto& it: header.params) {
                auto& param_name = it.first;
                auto& param_value = it.second;
                if (param_name == "server_no_context_takeover") params_correct = params_correct && param_value == "";
                else if (param_name == "client_no_context_takeover") params_correct = params_correct && param_value == "";
                else if (param_name == "server_max_window_bits") {
                    std::int8_t bits;
                    params_correct = params_correct && get_window_bits(param_value, bits);
                    if(params_correct && role == Role::CLIENT) {
                         params_correct = bits == cfg.server_max_window_bits;
                         // ignore for, use it later for compression client messages
                    }
                }
                else if (param_name == "client_max_window_bits") {
                    std::int8_t bits;
                    // value is optional
                    if (param_value) {
                        params_correct = params_correct && get_window_bits(param_value, bits);
                        if(params_correct && role == Role::CLIENT) {
                             params_correct = bits == cfg.client_max_window_bits;
                             // ignore for, use it later for decompression client messages
                        }
                    } else if(params_correct && role == Role::CLIENT) {
                        params_correct = cfg.client_max_window_bits == 15;
                    }
                } else { params_correct = false; }  // unknown parameter
            }
            if (params_correct) return &header;
        }
    }
    return nullptr;
}

DeflateExt* DeflateExt::uplift(const HTTPPacket::HeaderValue& deflate_extension, HTTPPacket::HeaderValues& extensions, Role role) {
    Config cfg;
    for(auto& it: deflate_extension.params) {
        auto& param_name = it.first;
        auto& param_value = it.second;
        if (param_name == "server_no_context_takeover") cfg.server_no_context_takeover = true;
        if (param_name == "client_no_context_takeover") cfg.client_no_context_takeover = true;
        if (param_name == "server_max_window_bits") cfg.server_max_window_bits = static_cast<std::uint8_t>(panda::stoi(param_value));
        if (param_name == "client_max_window_bits") cfg.client_max_window_bits = param_value ? static_cast<std::uint8_t>(panda::stoi(param_value)) : 15;
    }
    // echo back best match
    extensions.push_back(deflate_extension);
    return new DeflateExt(cfg, role);
}


DeflateExt::DeflateExt(const DeflateExt::Config& cfg, Role role) {
    auto rx_window = role == Role::CLIENT ? cfg.server_max_window_bits : cfg.client_max_window_bits;
    auto tx_window = role == Role::CLIENT ? cfg.client_max_window_bits : cfg.server_max_window_bits;

    //rx_stream.next_in = reinterpret_cast<Bytef*>(rx_buff.buf());
    rx_stream.avail_in = 0;
    rx_stream.zalloc = Z_NULL;
    rx_stream.zfree = Z_NULL;
    rx_stream.opaque = Z_NULL;

    auto r = inflateInit2(&rx_stream, rx_window);
    if (r != Z_OK) {
        panda::string err = panda::string("zlib::inflateInit2 error ") + rx_stream.msg;
        throw std::runtime_error(err);
    }

    //tx_stream.next_in = reinterpret_cast<Bytef*>(tx_buff.buf());
    tx_stream.avail_in = 0;
    tx_stream.zalloc = Z_NULL;
    tx_stream.zfree = Z_NULL;
    tx_stream.opaque = Z_NULL;

    r = deflateInit2(&tx_stream, cfg.compression_level, Z_DEFLATED, tx_window, cfg.mem_level, cfg.strategy);
    if (r != Z_OK) {
        panda::string err = panda::string("zlib::deflateInit2 error ") +rx_stream.msg;
        throw std::runtime_error(err);
    }

    reset_after_tx =
               (role == Role::CLIENT && cfg.client_no_context_takeover)
            || (role == Role::SERVER && cfg.server_no_context_takeover);
    reset_after_rx =
               (role == Role::CLIENT && cfg.server_no_context_takeover)
            || (role == Role::SERVER && cfg.client_no_context_takeover);
}

void DeflateExt::reset_tx() {
    if (deflateReset(&tx_stream) != Z_OK) {
        panda::string err = panda::string("zlib::deflateEnd error ");
        if (tx_stream.msg) {
            err += tx_stream.msg;
        }
        throw std::runtime_error(err);
    }
}

void DeflateExt::reset_rx() {
    if (inflateReset(&rx_stream) != Z_OK) {
        panda::string err = panda::string("zlib::inflateEnd error ");
        if(rx_stream.msg) {
            err += rx_stream.msg;
        }
    }
}


DeflateExt::~DeflateExt(){
    if (deflateEnd(&tx_stream) != Z_OK) {
        panda::string err = panda::string("zlib::deflateEnd error ");
        if (tx_stream.msg) {
            err += tx_stream.msg;
        }
        assert(err.c_str());
    }
    if (inflateEnd(&rx_stream) != Z_OK) {
        panda::string err = panda::string("zlib::inflateEnd error ");
        if(rx_stream.msg) {
            err += rx_stream.msg;
        }
        assert(err.c_str());
    }
}


string& DeflateExt::compress(string& str, bool final) {
    auto sz = str.capacity();
    string in_copy = str;
    tx_stream.next_in = reinterpret_cast<Bytef*>(in_copy.buf());
    tx_stream.avail_in = static_cast<uInt>(in_copy.length());
    tx_stream.next_out = reinterpret_cast<Bytef*>(str.shared_buf());
    tx_stream.avail_out = static_cast<uInt>(sz);
    auto flush = Z_SYNC_FLUSH;
    do {
        if (!tx_stream.avail_out) {
            sz += 6;
            tx_stream.avail_out += 6;
            str.reserve(sz);
            str.length(sz);
        }
        auto r = deflate(&tx_stream, flush);
        if(r < 0 && r != Z_BUF_ERROR) {
            panda::string err = panda::string("zlib::deflate error ");
            if (tx_stream.msg) err += tx_stream.msg;
            throw std::runtime_error(err);
        }
    } while(!tx_stream.avail_out);

    sz -= tx_stream.avail_out;
    // remove tail empty-frame 0x00 0x00 0xff 0xff for final messages only
    assert(sz);
    if (final) sz -= 4;
    str.length(sz);

    if(final && reset_after_tx) reset_tx();
    return str;
}

bool DeflateExt::uncompress(Frame& frame) {
    using It = decltype(frame.payload)::iterator;
    if(frame.error) return false;
    string acc;

    It it_in = frame.payload.begin();
    It end = frame.payload.end();
    size_t sz = frame.payload_length();
    acc.reserve(sz);

    bool final = frame.final();
    rx_stream.next_out = reinterpret_cast<Bytef*>(acc.buf());
    rx_stream.avail_out = static_cast<uInt>(acc.capacity());
    do {
        string& chunk_in = *it_in;
        It it_next = ++it_in;
        if (it_next == end && final) {
            // append empty-frame 0x00 0x00 0xff 0xff
            unsigned char trailer[4] = { 0x00,  0x00, 0xFF, 0xFF };
            chunk_in.append(reinterpret_cast<char*>(trailer), 4);
            rx_stream.avail_in += 4;
        }
        rx_stream.next_in = reinterpret_cast<Bytef*>(chunk_in.buf());
        rx_stream.avail_in = static_cast<uInt>(chunk_in.length());
        auto flush = (it_next == end) ? Z_SYNC_FLUSH : Z_NO_FLUSH;
        bool has_more_output = true;
        do {
            has_more_output = !rx_stream.avail_out;
            auto r = inflate(&rx_stream, flush);
            if (r == Z_OK && !rx_stream.avail_out) {
                auto l = acc.capacity();
                acc.length(l);
                acc.reserve(l * 2);
                rx_stream.next_out = reinterpret_cast<Bytef*>(acc.buf() + l);
                rx_stream.avail_out = static_cast<uInt>(acc.capacity() - l);
                has_more_output = true;
                continue;
            }
            else if (r < 0) {
                string err = "zlib::inflate error ";
                if (rx_stream.msg) err += rx_stream.msg;
                else err += to_string(r);
                frame.error = err;
                reset_rx();
                return false;
            }
            else {
                has_more_output = false;
            }
        } while(has_more_output);
        it_in = it_next;
    } while(it_in != end);

    acc.length(acc.capacity() - rx_stream.avail_out);

    frame.payload.resize(1);
    frame.payload.at(0) = std::move(acc);

    if(final && reset_after_rx) reset_rx();
    return true;
}



}}}
