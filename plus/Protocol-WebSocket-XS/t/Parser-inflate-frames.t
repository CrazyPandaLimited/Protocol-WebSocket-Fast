use 5.020;
use warnings;
use lib 't'; use MyTest;

my $create_pair = sub {
    my $req = {
        uri    => "ws://crazypanda.ru",
        ws_key => "dGhlIHNhbXBsZSBub25jZQ==",
    };

    local $Test::Builder::Level = $Test::Builder::Level + 1;
    my $client = Protocol::WebSocket::XS::ClientParser->new;
    $client->use_deflate;

    my $server = Protocol::WebSocket::XS::ServerParser->new;
    $server->use_deflate;

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
    my $f = $c->get_frames($bin);
    ok $f;
};


done_testing;
