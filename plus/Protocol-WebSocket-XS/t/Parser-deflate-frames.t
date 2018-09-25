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
    ok $p->accept($handshake_message);
    $p->accept_response;
    ok $p->established;
    return $p;
};

subtest 'empty payload frame' => sub {
    my $payload = "";
    my $bin = $create_server->($default_compression)->start_message({final => 1})->send($payload);
    ok $bin;
    is(length($bin), 6, "frame length ok"); # 2 header + 4 bytes empty zlib frame
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
    is $encoded, 'eJwrKEotS00BAA==';
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
    is(length($bin), 21, "frame length ok");
    my $deflate_payload = substr($bin, 2);
    my $encoded = encode_base64pad($deflate_payload);
    note $encoded;
    is $encoded, 'eJwzMBgFo2AUjIJRMApGwQAAAA==';
    is_bin($bin, gen_frame({mask => 0, fin => 1, rsv1 => 1, opcode => OPCODE_BINARY, data => $deflate_payload}), "frame ok");

    subtest "it mode" => sub {
        my $bin2 = $create_server->($default_compression)->start_message({final => 1})->send_av(\@payload);
        is(length($bin2), length($bin), "frame length ok");
        my $deflate_payload2 = substr($bin2, 2);
        my $encoded2 = encode_base64pad($deflate_payload2);
        note $encoded2;
        is_bin($deflate_payload2, $deflate_payload, "it mode ok");
        is_bin($bin2, $bin, "it mode ok");
    };
};

subtest 'big (107 kb) server2client frame' => sub {
    my @payload = ('0') x (1024 * 107);
    my $payload = join('', @payload);
    my $bin = $create_server->($default_compression)->start_message({final => 1})->send($payload);
    is(length($bin), 130, "frame length ok");
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

subtest '2 messages in a sequence' => sub {
    my $p = $create_server->($default_compression);
    my @payload = ('0') x (1024);

    subtest "as single lines" => sub {
        my $payload = join('', @payload);
        my $bin_1 = $p->start_message()->final(1)->send($payload);
        ok $bin_1;
        my $bin_2 = $p->start_message()->final(1)->send($payload);
        is $bin_1, $bin_2;
    };

    subtest "as iterators" => sub {
        my $bin_1 = $p->start_message()->final(1)->send_av(\@payload);
        ok $bin_1;
        my $bin_2 = $p->start_message()->final(1)->send_av(\@payload);
        is $bin_1, $bin_2;
    };
};

=x
no longer valid
subtest "2 frames with split payload eq 1 frame" => sub  {
    my $p = MyTest::get_established_server();
    my @payload = ('0') x (1024);
    my $payload = join('', @payload);
    my $payload_merged = join('', $payload, $payload);
    my $bin_1 = $p->start_message({final => 1, deflate => 1})->send($payload_merged);
    my $builder = $p->start_message({deflate => 1});
    my $bin_21 = $builder->send($payload);
    my $bin_22 = $builder->final(1)->send($payload);

    my $data_1  = substr($bin_1, 2);
    my $data_21 = substr($bin_21, 2);
    my $data_22 = substr($bin_22, 2);
    note "merged length = ", length($data_1), ", chunk1 = ", length($data_21), ", chunk2 = ", length($data_22);
    my $data_2 = join('', $data_21, $data_22);
    is_bin($data_1, $data_2);
};
=cut

=x
subtest "no context takeover" => sub {
    my $handshake =<<END;
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
#Sec-WebSocket-Extensions: permessage-deflate; client_no_context_takeover, server_no_context_takeover\r
    my $p = $create_server->($handshake);
    my $payload = join('', ('0') x (1024));
    my $bin_1 = $p->start_message()->final(1)->send($payload);
    ok $bin_1;
    my $bin_2 = $p->start_message()->final(1)->send($payload);
    is $bin_1, $bin_2;
};
=cut

sub is_bin {
    my ($got, $expected, $name) = @_;
    return if our $leak_test;
    local $Test::Builder::Level = $Test::Builder::Level + 1;
    state $has_binary = eval { require Test::BinaryData; Test::BinaryData->import(); 1 };
    $has_binary ? is_binary($got, $expected, $name) : is($got, $expected, $name);
}

done_testing;
