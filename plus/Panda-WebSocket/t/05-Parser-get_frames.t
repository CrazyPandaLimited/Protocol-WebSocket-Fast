use 5.020;
use warnings;
use lib 't/lib'; use WSTest;

*gen_frame = \&WSTest::gen_frame;

my $p = WSTest::get_established_server();

ok(!eval { Panda::WebSocket::ServerParser->new->get_frames('asdasd'); }, "cant get frames until established");

subtest 'small frame'  => \&test_frame, {opcode => OPCODE_BINARY, mask => 1, fin => 1, data => "hello world"};
subtest 'medium frame' => \&test_frame, {opcode => OPCODE_BINARY, mask => 1, fin => 1, data => ("1" x 1024)};
subtest 'big frame'    => \&test_frame, {opcode => OPCODE_TEXT,   mask => 1, fin => 1, data => ("1" x 100000)};
subtest 'empty frame'  => \&test_frame, {opcode => OPCODE_TEXT,   mask => 1, fin => 1};

subtest 'max frame size' => sub {
	$p->max_frame_size(1000);
	subtest 'allowed' => \&test_frame, {opcode => OPCODE_TEXT, mask => 1, fin => 1, data => ("1" x 1000)};
	subtest 'exceeds' => \&test_frame, {opcode => OPCODE_TEXT, mask => 1, fin => 1, data => ("1" x 1001)}, "max frame size exceeded";
	$p->max_frame_size(0);
};

subtest 'unmasked frame' => \&test_frame, {opcode => OPCODE_TEXT, mask => 0, fin => 1, data => "jopa"}, "frame is not masked";

subtest 'ping' => sub {
	subtest 'empty'      => \&test_frame, {opcode => OPCODE_PING, mask => 1, fin => 1};
	subtest 'payload'    => \&test_frame, {opcode => OPCODE_PING, mask => 1, fin => 1, data => "pingdata"};
	subtest 'fragmented' => \&test_frame, {opcode => OPCODE_PING, mask => 1, fin => 0}, "control frame can't be fragmented";
	subtest 'long'       => \&test_frame, {opcode => OPCODE_PING, mask => 1, fin => 1, data => ("1" x 1000)}, "control frame payload is too big";
};

subtest 'pong' => sub {
	subtest 'empty'      => \&test_frame, {opcode => OPCODE_PONG, mask => 1, fin => 1};
	subtest 'payload'    => \&test_frame, {opcode => OPCODE_PONG, mask => 1, fin => 1, data => "pongdata"};
	subtest 'fragmented' => \&test_frame, {opcode => OPCODE_PONG, mask => 1, fin => 0}, "control frame can't be fragmented";
	subtest 'long'       => \&test_frame, {opcode => OPCODE_PONG, mask => 1, fin => 1, data => ("1" x 1000)}, "control frame payload is too big";
};

subtest 'close' => sub {
	subtest 'empty'           => \&test_frame, {opcode => OPCODE_CLOSE, mask => 1, fin => 1, close_code => all(CLOSE_UNKNOWN)};
	subtest 'code'            => \&test_frame, {opcode => OPCODE_CLOSE, mask => 1, fin => 1, close_code => CLOSE_NORMAL};
	subtest 'message'         => \&test_frame, {opcode => OPCODE_CLOSE, mask => 1, fin => 1, close_code => CLOSE_AWAY, data => "walk"};
	subtest 'invalid payload' => \&test_frame, {opcode => OPCODE_CLOSE, mask => 1, fin => 1, data => "a"}, "control frame CLOSE contains invalid data";
    subtest 'fragmented'      => \&test_frame, {opcode => OPCODE_CLOSE, mask => 1, fin => 0}, "control frame can't be fragmented";
	subtest 'long'            => \&test_frame, {opcode => OPCODE_CLOSE, mask => 1, fin => 1, close_code => CLOSE_AWAY, data => ("1" x 1000)}, "control frame payload is too big";
};

subtest '2 frames via it->next' => sub {
	my $bin = gen_frame({opcode => OPCODE_TEXT, mask => 1, fin => 1, data => "jopa1"}).
	          gen_frame({opcode => OPCODE_TEXT, mask => 1, fin => 1, data => "jopa2"});
	my $it = $p->get_frames($bin);
	my ($first, $second) = ($it->next, $it->next);
	ok($first && $second && !$it->next, "2 frames returned");
	cmp_deeply([$first->error, $second->error], [undef, undef], "no errors");
	cmp_deeply([$first, $second], [methods(final => 1, payload => "jopa1"), methods(final => 1, payload => "jopa2")], "data ok");
};

