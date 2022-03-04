package Goblin;

use strict;
use warnings;

use Carp qw/croak/;
use FFI::Platypus::Buffer qw/grow scalar_to_pointer/;
use FFI::Platypus qw//;

use namespace::clean;

my $ffi = FFI::Platypus->new( api => 1 );

$ffi->find_lib( lib => "goblin" );

$ffi->mangler( sub { shift =~ s/^/goblin__/r } );

my %INTERNAL = (
    strerror => $ffi->function( strerror => [qw/int/] => "string" ),
);

$ffi->attach(
    img_stat => [qw/string size_t* uint32_t* uint32_t*/] => "int" => sub {
        my ( $xsub, $class, $path ) = @_;
        croak "missing required parameter: path" unless defined $path;
        my %stat;
        my $rs = $xsub->( $path, \@stat{qw/bytes width height/} );
        croak $INTERNAL{strerror}->($rs) if $rs;
        return \%stat;
    }
);

$ffi->attach(
    img_load => [qw/string opaque/] => "int" => sub {
        my ( $xsub, $class, $path ) = @_;
        croak "missing required parameter: path" unless defined $path;
        my $rgba;
        grow $rgba, $class->img_stat($path)->{bytes};
        my $rs = $xsub->( $path, scalar_to_pointer $rgba );
        croak $INTERNAL{strerror}->($rs) if $rs;
        return $rgba;
    }
);

1;
