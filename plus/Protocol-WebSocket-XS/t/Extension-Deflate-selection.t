use 5.020;
use warnings;
use lib 't'; use MyTest;

my $req = {
    uri    => "ws://crazypanda.ru",
    ws_key => "dGhlIHNhbXBsZSBub25jZQ==",
};

subtest "permessage-deflate extension in request" => sub {
    subtest "no deflate enabled => no extension" => sub {
        my $client = Protocol::WebSocket::XS::ClientParser->new;
        my $str = $client->connect_request($req);
        unlike $str, qr/permessage-deflate/;
    };

    subtest "deflate enabled => extension is present" => sub {
        my $client = Protocol::WebSocket::XS::ClientParser->new;
        $client->use_deflate;
        my $str = $client->connect_request($req);
        like $str, qr/permessage-deflate/;
    };

    subtest "deflate enabled(custom params) => extension is present" => sub {
        my $client = Protocol::WebSocket::XS::ClientParser->new;
        $client->use_deflate({
            client_no_context_takeover => 1,
            server_no_context_takeover => 1,
            server_max_window_bits     => 13,
            client_max_window_bits     => 14,
        });
        my $str = $client->connect_request($req);
        like $str, qr/permessage-deflate/;
        like $str, qr/client_no_context_takeover/;
        like $str, qr/server_no_context_takeover/;
        like $str, qr/server_max_window_bits=13/;
        like $str, qr/client_max_window_bits=14/;
    };
};

