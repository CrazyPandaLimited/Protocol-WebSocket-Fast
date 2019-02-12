use 5.020;
use XS::Install;

my %params = (
    NAME          => 'Protocol::WebSocket::XS',
    ABSTRACT_FROM => 'lib/Protocol/WebSocket/XS.pod',
    CPLUS         => 11,
    TEST_REQUIRES => {'Test::Exception' => 0},
    BIN_DEPS      => ['Protocol::HTTP', 'Panda::Export', 'URI::XS', 'Encode::Base2N', 'Test::Catch'],
    LIBS          => ['-lcrypto -lz'], #TODO: global path 0_0 RLY?
    SRC           => 'src',
    INC           => '-Isrc -I/usr/local/opt/openssl/include',
    BIN_SHARE => {
    	CPLUS    => 11,
        INCLUDE  => {'src' => '/'},
    },
    CCFLAGS  => '-fno-omit-frame-pointer -Wall -Wextra',
    test     => {
        SRC                  => 't',
        RECURSIVE_TEST_FILES => 1,
    },
);

if ($ENV{SANITIZE}) {
    ($params{CCFLAGS}||='') .= ' -fsanitize=undefined -fsanitize=address -fno-omit-frame-pointer';
    $params{LIBS}[0] .= ' -lubsan -lasan'; #sanitizer
}


write_makefile(%params);


