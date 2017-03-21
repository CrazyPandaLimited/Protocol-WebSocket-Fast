use 5.020;
use warnings;
use lib 't/lib'; use WSTest;

my $has_binary = eval { require Test::BinaryData; Test::BinaryData->import(); 1 };

*gen_frame = \&WSTest::gen_frame;

my $p = WSTest::get_established_server();

subtest 'one frame message' => sub {
    my $payload = "preved"; # must be <= 125
    my $bin = $p->send_message($payload);
    is(length($bin), 8, "frame length ok"); # 2 header + 6 payload
    is_bin($bin, gen_frame({mask => 0, fin => 1, opcode => OPCODE_BINARY, data => $payload}), "frame ok");
    is_bin($p->send_message_av([qw/pr ev ed/]), $bin, "it mode ok");
};

subtest 'multi frame message' => sub {
    my $bin = $p->send_message_multiframe([qw/first second third/]);
    is(length($bin), 22, "message length ok"); # (2 header + 5 payload) + (2 header + 6 payload) + (2 header + 5 payload)
    is_bin($bin, gen_frame({mask => 0, fin => 0, opcode => OPCODE_BINARY, data => "first"}).
                 gen_frame({mask => 0, fin => 0, opcode => OPCODE_CONTINUE, data => "second"}).
                 gen_frame({mask => 0, fin => 1, opcode => OPCODE_CONTINUE, data => "third"}),
                 "message ok");
};

done_testing();

sub is_bin {
    my ($got, $expected, $name) = @_;
    return if our $leak_test;
    $has_binary ? is_binary($got, $expected, $name) : is($got, $expected, $name);
}