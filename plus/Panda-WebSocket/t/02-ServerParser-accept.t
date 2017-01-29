use 5.020;
use warnings;
use lib 't/lib'; use WSTest;

my ($p, $creq, $data, $last);

$p = new Panda::WebSocket::ServerParser;
ok($p, "parser created");
ok(!$p->accepted, "not accepted");
ok(!$p->established, "not established");

# accept chunks
my @data = WSTest::accept_packet();
$last = pop @data;
foreach my $line (@data) {
    is($p->accept($line), undef, "no full data");
}
$creq = $p->accept($last);
ok($creq, "accept done");
cmp_deeply($creq, WSTest::accept_parsed(), "request correct");
ok($p->accepted, "now accepted");
ok(!$p->established, "still not established");

$p->reset();
ok(!$p->accepted, "not accepted after reset");

# accept all
$data = WSTest::accept_packet();
cmp_deeply($p->accept($data), WSTest::accept_parsed(), "request correct");
ok($p->accepted, "now accepted");

$p->reset();

# accept with body
@data = WSTest::accept_packet();
splice(@data, 1, 0, "Content-Length: 1\r\n");
$creq = $p->accept($_) for @data;
ok($creq && $creq->error, "body disallowed");
ok(!$p->accepted, "not accepted");

$p->reset();

# max_accept_size
@data = WSTest::accept_packet();
$last = pop @data;
my $big  = ("header: value\r\n" x 1000);

$p->accept($_) for @data;
$p->accept($big);
$creq = $p->accept($last);
ok($creq, "default unlimited buffer");
ok(!$creq->error, "default unlimited buffer");
is($creq->header('header'), "value", "default unlimited buffer");

$p->reset();

$p->max_accept_size(10000);
$p->accept($_) for @data;
$creq = $p->accept($big);
ok($creq, "buffer limit exceeded");
like($creq->error, qr/exceeded/, "buffer limit exceeded");

$p->reset();

done_testing();