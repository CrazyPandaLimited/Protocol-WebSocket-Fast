package Protocol::WebSocket::XS;
use 5.020;
use URI::XS;
use Export::XS();
use Encode::Base2N;

our $VERSION = '0.1.0';

XS::Loader::load();

1;
