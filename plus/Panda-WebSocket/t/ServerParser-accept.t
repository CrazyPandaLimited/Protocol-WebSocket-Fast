use 5.020;
use warnings;
use lib 't/lib'; use WSTest;

*accept_packet = \&WSTest::accept_packet;
*accept_parsed = \&WSTest::accept_parsed;

my $p = new Panda::WebSocket::ServerParser;

subtest 'parser create' => sub {
    ok($p, "parser created");
    ok(!$p->accepted, "not accepted");
    ok(!$p->established, "not established");
};

subtest 'accept chunks' => sub {
    my @data = accept_packet();
    my $last = pop @data;
    foreach my $line (@data) {
        is($p->accept($line), undef, "no full data");
    }
    my $creq = $p->accept($last);
    ok($creq, "accept done");
    cmp_deeply($creq, accept_parsed(), "request correct");
    ok($p->accepted, "now accepted");
    ok(!$p->established, "still not established");
};

$p->reset();
ok(!$p->accepted, "not accepted after reset");

subtest 'accept all' => sub {
    my $data = accept_packet();
    cmp_deeply($p->accept($data), accept_parsed(), "request correct");
    ok($p->accepted, "now accepted");
};

$p->reset();

subtest 'accept with body' => sub {
    my @data = accept_packet();
    splice(@data, 1, 0, "Content-Length: 1\r\n");
    my $creq;
    $creq = $p->accept($_) for @data;
    ok($creq && $creq->error, "body disallowed");
    ok(!$p->accepted, "not accepted");
};

$p->reset();

subtest 'max_handshake_size' => sub {
    my @data = accept_packet();
    my $last = pop @data;
    my $big  = ("header: value\r\n" x 100);
    $p->accept($_) for @data;
    $p->accept($big);
    my $creq = $p->accept($last);
    ok($creq, "default unlimited buffer");
    ok(!$creq->error, "default unlimited buffer");
    is($creq->header('header'), "value", "default unlimited buffer");
    
    $p->reset();
    
    $p->max_handshake_size(1000);
    $p->accept($_) for @data;
    $creq = $p->accept($big);
    ok($creq, "buffer limit exceeded");
    like($creq->error, qr/exceeded/, "buffer limit exceeded");
};

$p->reset();

done_testing();