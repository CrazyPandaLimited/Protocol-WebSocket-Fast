#pragma once
#include <cassert>
#include <panda/refcnt.h>
#include <panda/optional.h>
#include <panda/protocol/websocket/HTTPRequest.h>
#include <panda/protocol/websocket/Frame.h>
#include <zlib.h>

namespace panda { namespace protocol { namespace websocket {

class DeflateExt {
public:

    struct Config {
        bool client_no_context_takeover = false;    // sent is only, when it is true; always parsed
        bool server_no_context_takeover = false;    // sent is only, when it is true; always parsed
        std::uint8_t server_max_window_bits = 15;
        std::uint8_t client_max_window_bits = 15;

        // copied from parser config, no direct usage
        size_t max_message_size;
        // non-negotiatiable settings
        int mem_level = 8;
        int compression_level = Z_DEFAULT_COMPRESSION;
        int strategy = Z_DEFAULT_STRATEGY;
        size_t compression_threshold = 1410;  // try to fit into TCP frame
    };

    struct EffectiveConfig {
        static const constexpr int HAS_CLIENT_NO_CONTEXT_TAKEOVER = 1 << 0;
        static const constexpr int HAS_SERVER_NO_CONTEXT_TAKEOVER = 1 << 1;
        static const constexpr int HAS_SERVER_MAX_WINDOW_BITS     = 1 << 2;
        static const constexpr int HAS_CLIENT_MAX_WINDOW_BITS     = 1 << 3;

        Config cfg;
        int flags = 0;
    };

    enum class Role { CLIENT, SERVER };

    static const char* extension_name;

    static panda::optional<panda::string> bootstrap();

    static panda::optional<EffectiveConfig> select(const HTTPPacket::HeaderValues& values, const Config& cfg, Role role);
    static void request(HTTPPacket::HeaderValues& ws_extensions, const Config& cfg);
    static DeflateExt* uplift(const EffectiveConfig& cfg, HTTPPacket::HeaderValues& extensions, Role role);

    ~DeflateExt();

    void reset_tx();
    void reset() {
        reset_rx();
        reset_tx();
    }
    string& compress(string& str, bool final);

    template<typename It>
    It compress(It payload_begin, It payload_end, bool final) {
        // we should not try to compress empty last piece
        assert(!(final && (payload_begin == payload_end)));

        It it_in = payload_begin;
        It it_out = payload_begin;
        tx_stream.avail_out = 0;

        string* chunk_out = nullptr;
        while(it_in != payload_end) {
            auto it_next = it_in + 1;
            string chunk_in = *it_in;
            tx_stream.next_in = reinterpret_cast<Bytef*>(chunk_in.buf());
            tx_stream.avail_in = static_cast<uInt>(chunk_in.length());
            // for last fragment we either complete frame(Z_SYNC_FLUSH) or message(flush_policy())
            // otherwise no flush it perfromed
            auto flush = (it_next == payload_end) ? Z_SYNC_FLUSH : Z_NO_FLUSH;
            auto avail_out = tx_stream.avail_out;
            do {
                do {
                    if (tx_stream.avail_out == 0) {
                        if (it_out < it_next) {
                            if (chunk_out) {
                                auto tx_out = avail_out - tx_stream.avail_out;
                                chunk_out->length(chunk_out->length() + tx_out);
                            }
                            chunk_out = &(*it_out++);
                            chunk_out->length(0);
                            tx_stream.next_out = reinterpret_cast<Bytef*>(chunk_out->shared_buf());
                            avail_out = tx_stream.avail_out = static_cast<uInt>(chunk_out->shared_capacity());
                        }
                        else {
                            chunk_out->reserve(chunk_out->shared_capacity() + 6);
                            tx_stream.avail_out = 6;
                        }
                    }
                    auto r = deflate(&tx_stream, flush);
                    if (r < 0) {
                        assert(0);
                    }
                } while(tx_stream.avail_out == 0);
            } while(tx_stream.avail_in);
            auto tx_out = avail_out - tx_stream.avail_out;
            chunk_out->length(chunk_out->length() + tx_out);
            it_in = it_next;
        }
        if(final) {
            // remove tail empty-frame 0x00 0x00 0xff 0xff
            size_t tail_left = 4;
            while(tail_left){
                --it_out;
                string& chunk = *it_out;
                int delta = chunk.length() - tail_left;
                if (delta >= 0) {
                    chunk.length(delta);
                    tail_left = 0;
                }
                else {
                    tail_left -= chunk.length();
                    chunk.length(0);
                }
            }
            ++it_out;
            if(reset_after_tx) reset_tx();
        }
        return it_out;
    }

    bool uncompress(Frame& frame);

private:
    void reset_rx();
    bool uncompress_impl(Frame& frame);

    DeflateExt(const Config& cfg, Role role);
    size_t message_size;
    size_t max_message_size;
    z_stream rx_stream;
    z_stream tx_stream;
    bool reset_after_tx;
    bool reset_after_rx;
};

}}}
