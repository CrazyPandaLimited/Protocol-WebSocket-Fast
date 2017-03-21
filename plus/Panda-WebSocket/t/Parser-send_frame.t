use 5.020;
use warnings;
use lib 't/lib'; use WSTest;

my $has_binary = eval { require Test::BinaryData; Test::BinaryData->import(); 1 };

*gen_frame = \&WSTest::gen_frame;

my $p = WSTest::get_established_server();

subtest 'small server2client frame' => sub {
    my $payload = "preved"; # must be <= 125
    my $bin = $p->send_frame(1, $payload);
    is(length($bin), 8, "frame length ok"); # 2 header + 6 payload
    is_bin($bin, gen_frame({mask => 0, fin => 1, opcode => OPCODE_BINARY, data => $payload}), "frame ok");
    is_bin($p->send_frame_av(1, [qw/pr ev ed/]), $bin, "it mode ok");
};

subtest 'medium server2client frame' => sub {
    my $payload = "preved" x 100; # must be > 125
    my $bin = $p->send_frame(1, $payload);
    is(length($bin), 604, "frame length ok"); # 2 header + 2 length + 600 payload
    is_bin($bin, gen_frame({mask => 0, fin => 1, opcode => OPCODE_BINARY, data => $payload}), "frame ok");
};

subtest 'big server2client frame' => sub {
    my $payload = "preved!" x 10000; # must be > 65536
    my $bin = $p->send_frame(1, $payload);
    is(length($bin), 70010, "frame length ok"); # 2 header + 8 length + 70000 payload
    is_bin($bin, gen_frame({mask => 0, fin => 1, opcode => OPCODE_BINARY, data => $payload}), "frame ok");
};

$p = WSTest::get_established_client();

subtest 'small client2server frame' => sub {
    my $payload = "preved"; # must be <= 125
    my $bin = $p->send_frame(1, $payload, OPCODE_TEXT);
    is(length($bin), 12, "frame length ok"); # 2 header + 4 mask + 6 payload
    is_bin($bin, gen_frame({mask => substr($bin, 2, 4), fin => 1, opcode => OPCODE_TEXT, data => $payload}), "frame ok");
};

subtest 'medium client2server frame' => sub {
    my $payload = "preved" x 100; # must be > 125
    my $bin = $p->send_frame(1, $payload, OPCODE_TEXT);
    is(length($bin), 608, "frame length ok"); # 2 header + 2 length + 4 mask + 600 payload
    is_bin($bin, gen_frame({mask => substr($bin, 4, 4), fin => 1, opcode => OPCODE_TEXT, data => $payload}), "frame ok");
};

subtest 'big client2server frame' => sub {
    my $payload = "preved!" x 10000; # must be > 65536
    my $bin = $p->send_frame(1, $payload, OPCODE_TEXT);
    is(length($bin), 70014, "frame length ok"); # 2 header + 8 length + 4 mask + 70000 payload
    is_bin($bin, gen_frame({mask => substr($bin, 10, 4), fin => 1, opcode => OPCODE_TEXT, data => $payload}), "frame ok");
};

subtest 'empty frame still masked' => sub {
    my $bin = $p->send_frame(1, "");
    is(length($bin), 6, "frame length ok"); # 2 header + 4 mask
    is_bin($bin, gen_frame({mask => substr($bin, 2, 4), fin => 1, opcode => OPCODE_BINARY}), "frame ok");
};

$p = WSTest::get_established_server();

subtest 'opcode CONTINUE is forced for fragment frames of message (including final frame)' => sub {
    my $bin = $p->send_frame(0, "frame1");
    is_bin($bin, gen_frame({fin => 0, opcode => OPCODE_BINARY, data => "frame1"}), "initial frame ok");
    $bin = $p->send_frame(0, "frame2", OPCODE_TEXT);
    is_bin($bin, gen_frame({fin => 0, opcode => OPCODE_CONTINUE, data => "frame2"}), "fragment frame ok");
    $bin = $p->send_frame(1, "frame3", OPCODE_TEXT);
    is_bin($bin, gen_frame({fin => 1, opcode => OPCODE_CONTINUE, data => "frame3"}), "final frame ok");
    
    $bin = $p->send_frame(0, "frame4", OPCODE_TEXT);
    is_bin($bin, gen_frame({fin => 0, opcode => OPCODE_TEXT, data => "frame4"}), "first frame of next message ok");
    $p->send_frame(1, "frame5"); # reset frame count
};

subtest 'control frame send' => sub {
    my $bin = $p->send_frame(1, "myping", OPCODE_PING);
    is_bin($bin, gen_frame({fin => 1, opcode => OPCODE_PING, data => "myping"}), "ping ok");
    $bin = $p->send_frame(1, "mypong", OPCODE_PONG);
    is_bin($bin, gen_frame({fin => 1, opcode => OPCODE_PONG, data => "mypong"}), "pong ok");
    $bin = $p->send_frame(1, "myclose", OPCODE_CLOSE);
    is_bin($bin, gen_frame({fin => 1, opcode => OPCODE_CLOSE, data => "myclose"}), "close ok");
    WSTest::reset($p);
};

subtest 'frame count survives control message in the middle' => sub {
    my $bin = $p->send_frame(0, "frame1");
    is_bin($bin, gen_frame({fin => 0, opcode => OPCODE_BINARY, data => "frame1"}), "initial frame ok");
    $bin = $p->send_frame(1, "", OPCODE_PING);
    is_bin($bin, gen_frame({fin => 1, opcode => OPCODE_PING}), "control frame ok");
    $bin = $p->send_frame(0, "frame2");
    is_bin($bin, gen_frame({fin => 0, opcode => OPCODE_CONTINUE, data => "frame2"}), "fragment frame ok");
    $bin = $p->send_frame(1, "", OPCODE_PONG);
    is_bin($bin, gen_frame({fin => 1, opcode => OPCODE_PONG}), "control frame ok");
    $bin = $p->send_frame(1, "frame3");
    is_bin($bin, gen_frame({fin => 1, opcode => OPCODE_CONTINUE, data => "frame3"}), "final frame ok");
};

subtest 'control frame cannot be sent being non-final' => sub {
    ok(!eval { $p->send_frame(0, "", OPCODE_PING) }, "exception thrown");
    WSTest::reset($p);
};


done_testing();

sub is_bin {
    my ($got, $expected, $name) = @_;
    return if our $leak_test;
    $has_binary ? is_binary($got, $expected, $name) : is($got, $expected, $name);
}