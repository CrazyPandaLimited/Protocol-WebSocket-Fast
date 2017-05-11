use 5.020;
use Panda::Install;

write_makefile(
    NAME          => 'Panda::WebSocket',
    VERSION_FROM  => 'lib/Panda/WebSocket.pm',
    ABSTRACT_FROM => 'lib/Panda/WebSocket.pod',
    CPLUS         => 1,
    CCFLAGS       => '-fno-omit-frame-pointer',
    BIN_DEPS      => ['Panda::Export', 'Panda::URI', 'Panda::Encode::Base2N'],
    LIBS          => '-lssl -L/usr/local/opt/openssl/lib',
    SRC           => 'src',
    INC           => '-Isrc -I/usr/local/opt/openssl/include',
    TYPEMAPS      => 'typemap',
    BIN_SHARE => {
        INCLUDE  => {'src' => '/'},
        TYPEMAPS => {'typemap' => ''},
    },
    CCFLAGS => '-Wall -Wextra',
    OPTIMIZE => '-O2 -g0',
    #CC => 'clang++',
);
