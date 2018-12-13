#include "DeflateExt.h"
#include <panda/lib/from_chars.h>

namespace panda { namespace protocol { namespace websocket {

const char* DeflateExt::extension_name = "permessage-deflate";
static const char* PARAM_SERVER_NO_CONTEXT_TAKEOVER = "server_no_context_takeover";
static const char* PARAM_CLIENT_NO_CONTEXT_TAKEOVER = "client_no_context_takeover";
static const char* PARAM_SERVER_MAX_WINDOW_BITS = "server_max_window_bits";
static const char* PARAM_CLIENT_MAX_WINDOW_BITS = "client_max_window_bits";

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
    params.emplace(PARAM_SERVER_MAX_WINDOW_BITS, panda::to_string(cfg.server_max_window_bits));
    params.emplace(PARAM_CLIENT_MAX_WINDOW_BITS, panda::to_string(cfg.client_max_window_bits));
    if(cfg.server_no_context_takeover) params.emplace(PARAM_SERVER_NO_CONTEXT_TAKEOVER, "");
    if(cfg.client_no_context_takeover) params.emplace(PARAM_CLIENT_NO_CONTEXT_TAKEOVER, "");

    string name{extension_name};
    HTTPPacket::HeaderValue hv {name, std::move(params)};
    ws_extensions.emplace_back(std::move(hv));
}


static bool get_window_bits(const string& value, std::uint8_t& bits) {
    auto res = std::from_chars(value.data(), value.data() + value.size(), bits, 10);
    return !res.ec && (bits >= 9) && (bits <= 15);
}

DeflateExt::EffectiveConfig DeflateExt::select(const HTTPPacket::HeaderValues& values, const Config& cfg, Role role) {
    for(auto& header: values) {
        if (header.name == extension_name) {
            EffectiveConfig ecfg(cfg, EffectiveConfig::NegotiationsResult::ERROR);
            bool params_correct = true;
            for(auto it = begin(header.params); params_correct && it != end(header.params); ++it) {
                auto& param_name = it->first;
                auto& param_value = it->second;
                if (param_name == PARAM_SERVER_NO_CONTEXT_TAKEOVER) {
                    ecfg.flags |= EffectiveConfig::HAS_SERVER_NO_CONTEXT_TAKEOVER;
                    ecfg.cfg.server_no_context_takeover = true;
                }
                else if (param_name == PARAM_CLIENT_NO_CONTEXT_TAKEOVER) {
                    ecfg.flags |= EffectiveConfig::HAS_CLIENT_NO_CONTEXT_TAKEOVER;
                    ecfg.cfg.client_no_context_takeover = true;
                }
                else if (param_name == PARAM_SERVER_MAX_WINDOW_BITS) {
                    ecfg.flags |= EffectiveConfig::HAS_SERVER_MAX_WINDOW_BITS;
                    std::uint8_t bits;
                    params_correct = get_window_bits(param_value, bits);
                    if (params_correct) {
                        ecfg.cfg.server_max_window_bits = bits;
                        if (role == Role::CLIENT) {
                            params_correct = bits == cfg.server_max_window_bits;
                        } else {
                            params_correct = bits <= cfg.server_max_window_bits;
                        }
                    }
                }
                else if (param_name == PARAM_CLIENT_MAX_WINDOW_BITS) {
                    ecfg.flags |= EffectiveConfig::HAS_CLIENT_MAX_WINDOW_BITS;
                    std::uint8_t bits;
                    // value is optional
                    if (param_value) {
                        params_correct = get_window_bits(param_value, bits);
                        ecfg.cfg.client_max_window_bits = bits;
                        params_correct = params_correct && (
                                (role == Role::CLIENT) ? bits == cfg.client_max_window_bits
                                                       : bits <= cfg.client_max_window_bits
                            );
                    } else {
                        ecfg.cfg.client_max_window_bits = 15;
                        // the value must be supplied in server response, otherwise (for client) it is invalid
                        params_correct = role == Role::SERVER;
                    }
                } else { params_correct = false; }  // unknown parameter
            }
            if (params_correct) {
                // first best match wins (for server & client)
                ecfg.result = EffectiveConfig::NegotiationsResult::SUCCESS;
                return ecfg;
            }
            else if (role == Role::CLIENT) {
                // first fail (and terminate connection)
                return ecfg;
            }

        }
    }
    return EffectiveConfig(EffectiveConfig::NegotiationsResult::NOT_FOUND);
}

DeflateExt* DeflateExt::uplift(const EffectiveConfig& ecfg, HTTPPacket::HeaderValues& extensions, Role role) {
    HTTPPacket::HeaderValueParams params;
    if (ecfg.flags & EffectiveConfig::HAS_SERVER_NO_CONTEXT_TAKEOVER) {
        params.emplace(PARAM_SERVER_NO_CONTEXT_TAKEOVER, "");
    }
    if (ecfg.flags & EffectiveConfig::HAS_CLIENT_NO_CONTEXT_TAKEOVER) {
        params.emplace(PARAM_CLIENT_NO_CONTEXT_TAKEOVER, "");
    }
    if (ecfg.flags & EffectiveConfig::HAS_SERVER_MAX_WINDOW_BITS) {
        params.emplace(PARAM_SERVER_MAX_WINDOW_BITS, to_string(ecfg.cfg.server_max_window_bits));
    }
    if (ecfg.flags & EffectiveConfig::HAS_CLIENT_MAX_WINDOW_BITS) {
        params.emplace(PARAM_CLIENT_MAX_WINDOW_BITS, to_string(ecfg.cfg.client_max_window_bits));
    }
    extensions.emplace_back(HTTPPacket::HeaderValue{string(extension_name), params});
    return new DeflateExt(ecfg.cfg, role);
}


DeflateExt::DeflateExt(const DeflateExt::Config& cfg, Role role): effective_cfg{cfg}, message_size{0}, max_message_size{cfg.max_message_size} {
    auto rx_window = role == Role::CLIENT ? cfg.server_max_window_bits : cfg.client_max_window_bits;
    auto tx_window = role == Role::CLIENT ? cfg.client_max_window_bits : cfg.server_max_window_bits;

    //rx_stream.next_in = reinterpret_cast<Bytef*>(rx_buff.buf());
    rx_stream.avail_in = 0;
    rx_stream.zalloc = Z_NULL;
    rx_stream.zfree = Z_NULL;
    rx_stream.opaque = Z_NULL;

    // -1 is used as "raw deflate", i.e. do not emit header/trailers
    auto r = inflateInit2(&rx_stream, -1 * rx_window);
    if (r != Z_OK) {
        panda::string err = "zlib::inflateInit2 error";
        if (rx_stream.msg) err.append(panda::string(" : ") + rx_stream.msg);
        throw std::runtime_error(err);
    }

    //tx_stream.next_in = reinterpret_cast<Bytef*>(tx_buff.buf());
    tx_stream.avail_in = 0;
    tx_stream.zalloc = Z_NULL;
    tx_stream.zfree = Z_NULL;
    tx_stream.opaque = Z_NULL;

    // -1 is used as "raw deflate", i.e. do not emit header/trailers
    r = deflateInit2(&tx_stream, cfg.compression_level, Z_DEFLATED, -1 * tx_window , cfg.mem_level, cfg.strategy);
    if (r != Z_OK) {
        panda::string err = "zlib::deflateInit2 error";
        if (rx_stream.msg) err.append(panda::string(" : ") + rx_stream.msg);
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
    string in_copy = str;
    tx_stream.next_in = reinterpret_cast<Bytef*>(in_copy.buf());
    tx_stream.avail_in = static_cast<uInt>(in_copy.length());
    tx_stream.next_out = reinterpret_cast<Bytef*>(str.shared_buf());
    auto sz = str.capacity();
    str.length(sz);
    tx_stream.avail_out = static_cast<uInt>(sz);

    deflate_iteration(Z_SYNC_FLUSH, [&](){
        sz += reserve_for_trailer(str);
    });

    sz -= tx_stream.avail_out;
    // remove tail empty-frame 0x00 0x00 0xff 0xff for final messages only
    assert(sz);
    if (final) sz -= TRAILER_SIZE;
    str.length(sz);

    if(final && reset_after_tx) reset_tx();
    return str;
}

bool DeflateExt::uncompress(Frame& frame) {
    bool r;
    if (frame.error) r = false;
    else if (frame.is_control()) {
        frame.error = "compression of control frames is not allowed (rfc7692)";
        r = false;
    }
    else if (frame.payload_length() == 0) r = true;
    else {
        r = uncompress_impl(frame);
    }
    // reset stream in case of a) error and b) when it was last frame of message
    // and there was setting to do not use
    if(!r || (frame.final() && reset_after_rx)) reset_rx();
    if (frame.final()) message_size = 0;
    return r;
}

bool DeflateExt::uncompress_impl(Frame& frame) {
    using It = decltype(frame.payload)::iterator;

    bool final = frame.final();
    It it_in = frame.payload.begin();
    It end = frame.payload.end();

    string acc;
    acc.reserve(frame.payload_length() * 2);

    rx_stream.next_out = reinterpret_cast<Bytef*>(acc.buf());
    rx_stream.avail_out = static_cast<uInt>(acc.capacity());
    do {
        string& chunk_in = *it_in;
        It it_next = ++it_in;
        if (it_next == end && final) {
            // append empty-frame 0x00 0x00 0xff 0xff
            unsigned char trailer[TRAILER_SIZE] = { 0x00,  0x00, 0xFF, 0xFF };
            chunk_in.append(reinterpret_cast<char*>(trailer), TRAILER_SIZE);
            rx_stream.avail_in += TRAILER_SIZE;
        }
        rx_stream.next_in = reinterpret_cast<Bytef*>(chunk_in.buf());
        rx_stream.avail_in = static_cast<uInt>(chunk_in.length());
        auto flush = (it_next == end) ? Z_SYNC_FLUSH : Z_NO_FLUSH;
        bool has_more_output = true;
        do {
            has_more_output = !rx_stream.avail_out;
            auto r = inflate(&rx_stream, flush);
            if (r == Z_OK && max_message_size) {
                auto unpacked_frame_size = acc.capacity() - rx_stream.avail_out;
                auto unpacked_message_size = message_size + unpacked_frame_size;
                if (unpacked_message_size > max_message_size) {
                    frame.error = "zlib::inflate error: max message size has been reached";
                    return false;
                }
            }
            if (r == Z_OK && !rx_stream.avail_out) {
                auto l = acc.capacity();
                acc.length(l);
                acc.reserve(3 * l / 2); // * 1.5
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
                return false;
            }
            else {
                has_more_output = false;
            }
        } while(has_more_output);
        it_in = it_next;
    } while(it_in != end);

    acc.length(acc.capacity() - rx_stream.avail_out);
    if (max_message_size) message_size += acc.length();

    frame.payload.resize(1);
    frame.payload.at(0) = std::move(acc);

    return true;
}



}}}
