use 5.020;
use warnings;
use lib 't/lib'; use WSTest;

my ($p, $creq, $data, $ans);

$p = new Panda::WebSocket::ServerParser;

# successful response
$data = WSTest::accept_packet();
$p->accept($data);
ok($p->accepted, "accepted");
$ans = $p->accept_response();
like($ans, qr/^HTTP\/1\.1 101 Switching Protocols\r\n/, "accept answer ok");
like($ans, qr/^Upgrade: websocket\r\n/m, "accept answer ok");
like($ans, qr/^Connection: Upgrade\r\n/m, "accept answer ok");
like($ans, qr/^Sec-WebSocket-Protocol: chat\r\n/m, "accept answer ok");
unlike($ans, qr/^Sec-WebSocket-Extensions/, "no extensions now");
like($ans, qr/^Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK\+xOo=\r\n/m, "accept answer ok");
ok($p->established, "connection established");

$p->reset();

# successful response with args
$data = WSTest::accept_packet();
$p->accept($data);
ok($p->accepted, "accepted");
$ans = $p->accept_response({
    ws_protocol   => "jopa",
    ws_extensions => [["ext1"], ["ext2", {arg1 => 1}], ["ext3"]],
    headers       => {h1 => 1},
});
like($ans, qr/^HTTP\/1\.1 101 Switching Protocols\r\n/, "accept answer ok");
like($ans, qr/^Sec-WebSocket-Protocol: jopa\r\n/m, "accept answer ok");
like($ans, qr/^h1: 1\r\n/m, "accept answer ok");
unlike($ans, qr/^Sec-WebSocket-Extensions/, "unsupported extensions removed");

$p->reset();

done_testing();
