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
    my $server = Protocol::WebSocket::XS::ServerParser->new;

    $configure->($client, $server) if $configure;

    my $str = $client->connect_request($req);
    my $creq = $server->accept($str) or die "should not happen";
    my $res_str = $creq->error ? $server->accept_error : $server->accept_response;

    my $server_deflate = $client->deflate_config && $server->deflate_config;
    like $str, qr/permessage-deflate/ if($client->deflate_config);
    like $res_str, qr/permessage-deflate/ if($server->deflate_config);
    ok $server->is_deflate_active if ($server_deflate);

    $client->connect($res_str);
    ok $client->established;
    ok $client->is_deflate_active if ($server_deflate);

    return ($client, $server);
};

subtest 'empty payload frame' => sub {
    my $payload = "";
    my ($c, $s) = $create_pair->();
    my $bin = $s->start_message({final => 1, deflate => 1})->send($payload);
    my ($f) = $c->get_frames($bin);
    ok !$f->payload;
};

subtest 'tiny payload' => sub {
    my $payload = "preved";
    my ($c, $s) = $create_pair->();
    my $bin = $s->start_message({final => 1, deflate => 1})->send($payload);
    my ($f) = $c->get_frames($bin);
    is $f->payload, $payload;
};


subtest 'medium payload' => sub {
    my @payload = ('0') x (1923);
    my $payload = join('', @payload);
    my ($c, $s) = $create_pair->();
    my $bin = $s->start_message({final => 1, deflate => 1})->send($payload);
    my ($f) = $c->get_frames($bin);
    is $f->payload, $payload;
};

subtest 'medium payload (fragmented)' => sub {
    my @payload = ('0') x (1923);
    my $payload = join('', @payload);
    my ($c, $s) = $create_pair->();
    my $bin = $s->start_message({final => 1, deflate => 1})->send_av(\@payload);
    my ($f) = $c->get_frames($bin);
    is $f->payload, $payload;
};

subtest 'large payload' => sub {
    my @payload = ('0') x (1024 * 1024);
    my $payload = join('', @payload);
    my ($c, $s) = $create_pair->();
    my $bin = $s->start_message({final => 1, deflate => 1})->send($payload);
    my ($f) = $c->get_frames($bin);
    is $f->payload, $payload;
};

subtest '1-frame-message (tiny payload)' => sub {
    my $payload = "hello-world";
    my ($c, $s) = $create_pair->();
    my $bin = $s->start_message({final => 1, deflate => 1})->send($payload);
    my ($m) = $c->get_messages($bin);
    ok $m;
    is $m->payload, $payload;

};

subtest 'message, 2 frames, context_takeover = true, tiny payload' => sub {
    my @payload = ('a', 'b');
    my $payload = join('', @payload, @payload);
    my ($c, $s) = $create_pair->();
    my $builder = $s->start_message({deflate => 1});
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
    my $builder = $s->start_message({deflate => 1});
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
        $c->configure({deflate => {server_no_context_takeover => 1}});
    });
    my $bin1 = $s->start_message({deflate => 1})->final(1)->send_av(\@payload);
    my $bin2 = $s->start_message({deflate => 1})->final(1)->send_av(\@payload);
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
        $c->configure({deflate => {client_no_context_takeover => 1}});
    });
    my $bin1 = $c->start_message({deflate => 1})->final(1)->send_av(\@payload);
    my $bin2 = $c->start_message({deflate => 1})->final(1)->send_av(\@payload);
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
        $c->configure({deflate => {
            client_no_context_takeover => 1,
            server_no_context_takeover => 1,
            client_max_window_bits     => 10,
            server_max_window_bits     => 11,
            compression_level          => 1,
        }});
    });
    my $bin1 = $c->start_message({deflate => 1})->final(1)->send_av(\@payload);
    my $bin2 = $c->start_message({deflate => 1})->final(1)->send_av(\@payload);
    note "l1 = ", length($bin1), ", l2 = ", length($bin2);
    is length($bin1), length($bin2), "make sure there is no context takeover";
    my @m = $s->get_messages($bin1 . $bin2);
    is scalar(@m), 2;
    my ($m1, $m2) = @m;
    is $m1->payload, $payload;
    is $m2->payload, $payload;
};

subtest "multiframe message" => sub {
    my $payloads = [qw/first second third/];
    my ($c, $s) = $create_pair->();
    my $bin = $c->send_message_multiframe({deflate => 1, payload => $payloads});
    my ($m) = $s->get_messages($bin);
    is $m->payload, join('', @$payloads);
};


subtest "multiframe message (with empty pieces)" => sub {
    my $payloads = ['', 'hello', ''];
    my ($c, $s) = $create_pair->();
    my $bin = $c->send_message_multiframe({deflate => 1, payload => $payloads});
    my ($m) = $s->get_messages($bin);
    is $m->payload, join('', @$payloads);
};

subtest "multiframe message (empty)" => sub {
    my $payloads = ['', '', ''];
    my ($c, $s) = $create_pair->();
    my $bin = $c->send_message_multiframe({deflate => 1, payload => $payloads});
    my ($m) = $s->get_messages($bin);
    ok $m;
    ok !$m->payload;
};

subtest 'corrupted frame' => sub {
    my @payload = ('0') x (1923);
    my $payload = join('', @payload);
    my ($c, $s) = $create_pair->();
    my $bin             = $s->start_message({final => 1, deflate => 1})->send($payload);
    my $deflate_payload = substr($bin, 2);
    my $forged_bin      = substr($bin, 0, 2) . "xx" . substr($deflate_payload, 2);
    my ($f) = $c->get_frames($forged_bin);
    like $f->error, qr/zlib::inflate error/;
};

subtest 'corrupted 2nd frame from 3' => sub {
    my @payload = ('0') x (1923);
    my $payload = join('', @payload, @payload, @payload);
    my ($c, $s) = $create_pair->();
    my $builder = $s->start_message({deflate => 1});
    my $bin_1   = $builder->send_av(\@payload);
    my $bin_2   = $builder->send_av(\@payload);
    my $bin_3   = $builder->final(1)->send_av(\@payload);

    my $bin_2_payload = substr($bin_2, 2);
    my $bin_2_forged  = substr($bin_2, 0, 2) . substr($bin_2_payload, 0, 2) . 'xx' . substr($bin_2_payload, 4);
    my ($m) = $c->get_messages($bin_1 . $bin_2_forged . $bin_3);
    ok $m;
    like $m->error, qr/zlib::inflate error/;
};

subtest "compression threshold" => sub {
    my ($c, $s) = $create_pair->(sub {
        my ($c, $s) = @_;
        $s->configure({deflate => { compression_threshold => 5 }});
    });
    my $payload_1 = "1234";
    my $bin_1 = $s->send_message({payload => $payload_1});
    note $bin_1;
    is( substr($bin_1, 2), $payload_1);

    my $payload_2 = "12345";
    my $bin_2 = $s->send_message({payload => $payload_2, opcode => OPCODE_BINARY});
    note $bin_2;
    is( substr($bin_2, 2), $payload_2, "binary payload isn't compressed by default");

    my $bin_3 = $s->send_message({payload => $payload_2, opcode => OPCODE_TEXT});
    note $bin_3;
    isnt( substr($bin_3, 2), $payload_2, "text payload is compressed by default");
};

subtest "no_deflate" => sub {
    my ($c, $s) = $create_pair->(sub {
        my ($c, $s) = @_;
        $s->no_deflate;
    });
    my $payload = "1234";

    my $bin = $s->send_message({payload => $payload});
    note $bin;
    is( substr($bin, 2), $payload);
};


done_testing;
