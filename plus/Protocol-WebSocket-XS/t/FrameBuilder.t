use 5.020;
use warnings;
use lib 't'; use MyTest;
use Encode::Base2N qw/encode_base64pad/;
use Test::Fatal;

*gen_frame = \&MyTest::gen_frame;

{
     no warnings 'redefine';
     *MyTest::accept_packet = sub {
         my @data = (
             "GET /?encoding=text HTTP/1.1\r\n",
             "Host: dev.crazypanda.ru:4680\r\n",
             "Connection: Upgrade\r\n",
             "Pragma: no-cache\r\n",
             "Cache-Control: no-cache\r\n",
             "Upgrade: websocket\r\n",
             "Origin: http://www.websocket.org\r\n",
             "Sec-WebSocket-Version: 13\r\n",
             "User-Agent: Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/55.0.2883.87 Safari/537.36\r\n",
             "Accept-Encoding: gzip, deflate, sdch\r\n",
             "Accept-Language: ru-RU,ru;q=0.8,en-US;q=0.6,en;q=0.4\r\n",
             "Cookie: _ga=GA1.2.1700804447.1456741171\r\n",
             "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n",
             "Sec-WebSocket-Extensions: permessage-deflate\r\n",
             "\r\n",
         );
         return wantarray ? @data : join('', @data);
    };
};

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
        my $b = $server->start_message({ final => 1, deflate => 0, opcode => OPCODE_TEXT});
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
    my $b = MyTest::get_established_server()->start_message({opcode => OPCODE_BINARY});
    is $b->opcode, OPCODE_BINARY;
    $b->send("hello");

    $b->opcode, OPCODE_CONTINUE;
    $b->send("world");

    $b->final(1);
    $b->opcode, OPCODE_CONTINUE;
    $b->send("!");
};

done_testing;
