package JSON::Create;
require Exporter;
@ISA = qw(Exporter);
@EXPORT_OK = qw/create_json/;
%EXPORT_TAGS = (
    all => \@EXPORT_OK,
);
use warnings;
use strict;
our $VERSION = '0.04_02';
require XSLoader;
XSLoader::load ('JSON::Create', $VERSION);
1;
