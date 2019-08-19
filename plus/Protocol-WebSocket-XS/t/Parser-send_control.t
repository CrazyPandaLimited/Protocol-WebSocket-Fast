use 5.012;
use warnings;
use lib 't'; use MyTest;

*gen_frame = \&MyTest::gen_frame;

my $p = MyTest::get_established_server();

subtest 'send_control PING' => sub {
    subtest 'empty' => sub {
        my $bin = $p->send_control(OPCODE_PING);
        is(length($bin), 2, "frame length ok");
        is_bin($bin, gen_frame({fin => 1, opcode => OPCODE_PING}), "frame ok");
    };
    subtest 'with payload' => sub {
        my $bin = $p->send_control(OPCODE_PING, "h" x 125);
        is(length($bin), 127, "frame length ok");
        is_bin($bin, gen_frame({fin => 1, opcode => OPCODE_PING, data => "h" x 125}), "frame ok");
    };
    subtest 'long payload' => sub {
        my $bin = $p->send_control(OPCODE_PING, "h" x 126);
        is(length($bin), 127, "frame length ok");
        is_bin($bin, gen_frame({fin => 1, opcode => OPCODE_PING, data => "h" x 125}), "frame ok");
    }
};

subtest 'send_control PONG' => sub {
    subtest 'empty' => sub {
        my $bin = $p->send_control(OPCODE_PONG);
        is(length($bin), 2, "frame length ok");
        is_bin($bin, gen_frame({fin => 1, opcode => OPCODE_PONG}), "frame ok");
    };
    subtest 'with payload' => sub {
        my $bin = $p->send_control(OPCODE_PONG, "hi there");
        is(length($bin), 10, "frame length ok");
        is_bin($bin, gen_frame({fin => 1, opcode => OPCODE_PONG, data => "hi there"}), "frame ok");
    };
    subtest 'long payload' => sub {
        my $bin = $p->send_control(OPCODE_PONG, "h" x 126);
        is(length($bin), 127, "frame length ok");
        is_bin($bin, gen_frame({fin => 1, opcode => OPCODE_PONG, data => "h" x 125}), "frame ok");
    };
};

subtest 'send_control CLOSE' => sub {
    subtest 'empty' => sub {
        my $bin = $p->send_control(OPCODE_CLOSE);
        is(length($bin), 2, "frame length ok");
        is_bin($bin, gen_frame({fin => 1, opcode => OPCODE_CLOSE}), "frame ok");
    };
    
    ok(!eval { $p->start_message->final(1)->send("asdf"); 1 }, "can't send after CLOSE sent");
    MyTest::reset($p);
    ok(eval { $p->start_message->final(1)->send("asdf"); 1 }, "can send after reset");
    
    subtest 'with payload' => sub {
        my $bin = $p->send_control(OPCODE_CLOSE, "hi there");
        is(length($bin), 10, "frame length ok");
        is_bin($bin, gen_frame({fin => 1, opcode => OPCODE_CLOSE, data => "hi there"}), "frame ok");
        MyTest::reset($p);
    };

    subtest 'long payload' => sub {
        my $bin = $p->send_control(OPCODE_CLOSE, "h" x 126);
        is(length($bin), 127, "frame length ok");
        is_bin($bin, gen_frame({fin => 1, opcode => OPCODE_CLOSE, data => "h" x 125}), "frame ok");
        MyTest::reset($p);
    };
};

subtest 'send_ping' => sub {
    subtest 'empty' => sub {
        my $bin = $p->send_ping();
        is(length($bin), 2, "frame length ok");
        is_bin($bin, gen_frame({fin => 1, opcode => OPCODE_PING}), "frame ok");
    };
    subtest 'with payload' => sub {
        my $bin = $p->send_ping("hi buddy");
        is(length($bin), 10, "frame length ok");
        is_bin($bin, gen_frame({fin => 1, opcode => OPCODE_PING, data => "hi buddy"}), "frame ok");
    };
    subtest 'long payload' => sub {
        my $bin = $p->send_ping("h" x 126);
        is(length($bin), 127, "frame length ok");
        is_bin($bin, gen_frame({fin => 1, opcode => OPCODE_PING, data => "h" x 125}), "frame ok");
    };
};

subtest 'send_pong' => sub {
    subtest 'empty' => sub {
        my $bin = $p->send_pong();
        is(length($bin), 2, "frame length ok");
        is_bin($bin, gen_frame({fin => 1, opcode => OPCODE_PONG}), "frame ok");
    };
    subtest 'with payload' => sub {
        my $bin = $p->send_pong("hi buddy");
        is(length($bin), 10, "frame length ok");
        is_bin($bin, gen_frame({fin => 1, opcode => OPCODE_PONG, data => "hi buddy"}), "frame ok");
    };
    subtest 'long payload' => sub {
        my $bin = $p->send_pong("h" x 126);
        is(length($bin), 127, "frame length ok");
        is_bin($bin, gen_frame({fin => 1, opcode => OPCODE_PONG, data => "h" x 125}), "frame ok");
    };
};

subtest 'send_close' => sub {
    subtest 'empty' => sub {
        my $bin = $p->send_close();
        is(length($bin), 2, "frame length ok");
        is_bin($bin, gen_frame({fin => 1, opcode => OPCODE_CLOSE}), "frame ok");
        MyTest::reset($p);
    };
    subtest 'with code' => sub {
        my $bin = $p->send_close(CLOSE_DONE);
        is(length($bin), 4, "frame length ok");
        is_bin($bin, gen_frame({fin => 1, opcode => OPCODE_CLOSE, close_code => CLOSE_DONE}), "frame ok");
        MyTest::reset($p);
    };
    subtest 'with code and payload' => sub {
        my $bin = $p->send_close(CLOSE_AWAY, "f" x 123);
        is(length($bin), 127, "frame length ok");
        is_bin($bin, gen_frame({fin => 1, opcode => OPCODE_CLOSE, close_code => CLOSE_AWAY, data => "f" x 123}), "frame ok");
        MyTest::reset($p);
    };
    subtest 'with code and long payload' => sub {
        my $bin = $p->send_close(CLOSE_AWAY, "f" x 127);
        is(length($bin), 127, "frame length ok");
        is_bin($bin, gen_frame({fin => 1, opcode => OPCODE_CLOSE, close_code => CLOSE_AWAY, data => "f" x 123}), "frame ok");
        MyTest::reset($p);
    };
};

subtest 'control frames do not reset message state in frame mode' => sub {
    my $builder = $p->start_message({deflate => 0});
    my $bin = $builder->send("frame1");
    is_bin($bin, gen_frame({fin => 0, opcode => OPCODE_BINARY, data => "frame1"}), "initial frame ok");
    $p->send_control(OPCODE_PING);
    $p->send_control(OPCODE_PONG);
    $p->send_ping;
    $p->send_pong;
    $bin = $builder->send("frame2");
    is_bin($bin, gen_frame({fin => 0, opcode => OPCODE_CONTINUE, data => "frame2"}), "fragment frame ok");
    $p->send_control(OPCODE_PING);
    $p->send_control(OPCODE_PONG);
    $p->send_ping;
    $p->send_pong;
    $bin = $builder->final(1)->send("frame3");
    is_bin($bin, gen_frame({fin => 1, opcode => OPCODE_CONTINUE, data => "frame3"}), "final frame ok");
};

done_testing();
