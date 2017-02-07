use 5.020;
use warnings;
use lib 't/lib'; use WSTest;

*gen_frame   = \&WSTest::gen_frame;
*gen_message = \&WSTest::gen_message;

my $p = WSTest::get_established_client();

ok(!eval { Panda::WebSocket::ClientParser->new->get_frames('asdasd'); }, "cant get frames until established");

subtest 'accepts masked' => sub {
    subtest 'frames' => sub {
        my $bin = gen_frame({opcode => OPCODE_TEXT, mask => 1, fin => 1, data => "jopa1"});
        my ($f) = $p->get_frames($bin);
        cmp_deeply($f, methods(final => 1, payload => "jopa1"), "data ok");
    };
    subtest 'messages' => sub {
        my $bin = gen_message({mask => 1, data => "jopa1", nframes => 2});
        my ($m) = $p->get_messages($bin);
        cmp_deeply($m, methods(frame_count => 2, payload => "jopa1"), "data ok");
    };
};

subtest 'accepts unmasked' => sub {
    subtest 'frames' => sub {
        my $bin = gen_frame({opcode => OPCODE_TEXT, mask => 0, fin => 1, data => "jopa1"});
        my ($f) = $p->get_frames($bin);
        cmp_deeply($f, methods(final => 1, payload => "jopa1"), "data ok");
    };
    subtest 'messages' => sub {
        my $bin = gen_message({mask => 0, data => "jopa1", nframes => 2});
        my ($m) = $p->get_messages($bin);
        cmp_deeply($m, methods(frame_count => 2, payload => "jopa1"), "data ok");
    };
};

# everything else has been tested in ServerParser tests

done_testing();