use 5.020;
use warnings;
use lib 't/lib'; use WSTest;

my ($p, $bin, $frame, @frames, $it);

# not established
$p = new Panda::WebSocket::ServerParser;
ok(!eval { $p->get_frames('asdasd'); }, "cant get frames until established");

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
test_frame("unmasked frame", {opcode => OPCODE_TEXT, mask => 0, fin => 1, data => "jopa"}, "frame is not masked");

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

# TWO (next)
$bin = gen_frame({opcode => OPCODE_TEXT, mask => 1, fin => 1, data => "jopa1"}).
       gen_frame({opcode => OPCODE_TEXT, mask => 1, fin => 1, data => "jopa2"});
$it = $p->get_frames($bin);
my ($first, $second) = ($it->next, $it->next);
ok($first && $second && !$it->next, "[two] two frames returned");
cmp_deeply([$first->error, $second->error], [undef, undef], "[two] no errors");
cmp_deeply([$first, $second], [methods(final => 1, payload => "jopa1"), methods(final => 1, payload => "jopa2")], "[two] data ok");

# THREE (all)
$bin = gen_frame({opcode => OPCODE_TEXT, mask => 1, fin => 1, data => "jopa1"}).
       gen_frame({opcode => OPCODE_TEXT, mask => 1, fin => 1, data => "jopa2"}).
       gen_frame({opcode => OPCODE_TEXT, mask => 1, fin => 1, data => "jopa3"});
@frames = $p->get_frames($bin);
is(scalar(@frames), 3, "[three] three frames returned");
cmp_deeply([map {$_->error} @frames], [undef, undef, undef], "[three] no errors");
cmp_deeply(\@frames, [methods(payload => "jopa1"), methods(payload => "jopa2"), methods(payload => "jopa3")], "[three] data ok");

# 2.5 + 1.5
my $tmp = gen_frame({opcode => OPCODE_TEXT, mask => 1, fin => 1, data => "jopa3"});
$bin = gen_frame({opcode => OPCODE_TEXT, mask => 1, fin => 1, data => "jopa1"}).
       gen_frame({opcode => OPCODE_TEXT, mask => 1, fin => 1, data => "jopa2"}).
       substr($tmp, 0, length($tmp)-1, '');
@frames = $p->get_frames($bin);
is(scalar(@frames), 2, "[2.5+1.5] 2 returned");
cmp_deeply(\@frames, [methods(payload => "jopa1"), methods(payload => "jopa2")], "[2.5+1.5] data ok");
$bin = $tmp.gen_frame({opcode => OPCODE_TEXT, mask => 1, fin => 1, data => "jopa4"});
@frames = $p->get_frames($bin);
is(scalar(@frames), 2, "[2.5+1.5] 2 more returned");
cmp_deeply(\@frames, [methods(payload => "jopa3"), methods(payload => "jopa4")], "[2.5+1.5] 2 more data ok");

sub test_frame {
    my ($name, $frame_data, $error) = @_;
    my $bin = gen_frame($frame_data);
    my $check_data = {};
    $check_data->{opcode}         = $frame_data->{opcode};
    $check_data->{is_control}     = ($check_data->{opcode} >= OPCODE_CLOSE);
    $check_data->{final}          = $frame_data->{fin} || '';
    $check_data->{payload_length} = length($frame_data->{data}//'');
    $check_data->{payload}        = $frame_data->{data};
    $check_data->{close_code}     = $frame_data->{close_code} if exists $frame_data->{close_code};
    
    # check whole data
    my @frames = $p->get_frames($bin);
    my ($frame) = @frames;
    ok(scalar(@frames) == 1 && $frame, "[$name] one frame returned");
    if ($error) {
        cmp_deeply($frame->error, $error, "[$name] frame parsing error: $error");
        WSTest::reset_established_server($p);
    } else {
        is($frame->error, undef, "[$name] no errors");
        cmp_deeply($frame, methods(%$check_data), "[$name] frame properties ok");
    }
    
    $it = $frame = undef;
    
    # check by char
    while (length($bin) && !$it) { $it = $p->get_frames(substr($bin, 0, 1, '')); }
    $frame = $it->next;
    ok($frame && !$bin && !$it->next, "[$name(chunked)] got frame on last char") unless $error;
    
    if ($error) {
        cmp_deeply($frame->error, $error, "[$name(chunked)] frame parsing error: $error");
        WSTest::reset_established_server($p);
    } else {
        is($frame->error, undef, "[$name(chunked)] no errors");
        cmp_deeply($frame, methods(%$check_data), "[$name(chunked)] frame properties ok");
    }
}


done_testing();

