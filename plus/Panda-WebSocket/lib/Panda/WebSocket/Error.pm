package Panda::WebSocket::Error;
use parent 'Panda::Export';
use 5.020;

use overload
    '""' => \&to_string;

sub new {
    my ($class, $data) = @_;
    my $self = bless $data, $class;
    $self->{mess} //= '';
    return $self;
}

sub what { $_[0]->{what} }

sub to_string {
    my $self = shift;
    my $class = ref $self;
    my $rind = rindex($class, ':');
    $class = substr($class, $rind+1);
    return "[$class] $self->{what}$self->{mess}";
}

1;