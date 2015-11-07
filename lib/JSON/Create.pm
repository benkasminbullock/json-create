package JSON::Create;
require Exporter;
@ISA = qw(Exporter);
@EXPORT_OK = qw/create_json/;
%EXPORT_TAGS = (
    all => \@EXPORT_OK,
);
use warnings;
use strict;
our $VERSION = '0.13';

eval {
    require XSLoader;
    XSLoader::load ('JSON::Create', $VERSION);
#    die;
};
if ($@) {
    require JSON::Create::PP;
    JSON::Create::PP->import (':all');
}

sub set_fformat
{
    my ($obj, $fformat) = @_;
    if (! $fformat) {
	$obj->set_fformat_unsafe (0);
	return;
    }
    if ($fformat =~ /^%(?:(?:([0-9]+)?(?:\.([0-9]+)?)?)?[fFgGeE])$/) {
	my $d1 = $1;
	my $d2 = $2;
	if ((defined ($d1) && $d1 > 20) || (defined ($d2) && $d2 > 20)) {
	    warn "Format $fformat is too long";
	    $obj->set_fformat_unsafe (0);
	}
	else {
	    $obj->set_fformat_unsafe ($fformat);
	}
    }
    else {
	warn "Format $fformat is not OK for floating point numbers";
	$obj->set_fformat_unsafe (0);
    }
}

sub bool
{
    my ($obj, @list) = @_;
    my $handlers = $obj->get_handlers ();
    for my $item (@list) {
	$handlers->{$item} = 'bool';
    }
}

sub obj
{
    my ($obj, %handlers) = @_;
    my $handlers = $obj->get_handlers ();
    for my $item (keys %handlers) {
	my $value = $handlers{$item};
	# Check it's a code reference somehow.
	$handlers->{$item} = $value;
    }
}

sub del
{
    my ($obj, @list) = @_;
    my $handlers = $obj->get_handlers ();
    for my $item (@list) {
	delete $handlers->{$item};
    }
}

sub validate
{
    my ($obj, $value) = @_;
    if ($value) {
	require JSON::Parse;
	JSON::Parse->import ('assert_valid_json');
    }
    $obj->set_validate ($value);
}

1;

