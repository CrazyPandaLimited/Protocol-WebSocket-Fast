use 5.020;
use warnings;
use lib 't/lib'; use WSTest;

*gen_frame = \&WSTest::gen_frame;
*gen_message = \&WSTest::gen_message;

ok(!eval { Panda::WebSocket::ServerParser->new->get_messages('asdasd'); }, "cant get messages until established");

my $p = WSTest::get_established_server();

subtest 'single frame' => \&test_message, {mask => 1, data => "hello world", nframes => 1};
subtest '2 frames'     => \&test_message, {mask => 1, data => "hello world", nframes => 2};
subtest 'many frames'  => \&test_message, {mask => 1, data => ("suchka hey" x 100), nframes => 49};
subtest 'empty'        => \&test_message, {mask => 1};

subtest 'ping' => sub {
    subtest 'empty'      => \&test_message, {opcode => OPCODE_PING, mask => 1, fin => 1};
    subtest 'payload'    => \&test_message, {opcode => OPCODE_PING, mask => 1, fin => 1, data => "pingdata"};
    subtest 'fragmented' => \&test_message, {opcode => OPCODE_PING, mask => 1, fin => 0}, "control frame can't be fragmented";
    subtest 'long'       => \&test_message, {opcode => OPCODE_PING, mask => 1, fin => 1, data => ("1" x 1000)}, "control frame payload is too big";
};

subtest 'pong' => sub {
    subtest 'empty'      => \&test_message, {opcode => OPCODE_PONG, mask => 1, fin => 1};
    subtest 'payload'    => \&test_message, {opcode => OPCODE_PONG, mask => 1, fin => 1, data => "pongdata"};
    subtest 'fragmented' => \&test_message, {opcode => OPCODE_PONG, mask => 1, fin => 0}, "control frame can't be fragmented";
    subtest 'long'       => \&test_message, {opcode => OPCODE_PONG, mask => 1, fin => 1, data => ("1" x 1000)}, "control frame payload is too big";
};

subtest 'close' => sub {
    subtest 'empty'           => \&test_message, {opcode => OPCODE_CLOSE, mask => 1, fin => 1, close_code => all(CLOSE_UNKNOWN)};
    subtest 'code'            => \&test_message, {opcode => OPCODE_CLOSE, mask => 1, fin => 1, close_code => CLOSE_NORMAL};
    subtest 'message'         => \&test_message, {opcode => OPCODE_CLOSE, mask => 1, fin => 1, close_code => CLOSE_AWAY, data => "walk"};
    subtest 'invalid payload' => \&test_message, {opcode => OPCODE_CLOSE, mask => 1, fin => 1, data => "a"}, "control frame CLOSE contains invalid data";
    subtest 'fragmented'      => \&test_message, {opcode => OPCODE_CLOSE, mask => 1, fin => 0}, "control frame can't be fragmented";
    subtest 'long'            => \&test_message, {opcode => OPCODE_CLOSE, mask => 1, fin => 1, close_code => CLOSE_AWAY, data => ("1" x 1000)}, "control frame payload is too big";
};

subtest 'max message size' => sub {
    $p->max_frame_size(2000);
    $p->max_message_size(1000);
    subtest 'allowed' => \&test_message, {opcode => OPCODE_TEXT, mask => 1, fin => 1, data => ("1" x 1000)};
    subtest 'exceeds' => \&test_message, {opcode => OPCODE_TEXT, mask => 1, fin => 1, data => ("1" x 1001)}, "max message size exceeded";
    $p->max_frame_size(0);
    $p->max_message_size(0);
};

subtest '2 messages via it->next' => sub {
    my $bin = gen_message({mask => 1, data => "jopa1", nframes => 1}).
              gen_message({mask => 1, data => "jopa2", nframes => 2});
    my $it = $p->get_messages($bin);
    my ($first, $second) = ($it->next, $it->next);
    ok($first && $second && !$it->next, "2 messages returned");
    cmp_deeply([$first->error, $second->error], [undef, undef], "no errors");
    cmp_deeply([$first, $second], [methods(frame_count => 1, payload => "jopa1"), methods(frame_count => 2, payload => "jopa2")], "data ok");
};

subtest '3 messages via list context' => sub {
    my $bin = gen_message({mask => 1, data => "jopa1", nframes => 1}).
              gen_message({mask => 1, data => "jopa2", nframes => 2}).
              gen_message({mask => 1, data => "jopa3", nframes => 3});
    my @messages = $p->get_messages($bin);
    ok(scalar(@messages) == 3, "3 messages returned");
    cmp_deeply([map {$_->error} @messages], [undef, undef, undef], "no errors");
    cmp_deeply(\@messages, [
        methods(frame_count => 1, payload => "jopa1"),
        methods(frame_count => 2, payload => "jopa2"),
        methods(frame_count => 3, payload => "jopa3"),
    ], "data ok");
};

subtest 'control frame in the middle of multi-frame message' => sub {
    my @bin = gen_message({mask => 1, data => "you are kewl", nframes => 4});
    splice(@bin, 2, 0, gen_frame({opcode => OPCODE_PING, mask => 1, fin => 1}));
    my @messages = $p->get_messages(join('', @bin));
    is(scalar(@messages), 2, "messages returned");
    cmp_deeply($messages[0], methods(frame_count => 1, opcode => OPCODE_PING), "control message first");
    cmp_deeply($messages[1], methods(frame_count => 4, payload => "you are kewl"), "regular message second");
};

