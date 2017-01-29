use 5.020;
use warnings;
use lib 't/lib'; use WSTest;

my ($p, $creq, $data, $ans);

$p = new Panda::WebSocket::ServerParser;

# bad http (not parsed)
my @data = WSTest::accept_packet();
$data[0] =~ s/GET/POST/;
$creq = $p->accept($_) for @data;
ok($creq, "accept with error done");
ok($creq->error, "accept error present");
is($creq->header('connection'), "Upgrade", "http parsed");
ok(!$p->accepted, "not accepted");
ok(!$p->established, "not established");
$ans = $p->accept_error;
like($ans, qr/^HTTP\/1\.1 400 Bad Request\r\n/, "error answer from request error ok");

# no need to reset because parser auto-resets on accept_error

# bad websocket http (parsed)
$data = WSTest::accept_packet();
$data =~ s/Upgrade: websocket\r\n/Upgrade: fuckoff\r\n/;
$creq = $p->accept($data);
ok($creq, "accept with error done");
ok($creq->error, "accept error present");
is($creq->header('connection'), "Upgrade", "http parsed");
ok(!$p->accepted, "not accepted");
$ans = $p->accept_error;
like($ans, qr/^HTTP\/1\.1 400 Bad Request\r\n/, "error answer from request error ok");

# no need to reset because parser auto-resets on accept_error

# bad websocket version (parsed)
$data = WSTest::accept_packet();
$data =~ s/(Sec-WebSocket-Version:) \d+\r\n/$1 999\r\n/;
$creq = $p->accept($data);
ok($creq, "accept with error done");
ok($creq->error, "accept error present");
is($creq->header('connection'), "Upgrade", "http parsed");
ok(!$p->accepted, "not accepted");
$ans = $p->accept_error;
like($ans, qr/^HTTP\/1\.1 426 Upgrade Required\r\n/, "error answer from request version error ok");
like($ans, qr/Sec-WebSocket-Version: 13\r\n/, "error answer from request version error ok");

# custom error override if request error
$data = WSTest::accept_packet();
$data =~ s/Upgrade: websocket\r\n/Upgrade: fuckoff\r\n/;
$p->accept($data);
ok(!$p->accepted, "not accepted");
$ans = $p->accept_error({code => 404});
like($ans, qr/^HTTP\/1\.1 400 Bad Request\r\n/, "error answer ignores http response if request error");

# no need to reset because parser auto-resets on accept_error

# custom error
$data = WSTest::accept_packet();
$p->accept($data);
ok($p->accepted, "accepted");
$ans = $p->accept_error({
    code    => 404,
    message => "Hello World",
    headers => {abc => 1, def => 2},
});
like($ans, qr/^HTTP\/1\.1 404 Hello World\r\n/, "custom error answer ok");
like($ans, qr/^abc: 1\r\n/m, "custom error answer ok");
like($ans, qr/^def: 2\r\n/m, "custom error answer ok");
like($ans, qr/^404 Hello World$/m, "custom error answer ok");

# no need to reset because parser auto-resets on accept_error

# custom error with body
$data = WSTest::accept_packet();
$p->accept($data);
ok($p->accepted, "accepted");
$ans = $p->accept_error({
    code    => 404,
    message => "Hello World",
    body    => "Fuck You",
});
like($ans, qr/^HTTP\/1\.1 404 Hello World\r\n/, "custom error answer ok");
like($ans, qr/^Fuck You$/m, "custom error answer ok");

# no need to reset because parser auto-resets on accept_error

done_testing();
