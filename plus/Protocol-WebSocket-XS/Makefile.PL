use 5.020;
use XS::Install;

write_makefile(
    NAME          => 'Protocol::WebSocket::XS',
    ABSTRACT_FROM => 'lib/Protocol/WebSocket/XS.pod',
    CPLUS         => 11,
    TEST_REQUIRES => {'Test::Exception' => 0},
    BIN_DEPS      => ['Panda::Export', 'URI::XS', 'Encode::Base2N'],
    LIBS          => '-lcrypto -L/usr/local/opt/openssl/lib',
    SRC           => 'src',
    INC           => '-Isrc -I/usr/local/opt/openssl/include',
    BIN_SHARE => {
    	CPLUS    => 11,
        INCLUDE  => {'src' => '/'},
    },
    CCFLAGS  => '-fno-omit-frame-pointer -Wall -Wextra',
    OPTIMIZE => '-O2',
);
