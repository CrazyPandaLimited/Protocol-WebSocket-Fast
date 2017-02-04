use 5.020;
use warnings;
use lib 't/lib'; use WSTest;

my ($p, $message, $bin, $it, @messages, @bin);

# not established
$p = new Panda::WebSocket::ServerParser;
ok(!eval { $p->get_messages('asdasd'); }, "cant get messages until established");

$p = WSTest::get_established_server();

# single frame
test_message("single", {mask => 1, data => "hello world", nframes => 1});

# split into 2 frames
test_message("2 frames", {mask => 1, data => "hello world", nframes => 2});

# many frames
test_message("many frames", {mask => 1, data => ("suchka hey" x 100), nframes => 49});

# PING
test_message("empty ping", {opcode => OPCODE_PING, mask => 1, fin => 1});
test_message("ping with payload", {opcode => OPCODE_PING, mask => 1, fin => 1, data => "pingdata"});
test_message("fragmented ping", {opcode => OPCODE_PING, mask => 1, fin => 0}, "control frame can't be fragmented");
test_message("long ping", {opcode => OPCODE_PING, mask => 1, fin => 1, data => ("1" x 1000)}, "control frame payload is too big");

# PONG
test_message("empty pong", {opcode => OPCODE_PONG, mask => 1, fin => 1});
test_message("pong with payload", {opcode => OPCODE_PONG, mask => 1, fin => 1, data => "pongdata"});
test_message("fragmented pong", {opcode => OPCODE_PONG, mask => 1, fin => 0}, "control frame can't be fragmented");
test_message("long pong", {opcode => OPCODE_PONG, mask => 1, fin => 1, data => ("1" x 1000)}, "control frame payload is too big");

# CLOSE
test_message("empty close", {opcode => OPCODE_CLOSE, mask => 1, fin => 1, close_code => all(CLOSE_UNKNOWN)});
test_message("close with code", {opcode => OPCODE_CLOSE, mask => 1, fin => 1, close_code => CLOSE_NORMAL});
test_message("close with code&msg", {opcode => OPCODE_CLOSE, mask => 1, fin => 1, close_code => CLOSE_AWAY, data => "walk"});
test_message("close with invalid payload", {opcode => OPCODE_CLOSE, mask => 1, fin => 1, data => "a"}, "control frame CLOSE contains invalid data");
test_message("long close", {opcode => OPCODE_CLOSE, mask => 1, fin => 1, close_code => CLOSE_AWAY, data => ("1" x 1000)}, "control frame payload is too big");

# max message size
$p->max_frame_size(2000);
$p->max_message_size(1000);
test_message("under max message size", {opcode => OPCODE_TEXT, mask => 1, fin => 1, data => ("1" x 1000)});
test_message("above max message size", {opcode => OPCODE_TEXT, mask => 1, fin => 1, data => ("1" x 1001)}, "max message size exceeded");
$p->max_frame_size(0);
$p->max_message_size(0);

# server parser does not accept messages with unmasked frames
$message = test_message("unmasked message", {opcode => OPCODE_TEXT, mask => 0, data => "jopa noviy god", nframes => 2}, "frame is not masked");
is($message->frame_count, 0, "error caught on first frame and rest is dropped, error frame is not counted");

# empty message
test_message("empty", {mask => 1});

# TWO (next)
$bin = gen_message({mask => 1, data => "jopa1", nframes => 1}).
       gen_message({mask => 1, data => "jopa2", nframes => 2});
$it = $p->get_messages($bin);
my ($first, $second) = ($it->next, $it->next);
ok($first && $second && !$it->next, "[two] two messages returned");
cmp_deeply([$first->error, $second->error], [undef, undef], "[two] no errors");
cmp_deeply([$first, $second], [methods(frame_count => 1, payload => "jopa1"), methods(frame_count => 2, payload => "jopa2")], "[two] data ok");

# THREE (all)
$bin = gen_message({mask => 1, data => "jopa1", nframes => 1}).
       gen_message({mask => 1, data => "jopa2", nframes => 2}).
       gen_message({mask => 1, data => "jopa3", nframes => 3});
@messages = $p->get_messages($bin);
ok(scalar(@messages) == 3, "[three] three messages returned");
cmp_deeply([map {$_->error} @messages], [undef, undef, undef], "[three] no errors");
cmp_deeply(\@messages, [
    methods(frame_count => 1, payload => "jopa1"),
    methods(frame_count => 2, payload => "jopa2"),
    methods(frame_count => 3, payload => "jopa3"),
], "[three] data ok");

