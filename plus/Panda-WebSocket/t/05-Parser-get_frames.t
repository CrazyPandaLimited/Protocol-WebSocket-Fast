use 5.020;
use warnings;
use lib 't/lib'; use WSTest;

my ($p, $binfr, $frame, @frames);

$p = WSTest::get_established_server();

# small frame
test_frame("small", {opcode => OPCODE_BINARY, mask => 1, fin => 1, data => "hello world"});

# medium size frame
test_frame("medium", {opcode => OPCODE_CONTINUE, mask => 1, fin => 0, data => ("1" x 1024)});

# big size frame
test_frame("big", {opcode => OPCODE_TEXT, mask => 1, fin => 1, data => ("1" x 100000)});

# empty frame
test_frame("empty", {opcode => OPCODE_TEXT, mask => 1, fin => 1});

# max frame size
$p->max_frame_size(1000);
test_frame("under max frame size", {opcode => OPCODE_TEXT, mask => 1, fin => 1, data => ("1" x 1000)});
test_frame("above max frame size", {opcode => OPCODE_TEXT, mask => 1, fin => 1, data => ("1" x 1001)}, "max frame size exceeded");
$p->max_frame_size(0);

# server parser does not accept unmasked frames
test_frame("unmasked frame", {opcode => OPCODE_TEXT, mask => 0, fin => 1, data => "jopa"}, "client frame MUST be masked");

# PING
test_frame("empty ping", {opcode => OPCODE_PING, mask => 1, fin => 1});
test_frame("ping with payload", {opcode => OPCODE_PING, mask => 1, fin => 1, data => "pingdata"});
test_frame("fragmented ping", {opcode => OPCODE_PING, mask => 1, fin => 0}, "control frame can't be fragmented");
test_frame("long ping", {opcode => OPCODE_PING, mask => 1, fin => 1, data => ("1" x 1000)}, "control frame payload is too big");

# PONG
test_frame("empty pong", {opcode => OPCODE_PONG, mask => 1, fin => 1});
test_frame("pong with payload", {opcode => OPCODE_PONG, mask => 1, fin => 1, data => "pongdata"});
test_frame("fragmented pong", {opcode => OPCODE_PONG, mask => 1, fin => 0}, "control frame can't be fragmented");
test_frame("long pong", {opcode => OPCODE_PONG, mask => 1, fin => 1, data => ("1" x 1000)}, "control frame payload is too big");

# CLOSE
test_frame("empty close", {opcode => OPCODE_CLOSE, mask => 1, fin => 1, close_code => all(CLOSE_UNKNOWN)});
test_frame("close with code", {opcode => OPCODE_CLOSE, mask => 1, fin => 1, close_code => CLOSE_NORMAL});
test_frame("close with code&msg", {opcode => OPCODE_CLOSE, mask => 1, fin => 1, close_code => CLOSE_AWAY, data => "walk"});
test_frame("close with invalid payload", {opcode => OPCODE_CLOSE, mask => 1, fin => 1, data => "a"}, "control frame CLOSE contains invalid data");
test_frame("long close", {opcode => OPCODE_CLOSE, mask => 1, fin => 1, close_code => CLOSE_AWAY, data => ("1" x 1000)}, "control frame payload is too big");

sub test_frame {
    my ($name, $frame_data, $error) = @_;
    my $binfr = WSTest::gen_frame($frame_data);
    my $check_data = {};
    $check_data->{opcode}         = $frame_data->{opcode};
    $check_data->{final}          = $frame_data->{fin} || '';
    $check_data->{payload_length} = length($frame_data->{data}//'');
    $check_data->{payload}        = $frame_data->{data};
    $check_data->{close_code}     = $frame_data->{close_code} if exists $frame_data->{close_code};
    
    # check whole data
    my @frames = $p->get_frames($binfr);
    is(scalar(@frames), 1, "[$name] one frame returned");
    my ($frame) = @frames;
    ok($frame, "[$name] frame present");
    
    if ($error) {
        cmp_deeply($frame->error, $error, "[$name] frame parsing error: $error");
        WSTest::reset_established_server($p);
    } else {
        is($frame->error, undef, "[$name] no errors");
        cmp_deeply($frame, methods(%$check_data), "[$name] frame properties ok");
    }
    
    undef $frame;
    
    # check by char
    while ($binfr && !$frame) { $frame = $p->get_frames(substr($binfr, 0, 1, '')); }
    ok($frame && !$binfr, "[$name(chunked)] got frame on last char") unless $error;
    
    if ($error) {
        cmp_deeply($frame->error, $error, "[$name(chunked)] frame parsing error: $error");
        WSTest::reset_established_server($p);
    } else {
        is($frame->error, undef, "[$name(chunked)] no errors");
        cmp_deeply($frame, methods(%$check_data), "[$name(chunked)] frame properties ok");
    }
}


done_testing();

