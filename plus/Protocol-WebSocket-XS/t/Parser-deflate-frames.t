use 5.020;
use warnings;
use lib 't'; use MyTest;
use Encode::Base2N qw/encode_base64pad/;

*gen_frame = \&MyTest::gen_frame;

my $default_compression =<<END;
GET /?encoding=text HTTP/1.1\r
Host: dev.crazypanda.ru:4680\r
Connection: Upgrade\r
Pragma: no-cache\r
Cache-Control: no-cache\r
Upgrade: websocket\r
Origin: http://www.websocket.org\r
Sec-WebSocket-Version: 13\r
User-Agent: Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/55.0.2883.87 Safari/537.36\r
Accept-Encoding: gzip, deflate, sdch\r
Accept-Language: ru-RU,ru;q=0.8,en-US;q=0.6,en;q=0.4\r
Cookie: _ga=GA1.2.1700804447.1456741171\r
Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r
Sec-WebSocket-Extensions: permessage-deflate\r
\r
END

my $create_server = sub {
    local $Test::Builder::Level = $Test::Builder::Level + 1;
    my $handshake_message = shift;
    my $p = Protocol::WebSocket::XS::ServerParser->new;
    $p->use_deflate;
    ok $p->accept($handshake_message);
    $p->accept_response;
    ok $p->established;
    return $p;
};

subtest 'empty payload frame' => sub {
    my $payload = "";
    my $bin = $create_server->($default_compression)->start_message({final => 1})->send($payload);
    ok $bin;
    is(length($bin), 5, "frame length ok"); # 2 header + 3 bytes empty zlib frame
    my $deflate_payload = substr($bin, 2);
    is_bin($bin, gen_frame({mask => 0, fin => 1, rsv1 => 1, opcode => OPCODE_BINARY, data => $deflate_payload}), "frame ok");
};

