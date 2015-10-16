=encoding UTF-8

=head1 NAME

JSON::Create - turn a Perl variable into JSON

=head1 SYNOPSIS

    use JSON::Create 'create_json';

=head1 DESCRIPTION

This module produces JSON out of Perl

=head1 FUNCTIONS

=head2 create_json

    my $json = create_json (\%hash);

=head1 SEE ALSO

This module is a companion module to the same author's L<JSON::Parse>.

=cut

package JSON::Create;
require Exporter;
@ISA = qw(Exporter);
@EXPORT_OK = qw/create_json/;
%EXPORT_TAGS = (
    all => \@EXPORT_OK,
);
use warnings;
use strict;
use Carp;
our $VERSION = '0.00_02';
require XSLoader;
XSLoader::load ('JSON::Create', $VERSION);
1;