subtest "permessage-deflate extension in server reply" => sub {

    subtest "no deflate enabled => no extension" => sub {
        my $client = Protocol::WebSocket::XS::ClientParser->new;
        my $str = $client->connect_request($req);
        my $server = Protocol::WebSocket::XS::ServerParser->new;
        my $creq = $server->accept($str) or die "should not happen";
        my $res_str = $creq->error ? $server->accept_error : $server->accept_response;
        unlike $res_str, qr/permessage-deflate/;
    };

    subtest "client deflate: on, server deflate: off => extension: off" => sub {
        my $client = Protocol::WebSocket::XS::ClientParser->new;
        $client->use_deflate;
        my $str = $client->connect_request($req);
        my $server = Protocol::WebSocket::XS::ServerParser->new;
        my $creq = $server->accept($str) or die "should not happen";
        my $res_str = $creq->error ? $server->accept_error : $server->accept_response;
        like $str, qr/permessage-deflate/;
        unlike $res_str, qr/permessage-deflate/;
        ok !$client->is_deflate_active;
        ok !$server->is_deflate_active;
    };

    subtest "client deflate: on, server deflate: on => extension: on" => sub {
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
    };

    subtest "client deflate: on (empty client_max_window_bits), server deflate: on => extension: on" => sub {
        my $client = Protocol::WebSocket::XS::ClientParser->new;
        $client->use_deflate;

        my $server = Protocol::WebSocket::XS::ServerParser->new;
        $server->use_deflate;

        my $str = $client->connect_request($req);
        $str =~ s/(client_max_window_bits)=15/$1/;
        my $creq = $server->accept($str) or die "should not happen";
        my $res_str = $creq->error ? $server->accept_error : $server->accept_response;
        like $str, qr/permessage-deflate/;
        like $res_str, qr/permessage-deflate/;
        like $res_str, qr/client_max_window_bits[^=]/;
        ok $server->is_deflate_active;

        $client->connect($res_str);
        ok $client->established;
        ok $client->is_deflate_active;
    };

    subtest "client deflate: on (wrong params), server deflate: off => extension: off" => sub {

        subtest "too small window" => sub {
            my $server = Protocol::WebSocket::XS::ServerParser->new;
            $server->use_deflate;

            my $client = Protocol::WebSocket::XS::ClientParser->new;
            $client->use_deflate({ client_max_window_bits => 3});

            my $str = $client->connect_request($req);
            my $creq = $server->accept($str) or die "should not happen";
            my $res_str = $creq->error ? $server->accept_error : $server->accept_response;
            like $str, qr/permessage-deflate/;
            unlike $res_str, qr/permessage-deflate/;
            ok !$server->is_deflate_active;

            $client->connect($res_str);
            ok $client->established;
            ok !$client->is_deflate_active;
        };

        subtest "too big window" => sub {
            my $server = Protocol::WebSocket::XS::ServerParser->new;
            $server->use_deflate;

            my $client = Protocol::WebSocket::XS::ClientParser->new;
            $client->use_deflate({ client_max_window_bits => 30});

            my $str = $client->connect_request($req);
            my $creq = $server->accept($str) or die "should not happen";
            my $res_str = $creq->error ? $server->accept_error : $server->accept_response;
            like $str, qr/permessage-deflate/;
            unlike $res_str, qr/permessage-deflate/;
            ok !$server->is_deflate_active;

            $client->connect($res_str);
            ok $client->established;
            ok !$client->is_deflate_active;
        };

        subtest "unknown parameter" => sub {
            my $req = {
                uri           => "ws://crazypanda.ru",
                ws_key        => "dGhlIHNhbXBsZSBub25jZQ==",
                ws_extensions => [ [ 'permessage-deflate', { 'hacker' => 'huyaker' } ] ],
            };


            my $server = Protocol::WebSocket::XS::ServerParser->new;
            $server->use_deflate;

            my $client = Protocol::WebSocket::XS::ClientParser->new;
            # $client->use_deflate(); - manually set up bu client

            my $str = $client->connect_request($req);
            my $creq = $server->accept($str) or die "should not happen";
            my $res_str = $creq->error ? $server->accept_error : $server->accept_response;
            like $str, qr/permessage-deflate/;
            unlike $res_str, qr/permessage-deflate/;
            ok !$server->is_deflate_active;

            $client->connect($res_str);
            ok $client->established;
            ok !$client->is_deflate_active;
        };

        subtest "incorrect parameter" => sub {
            my $req = {
                uri           => "ws://crazypanda.ru",
                ws_key        => "dGhlIHNhbXBsZSBub25jZQ==",
                ws_extensions => [ [ 'permessage-deflate', { 'client_max_window_bits' => 'kak tebe takoe, Ilon Mask?' } ] ],
            };

            my $server = Protocol::WebSocket::XS::ServerParser->new;
            $server->use_deflate;

            my $client = Protocol::WebSocket::XS::ClientParser->new;
            # $client->use_deflate(); - manually set up by client

            my $str = $client->connect_request($req);
            my $creq = $server->accept($str) or die "should not happen";
            my $res_str = $creq->error ? $server->accept_error : $server->accept_response;
            like $str, qr/permessage-deflate/;
            unlike $res_str, qr/permessage-deflate/;
            ok !$server->is_deflate_active;

            $client->connect($res_str);
            ok $client->established;
            ok !$client->is_deflate_active;
        };

    };

    subtest "client deflate: on, server deflate: on (wrong params) => extension: off" => sub {
        subtest "offected other windows size" => sub {
            my $client = Protocol::WebSocket::XS::ClientParser->new;
            $client->use_deflate();

            my $str = $client->connect_request($req);
            my $res_str =<<END;
HTTP/1.1 101 Switching Protocols\r
Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r
Sec-WebSocket-Extensions: permessage-deflate; server_max_window_bits=13; client_max_window_bits=15
Connection: Upgrade\r
Server: Panda-WebSocket\r
Upgrade: websocket\r
\r
END
            $client->connect($res_str);
            ok $client->established;
            ok !$client->is_deflate_active;
        };

        subtest "offected garbage windows size" => sub {
            my $client = Protocol::WebSocket::XS::ClientParser->new;
            $client->use_deflate();

            my $str = $client->connect_request($req);
            my $res_str =<<END;
HTTP/1.1 101 Switching Protocols\r
Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r
Sec-WebSocket-Extensions: permessage-deflate; server_max_window_bits=zzzz; client_max_window_bits=15
Connection: Upgrade\r
Server: Panda-WebSocket\r
Upgrade: websocket\r
\r
END
            $client->connect($res_str);
            ok $client->established;
            ok !$client->is_deflate_active;
        };

        subtest "offected garbage extension parameter" => sub {
            my $client = Protocol::WebSocket::XS::ClientParser->new;
            $client->use_deflate();

            my $str = $client->connect_request($req);
            my $res_str =<<END;
HTTP/1.1 101 Switching Protocols\r
Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r
Sec-WebSocket-Extensions: permessage-deflate; server_max_window_bits=15; client_max_window_bits=15; hello=world
Connection: Upgrade\r
Server: Panda-WebSocket\r
Upgrade: websocket\r
\r
END
            $client->connect($res_str);
            ok $client->established;
            ok !$client->is_deflate_active;
        };

    };

};

done_testing;
