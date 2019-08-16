use 5.012;
use warnings;
use lib 't'; use MyTest;

my $p = new Protocol::WebSocket::XS::ServerParser;

*accept_packet = \&MyTest::accept_packet;

subtest 'bad http' => sub {
    my @data = accept_packet();
    $data[0] =~ s/GET/POST/;
    my $creq;
    $creq = $p->accept($_) for @data;
    ok($creq, "request returned");
    ok($creq->error, "error present");
    is($creq->header('connection'), "Upgrade", "parsed");
    ok($p->accept_parsed, "accept parsed");
    ok(!$p->accepted, "not accepted");
    ok(!$p->established, "not established");
    my $ans = $p->accept_error;
    like($ans, qr/^HTTP\/1\.1 400 Bad Request\r\n/, "answer ok");
    $p->reset();
};

subtest 'bad websocket http' => sub {
    my $data = accept_packet();
    $data =~ s/Upgrade: websocket\r\n/Upgrade: fuckoff\r\n/;
    my $creq = $p->accept($data);
    ok($creq, "request returned");
    ok($creq->error, "error present");
    is($creq->header('connection'), "Upgrade", "parsed");
    ok($p->accept_parsed, "accept parsed");
    ok(!$p->accepted, "not accepted");
    my $ans = $p->accept_error;
    like($ans, qr/^HTTP\/1\.1 400 Bad Request\r\n/, "answer ok");
    $p->reset();
};

subtest 'bad websocket version' => sub {
    my $data = accept_packet();
    $data =~ s/(Sec-WebSocket-Version:) \d+\r\n/$1 999\r\n/;
    my $creq = $p->accept($data);
    ok($creq, "request returned");
    ok($creq->error, "error present");
    is($creq->header('connection'), "Upgrade", "parsed");
    ok($p->accept_parsed, "accept parsed");
    ok(!$p->accepted, "not accepted");
    my $ans = $p->accept_error;
    like($ans, qr/^HTTP\/1\.1 426 Upgrade Required\r\n/, "answer ok");
    like($ans, qr/Sec-WebSocket-Version: 13\r\n/, "version ok");
    $p->reset();
};

subtest 'custom error override ignored when request error' => sub {
    my $data = accept_packet();
    $data =~ s/Upgrade: websocket\r\n/Upgrade: fuckoff\r\n/;
    $p->accept($data);
    ok($p->accept_parsed, "accept parsed");
    ok(!$p->accepted, "not accepted");
    my $ans = $p->accept_error({code => 404});
    like($ans, qr/^HTTP\/1\.1 400 Bad Request\r\n/, "answer ok");
    $p->reset();
};

subtest 'custom error' => sub {
    my $data = accept_packet();
    $p->accept($data);
    ok($p->accept_parsed, "accept parsed");
    ok($p->accepted, "accepted");
    my $ans = $p->accept_error({
        code    => 404,
        message => "Hello World",
        headers => {abc => 1, def => 2},
    });
    like($ans, qr/^HTTP\/1\.1 404 Hello World\r\n/, "answer ok");
    like($ans, qr/^abc: 1\r\n/m, "answer ok");
    like($ans, qr/^def: 2\r\n/m, "answer ok");
    like($ans, qr/^404 Hello World$/m, "answer ok");
    $p->reset();
};

subtest 'custom error with body' => sub {
    my $data = accept_packet();
    $p->accept($data);
    ok($p->accept_parsed, "accept parsed");
    ok($p->accepted, "accepted");
    my $ans = $p->accept_error({
        code    => 404,
        message => "Hello World",
        body    => "Fuck You",
    });
    like($ans, qr/^HTTP\/1\.1 404 Hello World\r\n/, "answer ok");
    like($ans, qr/^Fuck You$/m, "answer ok");
    $p->reset();
};

done_testing();
