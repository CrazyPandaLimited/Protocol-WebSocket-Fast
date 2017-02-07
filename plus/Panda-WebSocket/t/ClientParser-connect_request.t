use 5.020;
use warnings;
use lib 't/lib'; use WSTest;

subtest 'parser create' => sub {
    my $p = new Panda::WebSocket::ClientParser;
    ok($p, "parser created");
    ok(!$p->established, "not established");
};

subtest 'default request' => sub {
    my $p = new Panda::WebSocket::ClientParser;
    my $req = {
        uri           => Panda::URI->new("ws://crazypanda.ru:4321/path?a=b"),
        ws_protocol   => 'fuck',
        ws_extensions => [ [ 'permessage-deflate', { 'client_max_window_bits' => '' } ] ],
        ws_version    => 12,
        headers       => {
            'Accept-Encoding' => 'gzip, deflate, sdch',
            'Origin'          => 'http://www.crazypanda.ru',
            'Cache-Control'   => 'no-cache',
            'User-Agent'      => 'PWS-Test',
        },
    };
    my $str = $p->connect_request($req);
    like($str, qr/^GET \/path\?a=b HTTP\/1.1\r\n/, "request line ok");
    like($str, qr/^Sec-Websocket-Protocol: fuck\r\n/im, "protocol ok");
    like($str, qr/^Sec-Websocket-Version: 12\r\n/im, "version ok");
    like($str, qr/^Sec-Websocket-Key: (.+)\r\n/im, "key ok");
    like($str, qr/^Sec-Websocket-Extensions: permessage-deflate; client_max_window_bits\r\n/im, "extensions ok");
    like($str, qr/^Connection: Upgrade\r\n/im, "connection ok");
    like($str, qr/^Upgrade: websocket\r\n/im, "upgrade ok");
    like($str, qr/^Origin: http:\/\/www.crazypanda.ru\r\n/im, "headers ok");
    like($str, qr/^Accept-Encoding: gzip, deflate, sdch\r\n/im, "headers ok");
    like($str, qr/^Cache-Control: no-cache\r\n/im, "headers ok");
    like($str, qr/^User-Agent: PWS-Test\r\n/im, "headers ok");
    like($str, qr/^Host: crazypanda.ru\r\n/im, "host ok");

    ok(!$p->established, "still not established");
    
    ok(!eval { $p->connect_request($req); 1 }, "already requested connection");
};

subtest 'default values' => sub {
    my $p = new Panda::WebSocket::ClientParser;
    my $str = $p->connect_request({uri => "ws://crazypanda.ru:4321/path?a=b"});
    like($str, qr/^GET \/path\?a=b HTTP\/1.1\r\n/, "request line ok");
    like($str, qr/^Sec-Websocket-Version: 13\r\n/im, "version ok");
    unlike($str, qr/^Sec-Websocket-Protocol: /im, "protocol ok");
    unlike($str, qr/^Sec-Websocket-Extensions: /im, "extensions ok");
};

subtest 'custom ws_key' => sub {
    my $p = new Panda::WebSocket::ClientParser;
    my $str = $p->connect_request({uri => "ws://crazypanda.ru:4321/path?a=b", ws_key => "suka"});
    like($str, qr/^Sec-Websocket-Key: suka\r\n/im, "key ok");
};

subtest 'empty relative url' => sub {
    my $p = new Panda::WebSocket::ClientParser;
    my $str = $p->connect_request({uri => "wss://crazypanda.ru"});
    like($str, qr/^GET \/ HTTP\/1.1\r\n/, "request line ok");
};

subtest 'server parser accepts connect request' => sub {
    my $p = new Panda::WebSocket::ClientParser;
    my $sp = new Panda::WebSocket::ServerParser;
    my $str = $p->connect_request({uri => "ws://crazypanda.ru/path?a=b"});
    my $req = $sp->accept($str);
    ok($sp->accepted, "request accepted");
    ok($req, "request returned");
    is($req->error, undef, "no errors");
    is($req->ws_version, 13, "version ok");
    is($req->uri, '/path?a=b', "uri ok");
};

subtest 'no host in uri' => sub {
    my $p = new Panda::WebSocket::ClientParser;
    ok(!eval { $p->connect_request({uri => "ws:path?a=b"}); 1}, "exception thrown: $@");
};

subtest 'wrong scheme' => sub {
    my $p = new Panda::WebSocket::ClientParser;
    ok(!eval { $p->connect_request({uri => "wsss://dev.ru/"}); 1}, "exception thrown: $@");
};

subtest 'body is not allowed' => sub {
    my $p = new Panda::WebSocket::ClientParser;
    ok(!eval { $p->connect_request({uri => "ws://dev.ru/", body => "hello world"}); 1}, "exception thrown: $@");
};

done_testing();