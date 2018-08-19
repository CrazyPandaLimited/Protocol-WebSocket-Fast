package Panda::WebSocket;
use parent 'Panda::Export';
use 5.020;
use Panda::URI;
use Panda::Export();
use Panda::Encode::Base2N;

our $VERSION = '0.1.0';

XS::Loader::load();

1;