subtest 'small server2client frame' => sub {
    my $payload = "preved"; # must be <= 125
    my $bin = $create_server->($default_compression)->start_message({final => 1})->send($payload);
    is(length($bin), 12, "frame length ok"); # 2 header + 10 payload
    my $deflate_payload = substr($bin, 2);
    my $encoded = encode_base64pad($deflate_payload);
    note $encoded;
    is $encoded, 'eJwqKEotS00BAA==';
    is_bin($bin, gen_frame({mask => 0, fin => 1, rsv1 => 1, opcode => OPCODE_BINARY, data => $deflate_payload}), "frame ok");

    subtest "it mode" => sub {
        my $bin2 = $create_server->($default_compression)->start_message({final => 1})->send_av([qw/pre ved/]);
        is(length($bin2), 12, "frame length ok");
        my $deflate_payload2 = substr($bin2, 2);
        is_bin($deflate_payload2, $deflate_payload, "it mode ok");
        is_bin($bin2, $bin, "it mode ok");
    };

    subtest "it mode by byte" => sub {
        my $bin3 = $create_server->($default_compression)->start_message({final => 1})->send_av([split //, $payload]);
        is_bin($bin3, $bin, "it mode ok");
    };
};

subtest 'big (1923 b) server2client frame' => sub {
    my @payload = ('0') x (1923);
    my $payload = join('', @payload);
    my $bin = $create_server->($default_compression)->start_message({final => 1})->send($payload);
    is(length($bin), 22, "frame length ok");
    my $deflate_payload = substr($bin, 2);
    my $encoded = encode_base64pad($deflate_payload);
    note $encoded;
    is $encoded, 'eJwyMBgFo2AUjIJRMApGwQAAAAA=';
    is_bin($bin, gen_frame({mask => 0, fin => 1, rsv1 => 1, opcode => OPCODE_BINARY, data => $deflate_payload}), "frame ok");

    subtest "it mode" => sub {
        # it seems this is special case, where they do not match, as deflate-ext cannot write out
        # all data in 1st chunk, so it is "split" by some divisor
        my $bin2 = $create_server->($default_compression)->start_message({final => 1})->send_av(\@payload);
        #is(length($bin2), length($bin), "frame length ok");
        my $deflate_payload2 = substr($bin2, 2);
        my $encoded2 = encode_base64pad($deflate_payload2);
        is $encoded2, 'eJwyMBgFo2AUjIJRMApGwQAAAAAAAP//AA==';
        note $encoded2;
        #is_bin($deflate_payload2, $deflate_payload, "it mode ok");
        #is_bin($bin2, $bin, "it mode ok");
    };
};

subtest 'big (107 kb) server2client frame' => sub {
    my @payload = ('0') x (1024 * 107);
    my $payload = join('', @payload);
    my $bin = $create_server->($default_compression)->start_message({final => 1})->send($payload);
    is(length($bin), 131, "frame length ok");
    my $deflate_payload = substr($bin, 4);
    my $encoded = encode_base64pad($deflate_payload);
    note $encoded;
    is_bin($bin, gen_frame({mask => 0, fin => 1, rsv1 => 1, opcode => OPCODE_BINARY, data => $deflate_payload}), "frame ok");

    subtest "it mode" => sub {
        my $bin2 = $create_server->($default_compression)->start_message({final => 1})->send_av(\@payload);
        is(length($bin2), length($bin), "frame length ok");
        my $deflate_payload2 = substr($bin2, 4);
        my $encoded2 = encode_base64pad($deflate_payload2);
        note $encoded2;
        is_bin($deflate_payload2, $deflate_payload, "it mode ok");
        is_bin($bin2, $bin, "it mode ok");
    };
};


subtest 'big (1 mb) server2client frame' => sub {
    my @payload = ('0') x (1024 * 1024);
    my $payload = join('', @payload);
    my $bin = $create_server->($default_compression)->start_message({final => 1})->send($payload);
    is(length($bin), 1040, "frame length ok");
    my $deflate_payload = substr($bin, 4);
    my $encoded = encode_base64pad($deflate_payload);
    note $encoded;
    is_bin($bin, gen_frame({mask => 0, fin => 1, rsv1 => 1, opcode => OPCODE_BINARY, data => $deflate_payload}), "frame ok");

    subtest "it mode" => sub {
        my $bin2 = $create_server->($default_compression)->start_message({final => 1})->send_av(\@payload);
        is(length($bin2), length($bin), "frame length ok");
        my $deflate_payload2 = substr($bin2, 4);
        my $encoded2 = encode_base64pad($deflate_payload2);
        note $encoded2;
        is_bin($deflate_payload2, $deflate_payload, "it mode ok");
        is_bin($bin2, $bin, "it mode ok");
    };
};

subtest '2 messages in a sequence (different due to context takeover)' => sub {
    my @payload = ('0') x (1024);

    subtest "as single lines" => sub {
        my $p = $create_server->($default_compression);
        my $payload = join('', @payload);
        my $bin_1 = $p->start_message()->final(1)->send($payload);
        ok $bin_1;
        my $bin_2 = $p->start_message()->final(1)->send($payload);
        ok $bin_2;
        ok length($bin_1) > length($bin_2);
        my $e1 = encode_base64pad($bin_1);
        my $e2 = encode_base64pad($bin_2);
        note "e1 = ", $e1, ", e2 = ", $e2;
        isnt $e1, $e2;
    };

    subtest "as iterators" => sub {
        my $p = $create_server->($default_compression);
        my $bin_1 = $p->start_message()->final(1)->send_av(\@payload);
        ok $bin_1;
        my $bin_2 = $p->start_message()->final(1)->send_av(\@payload);
        ok length($bin_1) > length($bin_2);
        my $e1 = encode_base64pad($bin_1);
        my $e2 = encode_base64pad($bin_2);
        note "e1 = ", $e1, ", e2 = ", $e2;
        isnt $e1, $e2;
    };
};

subtest "no context takeover" => sub {
    my $handshake =<<END;
GET /?encoding=text HTTP/1.1\r
Host: dev.crazypanda.ru:4680\r
Connection: Upgrade\r
Upgrade: websocket\r
Sec-WebSocket-Version: 13\r
User-Agent: Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/55.0.2883.87 Safari/537.36\r
Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r
Sec-WebSocket-Extensions: permessage-deflate; client_no_context_takeover; server_no_context_takeover\r
\r
END
    my $p = $create_server->($handshake);
    my @payload = ('0') x (1024);
    my $payload = join('', @payload);
    my $bin_1 = $p->start_message()->final(1)->send($payload);
    ok $bin_1;
    my $bin_2 = $p->start_message()->final(1)->send($payload);
    my $e1 = encode_base64pad($bin_1);
    my $e2 = encode_base64pad($bin_2);
    is $e1, $e2;
};

sub is_bin {
    my ($got, $expected, $name) = @_;
    return if our $leak_test;
    local $Test::Builder::Level = $Test::Builder::Level + 1;
    state $has_binary = eval { require Test::BinaryData; Test::BinaryData->import(); 1 };
    $has_binary ? is_binary($got, $expected, $name) : is($got, $expected, $name);
}

done_testing;
