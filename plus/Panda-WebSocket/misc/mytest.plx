#!/usr/local/bin/perl
use 5.020;
use lib 'blib/lib', 'blib/arch', 't';
use Benchmark qw/timethis timethese/;
use Time::HiRes;
use Panda::WebSocket;

say "START $$";

my $loop = new Panda::Event::Loop;

my $f = new Panda::WebSocket::Server;
$f->init({
    loop      => $loop,
    locations => [
        {host => 'dev', port => 4680},
        {host => 'dev', port => 4681, secure => 1},
    ],
});

$f->run;

1;