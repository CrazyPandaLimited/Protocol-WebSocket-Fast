#include <catch.hpp>
#include <vector>
#include <xs/protocol/websocket.h>
#include <panda/encode/base16.h>

using namespace panda;
using namespace panda::protocol::websocket;

template <typename T>
string to_string(T range) {
    string r;
    for (const string& s : range) r += s;
    return r;
}


TEST_CASE("FrameBuilder & Message builder", "[deflate-extension]") {
    Parser::Config cfg;
    cfg.deflate_config->compression_threshold = 0;

    ServerParser server;
    server.configure(cfg);
    ClientParser client;

    ConnectRequestSP connect_request(new ConnectRequest());
    connect_request->uri = new URI("ws://crazypanda.ru");
    connect_request->ws_key = "dGhlIHNhbXBsZSBub25jZQ==";
    connect_request->ws_version = 13;
    connect_request->ws_protocol = "ws";
    auto client_request = client.connect_request(connect_request);

    REQUIRE((bool)server.accept(client_request));
    auto server_reply = server.accept_response();
    client.connect(server_reply);

    REQUIRE(server.established());
    REQUIRE(client.established());
    REQUIRE(server.is_deflate_active());
    REQUIRE(client.is_deflate_active());

    SECTION("FrameBuffer") {
        SECTION("send (iterator)") {
            std::vector<string> fragments;
            fragments.push_back("hello");
            fragments.push_back(" world");
            auto data = server.start_message().deflate(true).final(true)
                .send(fragments.begin(), fragments.end());
            auto data_string = to_string(data);
            auto messages_it = client.get_messages(data_string);
            REQUIRE(std::distance(messages_it.begin(), messages_it.end()) == 1);
            REQUIRE(messages_it.begin()->payload[0] == "hello world");
        }

        SECTION("send (iterator, empty)") {
            std::vector<string> fragments;
            auto data = server.start_message().deflate(true).final(true)
                .send(fragments.begin(), fragments.end());
            auto data_string = to_string(data);
            auto messages_it = client.get_messages(data_string);
            REQUIRE(std::distance(messages_it.begin(), messages_it.end()) == 1);
            REQUIRE(messages_it.begin()->payload.empty());
        }
    }

    SECTION("MessageBuilder") {
        SECTION("MessageBuilder::send (fragmented message iterator)") {
            std::vector<string> fragments;
            fragments.push_back("hello");
            fragments.push_back(" world");
            auto data = server.message().send(fragments.begin(), fragments.end());
            auto data_string = to_string(data);
            auto messages_it = client.get_messages(data_string);
            REQUIRE(std::distance(messages_it.begin(), messages_it.end()) == 1);
            REQUIRE(messages_it.begin()->payload[0] == "hello world");
        }

        SECTION("MessageBuilder::send (fragmented multi-frame iterator, 1 fragment)") {
            std::vector<std::vector<string>> pieces;

            std::vector<string> fragments1;
            fragments1.push_back("hello");
            fragments1.push_back(" world!");
            pieces.push_back(fragments1);


            auto builder = server.message();
            auto data = builder.deflate(true).send(pieces.begin(), pieces.end());
            auto data_string = to_string(data);
            REQUIRE(data_string != "hello world!");
            auto messages_it = client.get_messages(data_string);
            REQUIRE(std::distance(messages_it.begin(), messages_it.end()) == 1);

            auto it = messages_it.begin();
            REQUIRE(it->payload[0] == "hello world!");
        }

        SECTION("MessageBuilder::send (fragmented multi-frame iterator, 2 fragments)") {
            std::vector<std::vector<string>> pieces;

            std::vector<string> fragments1;
            fragments1.push_back("hello");
            fragments1.push_back(" world!");
            pieces.push_back(fragments1);

            std::vector<string> fragments2;
            fragments2.push_back(" Let's do ");
            fragments2.push_back("some testing");
            pieces.push_back(fragments2);

            auto builder = server.message();
            auto data = builder.deflate(true).send(pieces.begin(), pieces.end());
            auto data_string = to_string(data);
            REQUIRE(data_string.find("hello") == std::string::npos);
            auto messages_it = client.get_messages(data_string);
            REQUIRE(std::distance(messages_it.begin(), messages_it.end()) == 1);

            auto it = messages_it.begin();
            REQUIRE(it->payload.size() == 2);
            REQUIRE(it->payload[0] == "hello world!");
            REQUIRE(it->payload[1] == " Let's do some testing");
        }

        SECTION("MessageBuilder::send (fragmented multi-frame iterator, 2 fragments, last empty)") {
            std::vector<std::vector<string>> pieces;

            std::vector<string> fragments1;
            fragments1.push_back("hello");
            fragments1.push_back(" world!");
            pieces.push_back(fragments1);

            std::vector<string> fragments2;
            pieces.push_back(fragments2);

            auto builder = server.message();
            auto data = builder.deflate(true).send(pieces.begin(), pieces.end());
            auto data_string = to_string(data);
            REQUIRE(data_string.find("hello") == std::string::npos);
            auto messages_it = client.get_messages(data_string);
            REQUIRE(std::distance(messages_it.begin(), messages_it.end()) == 1);

            auto it = messages_it.begin();
            REQUIRE(it->payload.size() == 1);
            REQUIRE(it->payload[0] == "hello world!");
        }

        SECTION("MessageBuilder::send (fragmented multi-frame iterator, 2 fragments, both empty)") {
            std::vector<std::vector<string>> pieces;

            std::vector<string> fragments1;
            pieces.push_back(fragments1);

            std::vector<string> fragments2;
            pieces.push_back(fragments2);

            auto builder = server.message();
            auto data = builder.deflate(true).send(pieces.begin(), pieces.end());
            auto data_string = to_string(data);
            REQUIRE(data_string.find("hello") == std::string::npos);
            auto messages_it = client.get_messages(data_string);
            REQUIRE(std::distance(messages_it.begin(), messages_it.end()) == 1);

            auto it = messages_it.begin();
            REQUIRE(it->payload.empty());
        }
    }

    SECTION("empty compressed frame with zero payload") {
        string payload;
        REQUIRE(payload.length() == 0);
        //auto builder =
        auto data = server.start_message().deflate(true).final(true).send(payload);
        auto data_string = to_string(data);

        SECTION("zero uncompressed payload") {
            auto messages_it = client.get_messages(data_string);
            REQUIRE(std::distance(messages_it.begin(), messages_it.end()) == 1);
            REQUIRE(messages_it.begin()->payload_length() == 0);
        }

        SECTION("non-zero network payload") {
            auto frames_it = client.get_frames(data_string);
            REQUIRE(std::distance(frames_it.begin(), frames_it.end()) == 1);
            REQUIRE(frames_it.begin()->payload_length() == 1);
        }
    }


    SECTION("compressed frame with zero payload") {
        string payload;
        REQUIRE(payload.length() == 0);
        FrameHeader fh(Opcode::TEXT, true, true, false, false, true, (uint32_t)std::rand());
        auto data_string = Frame::compile(fh, payload);
        auto messages_it = client.get_messages(data_string);
        REQUIRE(std::distance(messages_it.begin(), messages_it.end()) == 1);
        REQUIRE(messages_it.begin()->payload_length() == 0);
    }

    SECTION("Control compressed frame") {
        string payload;
        FrameHeader fh(Opcode::PING, true, true, false, false, true, (uint32_t)std::rand());
        auto data_string = Frame::compile(fh, payload);
        auto frames_it = client.get_frames(data_string);
        REQUIRE(frames_it.begin()->error == "compression of control frames is not allowed (rfc7692)");
    }

    SECTION("send compressed frame bigger then original") {
        string payload = encode::decode_base16("8e008f8f8f0090909000919191009292");
        //string payload = "hell";

        std::vector<string> fragments;
        fragments.push_back(payload);
        auto data = server.start_message().deflate(true).final(true).send(fragments.begin(), fragments.end());
        auto data_string = to_string(data);
        auto messages_it = client.get_messages(data_string);
        REQUIRE(std::distance(messages_it.begin(), messages_it.end()) == 1);
        REQUIRE(messages_it.begin()->error == "");
        //REQUIRE(messages_it.begin()->payload[0] == payload);
    }


}
