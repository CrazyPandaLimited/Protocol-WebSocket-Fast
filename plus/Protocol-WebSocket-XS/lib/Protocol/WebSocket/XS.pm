package Protocol::WebSocket::XS;
use parent 'Panda::Export';
use 5.020;
use URI::XS;
use Panda::Export();
use Encode::Base2N;

our $VERSION = '0.1.0';

XS::Loader::load();

1;