# control frame in the middle of multi-frame message
@bin = gen_message({mask => 1, data => "4k displays are quite kewl", nframes => 4});
splice(@bin, 2, 0, gen_frame({opcode => OPCODE_PING, mask => 1, fin => 1}));
@messages = $p->get_messages(join('', @bin));
is(scalar(@messages), 2, "[control-in-the-middle] messages returned");
cmp_deeply($messages[0], methods(frame_count => 1, opcode => OPCODE_PING), "[control-in-the-middle] control message first");
cmp_deeply($messages[1], methods(frame_count => 4, payload => "4k displays are quite kewl"), "[control-in-the-middle] regular message second");

# the same one-by-one frame
$it = $p->get_messages($bin[0]);
is($it, undef, "[control-in-the-middle(by frame)] not yet");
$it = $p->get_messages($bin[1]);
is($it, undef, "[control-in-the-middle(by frame)] not yet");
$it = $p->get_messages($bin[2]);
$message = $it->next;
cmp_deeply($message, methods(frame_count => 1, opcode => OPCODE_PING), "[control-in-the-middle(by frame)] control message arrived");
is($it->next, undef, "[control-in-the-middle(by frame)] nothing more yet");
$it = $p->get_messages($bin[3]);
is($it, undef, "[control-in-the-middle(by frame)] still not yet");
$it = $p->get_messages($bin[4]);
$message = $it->next;
cmp_deeply($message, methods(frame_count => 4, payload => "4k displays are quite kewl"), "[control-in-the-middle(by frame)] regular message arrived");
is($it->next, undef, "[control-in-the-middle(by frame)] and nothing more");

# 2.5 + 1.5 + control
my @first  = gen_message({mask => 1, data => "first message",  nframes => 1});
my @second = gen_message({mask => 1, data => "second message", nframes => 2});
my @third  = gen_message({mask => 1, data => "third message",  nframes => 3});
my @fourth = gen_message({mask => 1, data => "fourth message", nframes => 4});
my $stolen = substr($third[2], -1, 1, '');
my $pong   = gen_frame({opcode => OPCODE_PONG, mask => 1, fin => 1});
@messages = $p->get_messages(join('', @first, @second, @third));
is(scalar(@messages), 2, "[2.5+1.5+C] 2 arrived");
cmp_deeply(\@messages, [
    methods(payload => "first message", frame_count => 1),
    methods(payload => "second message", frame_count => 2),
], "[2.5+1.5+C] first two data ok");
@messages = $p->get_messages($stolen.$fourth[0].$fourth[1].$fourth[2].$pong.$fourth[3]);
is(scalar(@messages), 3, "[2.5+1.5+C] 3 more arrived");
cmp_deeply(\@messages, [
    methods(payload => "third message", frame_count => 3),
    methods(opcode  => OPCODE_PONG, is_control => 1),
    methods(payload => "fourth message", frame_count => 4),
], "[2.5+1.5+C] last three data ok, pong is between third and fourth");

sub test_message {
    my ($name, $message_data, $error) = @_;
    my $opcode = $message_data->{opcode} // OPCODE_TEXT;
    my $nframes = $message_data->{nframes} //= 1;
    
    my $check_data = {};
    $check_data->{opcode}         = $opcode;
    $check_data->{is_control}     = ($opcode >= OPCODE_CLOSE); 
    $check_data->{payload_length} = length($message_data->{data}//'');
    $check_data->{payload}        = $message_data->{data};
    $check_data->{close_code}     = $message_data->{close_code} if exists $message_data->{close_code};
    $check_data->{frame_count}    = $nframes;

    my $bin;
    if ($opcode && $opcode != OPCODE_TEXT && $opcode != OPCODE_BINARY) {
        $bin = WSTest::gen_frame($message_data);
    } else {
        $bin = gen_message($message_data);
    }
    
    # check whole data
    my @messages = $p->get_messages($bin);
    my ($message) = @messages;
    ok(scalar(@messages) == 1 && $message, "[$name] one message returned");
    if ($error) {
        cmp_deeply($message->error, $error, "[$name] message parsing error: $error");
        WSTest::reset_established_server($p);
    } else {
        is($message->error, undef, "[$name] no errors");
        cmp_deeply($message, methods(%$check_data), "[$name] message properties ok");
    }
    
    $it = $message = undef;
    
    # check by char
    while (length($bin) && !$it) { $it = $p->get_messages(substr($bin, 0, 1, '')); }
    $message = $it->next;
    ok($message && !$bin && !$it->next, "[$name(chunked)] got message on last char") unless $error;
    
    if ($error) {
        cmp_deeply($message->error, $error, "[$name(chunked)] message parsing error: $error");
        WSTest::reset_established_server($p);
    } else {
        is($message->error, undef, "[$name(chunked)] no errors");
        cmp_deeply($message, methods(%$check_data), "[$name(chunked)] message properties ok");
    }
    return $message;
}


done_testing();

