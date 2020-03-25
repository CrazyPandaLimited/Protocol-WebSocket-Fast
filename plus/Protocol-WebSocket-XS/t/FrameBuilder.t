use 5.012;
use warnings;
use lib 't'; use MyTest;
use Encode::Base2N qw/encode_base64pad/;
use Test::Fatal;

subtest 'settings' => sub {
    subtest "defaults" => sub {
        my $server = MyTest::get_established_server();
        my $b = $server->start_message;
        ok !$b->final;
        ok $b->deflate;
        is $b->opcode, OPCODE_BINARY;
    };

    subtest "user defined constants" => sub {
        my $server = MyTest::get_established_server();
        my $b = $server->start_message(final => 1, deflate => 0, opcode => OPCODE_TEXT);
        ok $b->final;
        ok !$b->deflate;
        is $b->opcode, OPCODE_TEXT;
    };
};

subtest "attempt to send frame after sending final frame" => sub {
    my $server = MyTest::get_established_server();
    my $b = $server->start_message;
    my $bin = $b->final(1)->send('payload');
    ok $bin;
    like( exception { $b->send('beyond payload') }, qr/messsage is already finished/);
    like( exception { $b->final(0)->send('beyond payload') }, qr/messsage is already finished/);
};

subtest "attempt to start another message, having unfinished one" => sub {
    my $server = MyTest::get_established_server();
    $server->start_message->send("hello");
    like( exception { $server->start_message->send('world') }, qr/previous message wasn't finished/);
};

subtest "opcode change for multi-frame messge" => sub {
    my $b = MyTest::get_established_server()->start_message(opcode => OPCODE_BINARY);
    is $b->opcode, OPCODE_BINARY;
    $b->send("hello");

    $b->opcode, OPCODE_CONTINUE;
    $b->send("world");

    $b->final(1);
    $b->opcode, OPCODE_CONTINUE;
    $b->send("!");
};

done_testing;
