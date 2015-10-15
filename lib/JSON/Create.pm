=encoding UTF-8

=head1 NAME

JSON::Create - abstract here.

=head1 SYNOPSIS

    use JSON::Create;

=head1 DESCRIPTION

=head1 FUNCTIONS

=cut
package JSON::Create;
require Exporter;
@ISA = qw(Exporter);
@EXPORT_OK = qw//;
%EXPORT_TAGS = (
    all => \@EXPORT_OK,
);
use warnings;
use strict;
use Carp;
our $VERSION = 0.01;
require XSLoader;
XSLoader::load ('JSON::Create', $VERSION);
1;
