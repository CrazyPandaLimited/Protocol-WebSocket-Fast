use 5.020;
use warnings;
use lib 't', 't/lib'; use WSTest;
use Benchmark ':hireswallclock';
use Test::Builder;

plan skip_all => 'set WITH_LEAKS=1 to enable leaks test' unless $ENV{WITH_LEAKS};
plan skip_all => 'BSD::Resource required to test for leaks' unless eval { require BSD::Resource; 1 };

test_leak(1, [
    '02-ServerParser-accept.t',
    '03-ServerParser-accept_error.t',
    '04-ServerParser-accept_response.t',
    '05-Parser-get_frames.t',
    '06-Parser-get_messages.t',
    '07-Parser-mixed-mode.t',
], 100);

# $leak_threshold in Kb
sub test_leak {
    my ($time, $tests, $leak_threshold) = @_;
    $leak_threshold ||= 100;
    $time = $time * 0.75;
    $time = 0.33 if $time < 0.33;
    my $warmup_time = $time / 3;
    
    foreach my $test (@$tests) {
        next if ref $test;
        my $filename = $test;
        $test = sub {
        	local $SIG{__WARN__} = sub {}; # remove warnings (redefine, etc) as we load the same files many times
            require $filename;
            delete $INC{$filename};
        };
    }
    
    my $run_all = sub { $_->() for @$tests };
    
    my @a = 1..100; undef @a; # warmup perl
    my $measure = 0;
    my $leak = 0;
    
    {
    	no warnings;
        local *ok = sub {};
        local *is = sub {};
        local *isnt = sub {};
        local *diag = sub {};
        local *like = sub {};
        local *unlike = sub {};
        local *cmp_ok = sub {};
        local *is_deeply = sub {};
        local *can_ok = sub {};
        local *isa_ok = sub {};
        local *pass = sub {};
        local *fail = sub {};
        local *plan = sub {};
        local *note = sub {};
        local *cmp_deeply = sub {};
        local *cmp_bag = sub {};
        local *cmp_set = sub {};
        local *cmp_methods = sub {};
        local *done_testing = sub {1};
        local *subtest = sub { my $name = shift; my $code = shift; $code->(@_) };
        local $main::leak_test = 1;
        use warnings;
        
        Benchmark::countit($warmup_time, $run_all);
        $measure = BSD::Resource::getrusage()->{"maxrss"};
        Benchmark::countit($time, $run_all);
        $leak = BSD::Resource::getrusage()->{"maxrss"} - $measure;
    }
    
    my $leak_ok = $leak < $leak_threshold;

    warn("LEAK DETECTED: ${leak}Kb") unless $leak_ok;
    ok($leak_ok, "leak test");
}

done_testing();
