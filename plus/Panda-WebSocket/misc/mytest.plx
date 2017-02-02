#!/usr/local/bin/perl
use 5.020;
use lib 'blib/lib', 'blib/arch', 't/lib';
use Benchmark qw/timethis timethese/;
use JSON::XS qw/encode_json/;
use Data::Dumper qw/Dumper/;
use Time::HiRes;
use Panda::WebSocket;
use WSTest;

say "START $$";

my $p = WSTest::get_established_server();

use Benchmark qw/timethis/;
my $s = WSTest::gen_frame({opcode => OPCODE_TEXT, mask => 1, fin => 1, data => ("1" x 100000)});
timethis(-1, sub {
    $p->get_frames($s);
});

1;