use 5.020;
use warnings;
use lib 't/lib'; use WSTest;

my ($p, $binfr, $frame, @frames);

$p = WSTest::get_established_server();

# whole frame

$binfr = WSTest::gen_frame({opcode => OPCODE_BINARY, mask => 1, fin => 1, data => "hello world"});
@frames = $p->get_frames($binfr);
is(scalar(@frames), 1, "frame returned");
$frame = (@frames);
ok($frame, "frame returned");

done_testing();
