#!/usr/bin/env perl
use lib 'blib/lib', 'blib/arch', 't/lib';
use Benchmark qw/timethis timethese/;

use URI::XS;
use Protocol::WebSocket::XS::ClientParser;
use Protocol::WebSocket::XS::ServerParser;
use Protocol::WebSocket::Frame;

my $client = Protocol::WebSocket::XS::ClientParser->new;
my $req_str = $client->connect_request({
    uri           => URI::XS->new("ws://example.com/"),
    ws_key        => "dGhlIHNhbXBsZSBub25jZQ==",
    ws_protocol   => "chat",
    ws_extensions => [ [ 'permessage-deflate'] ],
    headers       => {
        'Origin'          => 'http://www.crazypanda.ru',
        'User-Agent'      => 'My-UA',
    },
});

my $server = Protocol::WebSocket::XS::ServerParser->new;
my $req    = $server->accept($req_str);

my $accept_reply_str = $req->error ? $server->accept_error : $server->accept_response;
my $reply = $client->connect($accept_reply_str);
$client->established;      
$client->is_deflate_active;
$server->is_deflate_active;


my $builder = $server->start_message({opcode => OPCODE_TEXT, final => 1, deflate => 0});
my $data = $builder->send("Lorem ipsum dolor " x 10);

my ($f1) =  $client->get_frames($data);
print "Protocol::WebSocket::XS out: ", $f1->payload, "\n";

my $f2 =  Protocol::WebSocket::Frame->new($data);
print "Protocol::WebSocket out: ", $f2->next, "\n";

timethese(-1, {
    "Protocol::WebSocket::XS" => sub { $client->get_messages($data) },
    "Protocol::WebSocket "    => sub { Protocol::WebSocket::Frame->new($data)->next },
});