subtest '3 frames via list context' => sub {
	my $bin = gen_frame({opcode => OPCODE_TEXT, mask => 1, fin => 1, data => "jopa1"}).
	          gen_frame({opcode => OPCODE_TEXT, mask => 1, fin => 1, data => "jopa2"}).
	          gen_frame({opcode => OPCODE_TEXT, mask => 1, fin => 1, data => "jopa3"});
	my @frames = $p->get_frames($bin);
	is(scalar(@frames), 3, "3 frames returned");
	cmp_deeply([map {$_->error} @frames], [undef, undef, undef], "no errors");
	cmp_deeply(\@frames, [methods(payload => "jopa1"), methods(payload => "jopa2"), methods(payload => "jopa3")], "data ok");
};

subtest '2.5 frames + 1.5 frames' => sub {
	my $tmp = gen_frame({opcode => OPCODE_TEXT, mask => 1, fin => 1, data => "jopa3"});
	my $bin = gen_frame({opcode => OPCODE_TEXT, mask => 1, fin => 1, data => "jopa1"}).
	          gen_frame({opcode => OPCODE_TEXT, mask => 1, fin => 1, data => "jopa2"}).
	          substr($tmp, 0, length($tmp)-1, '');
	my @frames = $p->get_frames($bin);
	is(scalar(@frames), 2, "2 returned");
	cmp_deeply(\@frames, [methods(payload => "jopa1"), methods(payload => "jopa2")], "data ok");
	$bin = $tmp.gen_frame({opcode => OPCODE_TEXT, mask => 1, fin => 1, data => "jopa4"});
	@frames = $p->get_frames($bin);
	is(scalar(@frames), 2, "2 more returned");
	cmp_deeply(\@frames, [methods(payload => "jopa3"), methods(payload => "jopa4")], "2 more data ok");
};

subtest 'initial frame in message with CONTINUE' => \&test_frame, {opcode => OPCODE_CONTINUE, mask => 1, fin => 1, data => 'jopa'}, "initial frame can't have opcode CONTINUE";

subtest 'fragment frame in message without CONTINUE' => sub {
	my $bin = gen_frame({opcode => OPCODE_TEXT, mask => 1, fin => 0, data => 'p1'}).
	          gen_frame({opcode => OPCODE_TEXT, mask => 1, fin => 0, data => 'p2'});
	my ($first, $second) = $p->get_frames($bin);
	is($first->error, undef, "initial frame ok");
	is($first->payload, 'p1', "initial data ok");
	is($second->error, "fragment frame must have opcode CONTINUE", "fragment frame error ok");
    $bin = gen_frame({opcode => OPCODE_BINARY, mask => 1, fin => 0, data => 'p1'}).
           gen_frame({opcode => OPCODE_BINARY, mask => 1, fin => 1, data => 'p2'});
    ($first, $second) = $p->get_frames($bin);
    is($second->error, "fragment frame must have opcode CONTINUE", "fin does not matter");
};

sub test_frame {
    my ($frame_data, $error) = @_;
    my $bin = gen_frame($frame_data);
    my $check_data = {};
    $check_data->{opcode}         = $frame_data->{opcode};
    $check_data->{is_control}     = ($check_data->{opcode} >= OPCODE_CLOSE);
    $check_data->{final}          = $frame_data->{fin} || '';
    $check_data->{payload_length} = length($frame_data->{data}//'');
    $check_data->{payload}        = $frame_data->{data};
    $check_data->{close_code}     = $frame_data->{close_code} if exists $frame_data->{close_code};
    
    subtest 'whole buffer' => sub {
	    my @frames = $p->get_frames($bin);
	    my ($frame) = @frames;
	    ok(scalar(@frames) == 1 && $frame, "one frame returned");
	    if ($error) {
	        cmp_deeply($frame->error, $error, "frame parsing error: $error");
	        WSTest::reset_established_server($p);
	    } else {
	        is($frame->error, undef, "no errors");
	        cmp_deeply($frame, methods(%$check_data), "frame properties ok");
	    }
    };
        
    subtest 'buffer by char' => sub {
    	my $it;
	    while (length($bin) && !$it) { $it = $p->get_frames(substr($bin, 0, 1, '')); }
	    my $frame = $it->next;
	    ok($frame && !$bin && !$it->next, "got frame on last char") unless $error;
	    
	    if ($error) {
	        cmp_deeply($frame->error, $error, "frame parsing error: $error");
	        WSTest::reset_established_server($p);
	    } else {
	        is($frame->error, undef, "no errors");
	        cmp_deeply($frame, methods(%$check_data), "frame properties ok");
	    }
    }
}


done_testing();

