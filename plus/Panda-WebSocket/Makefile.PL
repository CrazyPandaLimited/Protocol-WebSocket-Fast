use 5.020;
use Panda::Install;

write_makefile(
    NAME          => 'Panda::WebSocket',
    VERSION_FROM  => 'lib/Panda/WebSocket.pm',
    ABSTRACT_FROM => 'lib/Panda/WebSocket.pod',
    CPLUS         => 1,
    CCFLAGS       => '-fno-omit-frame-pointer',
    BIN_DEPS      => ['Panda::Lib', 'Panda::Export', 'CPP::Range', 'Panda::URI', 'Panda::Encode::Base2N'],
    LIBS          => '-lssl',
    SRC           => 'src',
    INC           => '-Isrc',
    TYPEMAPS      => 'typemap',
    #BIN_SHARE => {
    #    INCLUDE  => {
    #        'src'           => '/',
    #        'libuv/include' => '/',
    #    },
    #    TYPEMAPS => {'typemap' => ''},
    #},
    CCFLAGS => '-Wall -Wextra',
    OPTIMIZE => '-O2 -g0',
    #CC => 'clang++',
);
