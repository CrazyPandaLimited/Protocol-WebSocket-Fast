use 5.020;
use warnings;
use lib 't'; use MyTest;

my $create_pair = sub {
    my $configure = shift;
    my $req = {
        uri    => "ws://crazypanda.ru",
        ws_key => "dGhlIHNhbXBsZSBub25jZQ==",
    };

    local $Test::Builder::Level = $Test::Builder::Level + 1;
    my $client = Protocol::WebSocket::XS::ClientParser->new;
    #$client->use_deflate({server_no_context_takeover => 1});
    $client->use_deflate;

    my $server = Protocol::WebSocket::XS::ServerParser->new;
    $server->use_deflate;
    $configure->($client, $server) if $configure;

    my $str = $client->connect_request($req);
    my $creq = $server->accept($str) or die "should not happen";
    my $res_str = $creq->error ? $server->accept_error : $server->accept_response;
    like $str, qr/permessage-deflate/;
    like $res_str, qr/permessage-deflate/;
    ok $server->is_deflate_active;

    $client->connect($res_str);
    ok $client->established;
    ok $client->is_deflate_active;

    return ($client, $server);
};

subtest 'empty payload frame' => sub {
    my $payload = "";
    my ($c, $s) = $create_pair->();
    my $bin = $s->start_message({final => 1})->send($payload);
    my ($f) = $c->get_frames($bin);
    ok !$f->payload;
};

subtest 'tiny payload' => sub {
    my $payload = "preved";
    my ($c, $s) = $create_pair->();
    my $bin = $s->start_message({final => 1})->send($payload);
    my ($f) = $c->get_frames($bin);
    is $f->payload, $payload;
};


subtest 'medium payload' => sub {
    my @payload = ('0') x (1923);
    my $payload = join('', @payload);
    my ($c, $s) = $create_pair->();
    my $bin = $s->start_message({final => 1})->send($payload);
    my ($f) = $c->get_frames($bin);
    is $f->payload, $payload;
};

subtest 'medium payload (fragmented)' => sub {
    my @payload = ('0') x (1923);
    my $payload = join('', @payload);
    my ($c, $s) = $create_pair->();
    my $bin = $s->start_message({final => 1})->send_av(\@payload);
    my ($f) = $c->get_frames($bin);
    is $f->payload, $payload;
};

subtest 'large payload' => sub {
    my @payload = ('0') x (1024 * 1024);
    my $payload = join('', @payload);
    my ($c, $s) = $create_pair->();
    my $bin = $s->start_message({final => 1})->send($payload);
    my ($f) = $c->get_frames($bin);
    is $f->payload, $payload;
};

subtest '1-frame-message (tiny payload)' => sub {
    my $payload = "hello-world";
    my ($c, $s) = $create_pair->();
    my $bin = $s->start_message({final => 1})->send($payload);
    my ($m) = $c->get_messages($bin);
    ok $m;
    is $m->payload, $payload;

};

subtest 'message, 2 frames, context_takeover = true, tiny payload' => sub {
    my @payload = ('a', 'b');
    my $payload = join('', @payload, @payload);
    my ($c, $s) = $create_pair->();
    my $builder = $s->start_message;
    my $bin1 = $builder->send_av(\@payload);
    my $bin2 = $builder->final(1)->send_av(\@payload);
    my ($m) = $c->get_messages($bin1 . $bin2);
    ok $m;
    is $m->payload, $payload;
};

subtest 'message, 2 frames, context_takeover = true, medium payload' => sub {
    my @payload = ('0') x (1024);
    my $payload = join('', @payload, @payload);
    my ($c, $s) = $create_pair->();
    my $builder = $s->start_message;
    my $bin1 = $builder->send_av(\@payload);
    my $bin2 = $builder->final(1)->send_av(\@payload);
    note "l1 = ", length($bin1), ", l2 = ", length($bin2);
    my ($m) = $c->get_messages($bin1 . $bin2);
    ok $m;
    is $m->payload, $payload;
};


subtest '2 messages, 2 frames, server_context_takeover = false, medium payload' => sub {
    my @payload = ('0') x (1024);
    my $payload = join('', @payload);
    my ($c, $s) = $create_pair->(sub {
        my ($c, $s) = @_;
        $c->use_deflate({server_no_context_takeover => 1});
    });
    my $bin1 = $s->start_message->final(1)->send_av(\@payload);
    my $bin2 = $s->start_message->final(1)->send_av(\@payload);
    note "l1 = ", length($bin1), ", l2 = ", length($bin2);
    is length($bin1), length($bin2), "make sure there is no context takeover";
    my @m = $c->get_messages($bin1 . $bin2);
    is scalar(@m), 2;
    my ($m1, $m2) = @m;
    is $m1->payload, $payload;
    is $m2->payload, $payload;
};

subtest '2 messages, 2 frames, client_context_takeover = false, medium payload' => sub {
    my @payload = ('0') x (1024);
    my $payload = join('', @payload);
    my ($c, $s) = $create_pair->(sub {
        my ($c, $s) = @_;
        $c->use_deflate({client_no_context_takeover => 1});
    });
    my $bin1 = $c->start_message->final(1)->send_av(\@payload);
    my $bin2 = $c->start_message->final(1)->send_av(\@payload);
    note "l1 = ", length($bin1), ", l2 = ", length($bin2);
    is length($bin1), length($bin2), "make sure there is no context takeover";
    my @m = $s->get_messages($bin1 . $bin2);
    is scalar(@m), 2;
    my ($m1, $m2) = @m;
    is $m1->payload, $payload;
    is $m2->payload, $payload;
};

subtest '2 messages, 2 frames, server_context_takeover = false = client_context_takeover = false, medium payload, custom windows' => sub {
    my @payload = ('0') x (1024 * 10);
    my $payload = join('', @payload);
    my ($c, $s) = $create_pair->(sub {
        my ($c, $s) = @_;
        $c->use_deflate({
            client_no_context_takeover => 1,
            server_no_context_takeover => 1,
            client_max_window_bits     => 10,
            server_max_window_bits     => 11,
            compression_level          => 1,
        });
    });
    my $bin1 = $c->start_message->final(1)->send_av(\@payload);
    my $bin2 = $c->start_message->final(1)->send_av(\@payload);
    note "l1 = ", length($bin1), ", l2 = ", length($bin2);
    is length($bin1), length($bin2), "make sure there is no context takeover";
    my @m = $s->get_messages($bin1 . $bin2);
    is scalar(@m), 2;
    my ($m1, $m2) = @m;
    is $m1->payload, $payload;
    is $m2->payload, $payload;
};


done_testing;
