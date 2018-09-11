#include "DeflateExt.h"
#include <charconv>
namespace panda { namespace protocol { namespace websocket {

const char* DeflateExt::extension_name = "permessage-deflate";

void DeflateExt::request(HTTPPacket::HeaderValues& ws_extensions, const config_t& cfg) {
    HTTPPacket::HeaderValueParams params;
    params.emplace("server_max_window_bits", panda::to_string(cfg.server_max_window_bits));
    params.emplace("client_max_window_bits", panda::to_string(cfg.client_max_window_bits));
    if(cfg.server_no_context_takeover) params.emplace("server_no_context_takeover", "");
    if(cfg.client_no_context_takeover) params.emplace("client_no_context_takeover", "");

    string name{extension_name};
    HTTPPacket::HeaderValue hv {name, std::move(params)};
    ws_extensions.emplace_back(std::move(hv));
}


const HTTPPacket::HeaderValue* DeflateExt::select(const HTTPPacket::HeaderValues& values) {
    for(auto& header: values) {
        if (header.name == extension_name) {
            bool params_correct = true;
            for(auto& it: header.params) {
                auto& param_name = it.first;
                auto& param_value = it.first;
                if (param_name == "server_no_context_takeover") params_correct = params_correct && param_value == "";
                else if (param_name == "client_no_context_takeover") params_correct = params_correct && param_value == "";
                else if ((param_name == "server_max_window_bits") || (param_name == "client_max_window_bits")) {
                    std::int8_t bits;
                    auto res = std::from_chars(param_value.data(), param_value.data() + param_name.size(), bits, 10);
                    params_correct = params_correct && !res.ec && (bits >= 8) && (bits <= 15);
                }
            }
            if (params_correct) return &header;
        }
    }
    return nullptr;
}

DeflateExt* DeflateExt::uplift(const HTTPPacket::HeaderValue& deflate_extension, HTTPPacket::HeaderValues& extensions) {
    config_t cfg;
    for(auto& it: deflate_extension.params) {
        auto& param_name = it.first;
        auto& param_value = it.first;
        if (param_name == "server_no_context_takeover") cfg.server_no_context_takeover = true;
        if (param_name == "client_no_context_takeover") cfg.client_no_context_takeover = true;
        if (param_name == "server_max_window_bits") cfg.server_max_window_bits = static_cast<std::uint8_t>(panda::stoi(param_value));
        if (param_name == "client_max_window_bits") cfg.client_max_window_bits = static_cast<std::uint8_t>(panda::stoi(param_value));
    }
    // echo back best match
    extensions.push_back(deflate_extension);
    return new DeflateExt(cfg);
}


}}}
