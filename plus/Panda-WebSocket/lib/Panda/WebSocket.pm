package Panda::WebSocket;
use parent 'Panda::Export';
use 5.020;
use Panda::Event;
use Panda::WebSocket::Error;

our $VERSION = '0.1.0';

require Panda::XSLoader;
Panda::XSLoader::load();

1;