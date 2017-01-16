#!/usr/local/bin/perl
use 5.020;
use lib 'blib/lib', 'blib/arch', 't';
use Benchmark qw/timethis timethese/;
use JSON::XS qw/encode_json/;
use Data::Dumper qw/Dumper/;
use Time::HiRes;
use Panda::WebSocket;

say "START $$";

my $ws = new Panda::WebSocket::ServerParser();

my @data = (
    'GET /?encoding=text HTTP/1.1',
    'Host: dev.crazypanda.ru:4680',
    'Connection: Upgrade',
    'Pragma: no-cache',
    'Cache-Control: no-cache',
    'Upgrade: websocket',
    'Origin: http://www.websocket.org',
    'Sec-WebSocket-Version: 13',
    'User-Agent: Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/55.0.2883.87 Safari/537.36',
    'Accept-Encoding: gzip, deflate, sdch',
    'Accept-Language: ru-RU,ru;q=0.8,en-US;q=0.6,en;q=0.4',
    'Cookie: _ga=GA1.2.1700804447.1456741171',
    'Sec-WebSocket-Key: UCBGOTpHEtVI5XYRBb5lDg==',
    'Sec-WebSocket-Extensions: permessage-deflate; client_max_window_bits',
    "\r\n"#."allo",
    #"hello",
    #"world",
);
my $str = join("\r\n", @data);

if (0) {
    foreach my $chunk (@data) {
        $ws->add_buffer("$chunk\r\n") or die "no more buffer space";
    #    if (rand(1) > 0.5 or $chunk eq $data[$#data]) {
    #        my $ret = $ws->accept();
    #        say "RET=".($ret ? encode_json($ret) : "N/A");
    #    }
    }
}
else {
    $ws->add_buffer($str);
    say length $str;
}

my $ret = $ws->accept();
say "after accept in perl";
say "RET=".Dumper($ret);
say $ret->{uri};

#timethis(-1, sub { $ws->accept() });

timethis(-1, sub {
    $ws->reset();
    $ws->add_buffer($str);
    my $ret = $ws->accept();
});

1;