subtest 'the same one-by-one frame' => sub {
    my @bin = gen_message({mask => 1, data => "you are bad", nframes => 4});
    splice(@bin, 2, 0, gen_frame({opcode => OPCODE_PONG, mask => 1, fin => 1}));
    my $it = $p->get_messages($bin[0]);
    is($it, undef, "not yet");
    $it = $p->get_messages($bin[1]);
    is($it, undef, "and not yet");
    $it = $p->get_messages($bin[2]);
    my $message = $it->next;
    cmp_deeply($message, methods(frame_count => 1, opcode => OPCODE_PONG), "control message arrived");
    is($it->next, undef, "nothing more yet");
    $it = $p->get_messages($bin[3]);
    is($it, undef, "still not yet");
    $it = $p->get_messages($bin[4]);
    $message = $it->next;
    cmp_deeply($message, methods(frame_count => 4, payload => "you are bad"), "regular message arrived");
    is($it->next, undef, "and nothing more");
};

subtest '2.5 messages + 1.5 messages + control message' => sub {
    my @first  = gen_message({mask => 1, data => "first message",  nframes => 1});
    my @second = gen_message({mask => 1, data => "second message", nframes => 2});
    my @third  = gen_message({mask => 1, data => "third message",  nframes => 3});
    my @fourth = gen_message({mask => 1, data => "fourth message", nframes => 4});
    my $stolen = substr($third[2], -1, 1, '');
    my $pong   = gen_frame({opcode => OPCODE_PONG, mask => 1, fin => 1});
    my @messages = $p->get_messages(join('', @first, @second, @third));
    is(scalar(@messages), 2, "2 arrived");
    cmp_deeply(\@messages, [
        methods(payload => "first message", frame_count => 1),
        methods(payload => "second message", frame_count => 2),
    ], "first 2 data ok");
    @messages = $p->get_messages($stolen.$fourth[0].$fourth[1].$fourth[2].$pong.$fourth[3]);
    is(scalar(@messages), 3, "3 more arrived");
    cmp_deeply(\@messages, [
        methods(payload => "third message", frame_count => 3),
        methods(opcode  => OPCODE_PONG, is_control => 1),
        methods(payload => "fourth message", frame_count => 4),
    ], "last 3 data ok, pong is between 3rd and 4th");
};

subtest 'first frame in message with CONTINUE' => \&test_message, {opcode => OPCODE_CONTINUE, mask => 1, fin => 1, data => 'jopa'}, "initial frame can't have opcode CONTINUE";

subtest 'fragment frame in message without CONTINUE' => sub {
    my $bin = gen_frame({opcode => OPCODE_TEXT, mask => 1, fin => 0, data => 'p1'}).
              gen_frame({opcode => OPCODE_TEXT, mask => 1, fin => 1, data => 'p2'});
    my ($message) = $p->get_messages($bin);
    is($message->error, "fragment frame must have opcode CONTINUE", "error ok");
    $bin = gen_frame({opcode => OPCODE_BINARY, mask => 1, fin => 0, data => 'p1'}).
           gen_frame({opcode => OPCODE_BINARY, mask => 1, fin => 0, data => 'p2'});
    ($message) = $p->get_messages($bin);
    is($message->error, "fragment frame must have opcode CONTINUE", "uncompleted does not matter");
};

subtest 'message with unmasked frame in server parser' => sub {
    my $message = test_message({opcode => OPCODE_TEXT, mask => 0, data => "jopa noviy god", nframes => 2}, "frame is not masked");
    is($message->frame_count, 0, "error caught on first frame and rest is dropped, error frame is not counted");
};

$p = WSTest::get_established_client();
subtest 'message with masked frame in client parser'   => \&test_message, {mask => 1, data => "jopa"};
subtest 'message with unmasked frame in client parser' => \&test_message, {mask => 0, data => "jopa"};
$p = WSTest::get_established_server();

sub test_message {
    my ($message_data, $error) = @_;
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
    if ($opcode >= OPCODE_CLOSE) {
        $bin = WSTest::gen_frame($message_data);
    } else {
        $bin = gen_message($message_data);
    }
    
    subtest 'whole buffer' => sub {
        my @messages = $p->get_messages($bin);
        my ($message) = @messages;
        ok(scalar(@messages) == 1 && $message, "one message returned");
        if ($error) {
            cmp_deeply($message->error, $error, "message parsing error: $error");
            WSTest::reset($p);
        } else {
            is($message->error, undef, "no errors");
            cmp_deeply($message, methods(%$check_data), "message properties ok");
        }
    };
    
    my $message;
    subtest 'buffer by char' => sub {
        my $it;
        while (length($bin) && !$it) { $it = $p->get_messages(substr($bin, 0, 1, '')); }
        $message = $it->next;
        ok($message && !$bin && !$it->next, "got message on last char") unless $error;
        
        if ($error) {
            cmp_deeply($message->error, $error, "message parsing error: $error");
            WSTest::reset($p);
        } else {
            is($message->error, undef, "no errors");
            cmp_deeply($message, methods(%$check_data), "message properties ok");
        }
    };
    
    return $message;
}

done_testing();