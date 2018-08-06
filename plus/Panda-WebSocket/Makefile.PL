use 5.020;
use Panda::Install;

write_makefile(
    NAME          => 'Panda::WebSocket',
    VERSION_FROM  => 'lib/Panda/WebSocket.pm',
    ABSTRACT_FROM => 'lib/Panda/WebSocket.pod',
    CPLUS         => 1,
    BIN_DEPS      => ['Panda::Export', 'Panda::URI', 'Panda::Encode::Base2N'],
    LIBS          => '-lcrypto -L/usr/local/opt/openssl/lib',
    SRC           => 'src',
    INC           => '-Isrc -I/usr/local/opt/openssl/include',
    BIN_SHARE => {
        INCLUDE  => {'src' => '/'},
    },
    CCFLAGS  => '-fno-omit-frame-pointer -Wall -Wextra',
    OPTIMIZE => '-O2',
);
