package JSON::Create::PP;
use parent Exporter;
our @EXPORT_OK = qw/create_json/;
our %EXPORT_TAGS = (all => \@EXPORT_OK);
use warnings;
use strict;
use utf8;
use Carp;
use Scalar::Util 'looks_like_number';

sub stringify
{
    my ($input) = @_;
    $input =~ s/("|\\)/\\$1/g;
    $input =~ s/[\x00-\x1f]/sprintf ("\\u%04", $1)/ge;
    return "\"$input\"";
}

sub create_json_recursively
{
    my ($input) = @_;
    my $output = '';
    if (ref $input eq 'HASH') {
	$output .= '{';
	my @v;
	for my $k (keys %$input) {
	    my $item = stringify ($k);
	    $item .= ':';
	    $item .= create_json_recursively ($input->{$k});
	    push @v, $item;
	}
	$output .= join (',', @v);
	$output .= '}';
    }
    elsif (ref $input eq 'ARRAY') {
	$output .= '[';
	my @v;
	for my $k (@$input) {
	    push @v, create_json_recursively ($k);
	}
	$output .= join (',', @v);
	$output .= ']';
    }
    else {
	if (looks_like_number ($input)) {
	    return $input;
	}
	elsif (! defined $input) {
	    return 'null';
	}
	else {
	    my $output = stringify ($input);
	}
    }
}

sub create_json
{
    my ($input, %options) = @_;
    my $output = create_json_recursively ($input);
    return $output;
}

1;
