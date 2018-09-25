use 5.020;
use warnings;
use lib 't'; use MyTest;

*gen_frame = \&MyTest::gen_frame;

sub get_server_builder {
    my $settings = shift // {};
    return MyTest::get_established_server()->start_message($settings);
};

subtest 'small server2client frame' => sub {
    my $payload = "preved"; # must be <= 125
    my $bin = get_server_builder()->final(1)->send($payload);
    is(length($bin), 8, "frame length ok"); # 2 header + 6 payload
    is_bin($bin, gen_frame({mask => 0, fin => 1, opcode => OPCODE_BINARY, data => $payload}), "frame ok");
    is_bin(get_server_builder()->final(1)->send_av([qw/pr ev ed/]), $bin, "it mode ok");
};

subtest 'medium server2client frame' => sub {
    my $payload = "preved" x 100; # must be > 125
    my $bin = get_server_builder()->final(1)->send($payload);
    is(length($bin), 604, "frame length ok"); # 2 header + 2 length + 600 payload
    is_bin($bin, gen_frame({mask => 0, fin => 1, opcode => OPCODE_BINARY, data => $payload}), "frame ok");
};

subtest 'big server2client frame' => sub {
    my $payload = "preved!" x 10000; # must be > 65536
    my $bin = get_server_builder()->final(1)->send($payload);
    is(length($bin), 70010, "frame length ok"); # 2 header + 8 length + 70000 payload
    is_bin($bin, gen_frame({mask => 0, fin => 1, opcode => OPCODE_BINARY, data => $payload}), "frame ok");
};

subtest 'small client2server frame' => sub {
    my $builder = MyTest::get_established_client()->start_message({opcode => OPCODE_TEXT, final => 1});
    my $payload = "preved"; # must be <= 125
    my $bin = $builder->send($payload);
    is(length($bin), 12, "frame length ok"); # 2 header + 4 mask + 6 payload
    is_bin($bin, gen_frame({mask => substr($bin, 2, 4), fin => 1, opcode => OPCODE_TEXT, data => $payload}), "frame ok");
};

subtest 'medium client2server frame' => sub {
    my $builder = MyTest::get_established_client()->start_message({opcode => OPCODE_TEXT, final => 1});
    my $payload = "preved" x 100; # must be > 125
    my $bin = $builder->send($payload);
    is(length($bin), 608, "frame length ok"); # 2 header + 2 length + 4 mask + 600 payload
    is_bin($bin, gen_frame({mask => substr($bin, 4, 4), fin => 1, opcode => OPCODE_TEXT, data => $payload}), "frame ok");
};

subtest 'big client2server frame' => sub {
    my $builder = MyTest::get_established_client()->start_message({opcode => OPCODE_TEXT, final => 1});
    my $payload = "preved!" x 10000; # must be > 65536
    my $bin = $builder->send($payload);
    is(length($bin), 70014, "frame length ok"); # 2 header + 8 length + 4 mask + 70000 payload
    is_bin($bin, gen_frame({mask => substr($bin, 10, 4), fin => 1, opcode => OPCODE_TEXT, data => $payload}), "frame ok");
};

subtest 'empty frame still masked' => sub {
    my $builder = MyTest::get_established_client()->start_message({opcode => OPCODE_BINARY, final => 1});
    my $bin = $builder->send("");
    is(length($bin), 6, "frame length ok"); # 2 header + 4 mask
    is_bin($bin, gen_frame({mask => substr($bin, 2, 4), fin => 1, opcode => OPCODE_BINARY}), "frame ok");
};

subtest 'opcode CONTINUE is forced for fragment frames of message (including final frame)' => sub {
    my $builder = get_server_builder({opcode => OPCODE_BINARY});
    my $bin = $builder->send("frame1");
    is_bin($bin, gen_frame({fin => 0, opcode => OPCODE_BINARY, data => "frame1"}), "initial frame ok");
    $bin = $builder->send("frame2");
    is_bin($bin, gen_frame({fin => 0, opcode => OPCODE_CONTINUE, data => "frame2"}), "fragment frame ok");
    $bin = $builder->final(1)->send("frame3");
    is_bin($bin, gen_frame({fin => 1, opcode => OPCODE_CONTINUE, data => "frame3"}), "final frame ok");

    $builder = get_server_builder({opcode => OPCODE_TEXT});
    $bin = $builder->send("frame4");
    is_bin($bin, gen_frame({fin => 0, opcode => OPCODE_TEXT, data => "frame4"}), "first frame of next message ok");
    $bin = $builder->final(1)->send("frame5"); # reset frame count
    is_bin($bin, gen_frame({fin => 1, opcode => OPCODE_CONTINUE, data => "frame5"}));
};


subtest 'control frame send' => sub {
    my $p = MyTest::get_established_server();
    my $bin = $p->send_control(OPCODE_PING, "myping");
    is_bin($bin, gen_frame({fin => 1, opcode => OPCODE_PING, data => "myping"}), "ping ok");
    $bin = $p->send_control(OPCODE_PONG, "mypong");
    is_bin($bin, gen_frame({fin => 1, opcode => OPCODE_PONG, data => "mypong"}), "pong ok");
    $bin = $p->send_control(OPCODE_CLOSE, "myclose");
    is_bin($bin, gen_frame({fin => 1, opcode => OPCODE_CLOSE, data => "myclose"}), "close ok");
    MyTest::reset($p);
};

subtest 'frame count survives control message in the middle' => sub {
    my $p = MyTest::get_established_server();
    my $builder = $p->start_message;
    my $bin = $builder->send("frame1");
    is_bin($bin, gen_frame({fin => 0, opcode => OPCODE_BINARY, data => "frame1"}), "initial frame ok");
    $bin = $p->send_control(OPCODE_PING, "");
    is_bin($bin, gen_frame({fin => 1, opcode => OPCODE_PING}), "control frame ok");
    $bin = $builder->send("frame2");
    is_bin($bin, gen_frame({fin => 0, opcode => OPCODE_CONTINUE, data => "frame2"}), "fragment frame ok");
    $bin = $p->send_control(OPCODE_PONG, "");
    is_bin($bin, gen_frame({fin => 1, opcode => OPCODE_PONG}), "control frame ok");
    $bin = $builder->final(1)->send("frame3");
    is_bin($bin, gen_frame({fin => 1, opcode => OPCODE_CONTINUE, data => "frame3"}), "final frame ok");
};

done_testing();

sub is_bin {
    my ($got, $expected, $name) = @_;
    return if our $leak_test;
    state $has_binary = eval { require Test::BinaryData; Test::BinaryData->import(); 1 };
    $has_binary ? is_binary($got, $expected, $name) : is($got, $expected, $name);
}
