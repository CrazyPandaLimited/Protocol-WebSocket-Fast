#include "../test.h"

#define TEST(name) TEST_CASE("send-frame: " name, "[send-frame]")

//my $create_builder = sub {
//    my ($server_or_client, $settings) = @_;
//    return $create_parser->($server_or_client)->start_message(%$settings);
//}

TEST("server -> client frame") {
    EstablishedServerParser p;

    SECTION("small frame") {
        string payload = "preved"; // must be <= 125
        auto bin = p.start_message().send(payload, IsFinal::YES);
        CHECK_BINFRAME(bin).final().opcode(Opcode::BINARY).payload(payload).binlen(2 + 6); // 2 header + 6 payload
        std::vector<string> vpl = {"pr", "ev", "ed"};
        auto bin2 = p.start_message().send(vpl.begin(), vpl.end(), IsFinal::YES);
        CHECK((bin2 == bin)); // check iterator mode
    }

    SECTION("medium frame") {
        string payload = repeat("preved", 100); // must be: 125 < X < 65536
        auto bin = p.start_message().send(payload, IsFinal::YES);
        CHECK_BINFRAME(bin).final().opcode(Opcode::BINARY).payload(payload).binlen(2 + 2 + 600); // 2 header + 2 length + 600 payload
    }

    SECTION("big frame") {
        string payload = repeat("preved!", 10000); // must be > 65536
        auto bin = p.start_message().send(payload, IsFinal::YES);
        CHECK_BINFRAME(bin).final().opcode(Opcode::BINARY).payload(payload).binlen(2 + 8 + 70000);
    }
}

TEST("client -> server frame") {
    EstablishedClientParser p;

    SECTION("small frame") {
        string payload = "preved"; // must be <= 125
        auto bin = p.start_message(Opcode::TEXT).send(payload, IsFinal::YES);
        CHECK_BINFRAME(bin).mask(bin.substr(2, 4)).final().opcode(Opcode::TEXT).payload(payload).binlen(2 + 4 + 6); // 2 header + 4 mask + 6 payload

        EstablishedServerParser p;
        auto f = get_frame(p, bin);
        CHECK_FRAME(f).final().payload(payload);
    }

    SECTION("medium frame") {
        string payload = repeat("preved", 100); // must be: 125 < X < 65536
        auto bin = p.start_message(Opcode::TEXT).send(payload, IsFinal::YES);
        CHECK_BINFRAME(bin).mask(bin.substr(4, 4)).final().opcode(Opcode::TEXT).payload(payload).binlen(2 + 2 + 4 + 600); // 2 header + 2 length + 4 mask + 600 payload
    }

    SECTION("big frame") {
        string payload = repeat("preved!", 10000); // must be > 65536
        auto bin = p.start_message(Opcode::TEXT).send(payload, IsFinal::YES);
        CHECK_BINFRAME(bin).mask(bin.substr(10, 4)).final().opcode(Opcode::TEXT).payload(payload).binlen(2 + 8 + 4 + 70000);
    }
}



//TEST("empty frame still masked") {
//    my $builder = $create_builder->(1,{opcode(Opcode::BINARY});
//    auto bin = $builder->send("", 1);
//    CHECK(bin.length() == 6); // 2 header + 4 mask
//    CHECK_BINFRAME(bin).mask => substr($bin, 2, 4), final().opcode(Opcode::BINARY});
//}
//
//TEST("opcode CONTINUE is forced for fragment frames of message (including final frame)") {
//    my $builder = $create_builder->(0,{opcode(Opcode::BINARY});
//    auto bin = $builder->send("frame1");
//    CHECK_BINFRAME(bin).opcode(Opcode::BINARY.payload("frame1"}), "initial frame ok");
//    $bin = $builder->send("frame2");
//    CHECK_BINFRAME(bin).opcode(Opcode::CONTINUE.payload("frame2"}), "fragment frame ok");
//    $bin = $builder->send("frame3", 1);
//    CHECK_BINFRAME(bin).final().opcode(Opcode::CONTINUE.payload("frame3"}), "final frame ok");
//
//    $builder = $create_builder->(0,{opcode(Opcode::TEXT});
//    $bin = $builder->send("frame4");
//    CHECK_BINFRAME(bin).opcode(Opcode::TEXT.payload("frame4"}), "first frame of next message ok");
//    $bin = $builder->send("frame5", 1); // reset frame count
//    CHECK_BINFRAME(bin).final().opcode(Opcode::CONTINUE.payload("frame5"}));
//}
//
//TEST("control frame send") {
//    my $p = $create_parser->(0);
//    auto bin = $p->send_control(OPCODE_PING, "myping");
//    CHECK_BINFRAME(bin).final().opcode(Opcode::PING.payload("myping"}), "ping ok");
//    $bin = $p->send_control(OPCODE_PONG, "mypong");
//    CHECK_BINFRAME(bin).final().opcode(Opcode::PONG.payload("mypong"}), "pong ok");
//    $bin = $p->send_control(OPCODE_CLOSE, "myclose");
//    CHECK_BINFRAME(bin).final().opcode(Opcode::CLOSE.payload("myclose"}), "close ok");
//    MyTest::reset($p);
//}
//
//TEST("frame count survives control message in the middle") {
//    my $p = $create_parser->(0);
//    my $builder = $p->start_message;
//    auto bin = $builder->send("frame1");
//    CHECK_BINFRAME(bin).opcode(Opcode::BINARY.payload("frame1"}), "initial frame ok");
//    $bin = $p->send_control(OPCODE_PING, "");
//    CHECK_BINFRAME(bin).final().opcode(Opcode::PING}), "control frame ok");
//    $bin = $builder->send("frame2");
//    CHECK_BINFRAME(bin).opcode(Opcode::CONTINUE.payload("frame2"}), "fragment frame ok");
//    $bin = $p->send_control(OPCODE_PONG, "");
//    CHECK_BINFRAME(bin).final().opcode(Opcode::PONG}), "control frame ok");
//    $bin = $builder->send("frame3", 1);
//    CHECK_BINFRAME(bin).final().opcode(Opcode::CONTINUE.payload("frame3"}), "final frame ok");
//}
