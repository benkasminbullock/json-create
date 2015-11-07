=head1 NAME

JSON::Create::PP - Pure-Perl version of JSON::Create

=head1 DESCRIPTION

=head1 DEPENDENCIES

=over

=item L<Carp>

This uses Carp to report errors.

=item L<Scalar::Util>

Scalar::Util is used to distinguish strings from numbers.

=back

=head1 BUGS

=cut

package JSON::Create::PP;
use parent Exporter;
our @EXPORT_OK = qw/create_json/;
our %EXPORT_TAGS = (all => \@EXPORT_OK);
use warnings;
use strict;
use utf8;
use Carp;
use Scalar::Util qw/looks_like_number blessed reftype/;

# http://stackoverflow.com/questions/1185822/how-do-i-create-or-test-for-nan-or-infinity-in-perl#1185828

sub isinf { $_[0]==9**9**9 || $_[0]==-9**9**9 }
sub isnan { ! defined( $_[0] <=> 9**9**9 ) }

sub escape_all_unicode
{
    my ($input) = @_;
    $input =~ s/([\x{007f}-\x{ffff}])/sprintf ("\\u%04", ord ($1))/ge;
    $input =~ s/([\x{10000}-\x{10ffff}])/
    sprintf ("\\u%04x", 0xD800 | ((ord ($1) >>10) & 0x3ff)) .
    sprintf ("\\u%04x", 0xDC00 | ((ord ($1)) & 0x3ff))
    /gex;
    return $input;
}

sub stringify
{
    my ($obj, $input) = @_;
    $input =~ s/("|\\)/\\$1/g;
    $input =~ s/\x08/\\b/g;
    $input =~ s/\f/\\f/g;
    $input =~ s/\n/\\n/g;
    $input =~ s/\r/\\r/g;
    $input =~ s/\t/\\t/g;
    $input =~ s/([\x00-\x1f])/sprintf ("\\u%04x", ord ($1))/ge;
    if ($obj->{unicode_escape_all}) {
	$input = escape_all_unicode ($input);
    }
    return "\"$input\"";
}

sub create_json_recursively
{
    my ($obj, $input) = @_;
    my $output = '';
    my $ref;
    if ($obj->{_handlers} || $obj->{obj_handler}) {
	$ref = ref ($input);
    }
    else {
	$ref = reftype ($input);
    }
    if ($ref) {
	if ($ref eq 'HASH') {
	    $output .= '{';
	    my @v;
	    for my $k (keys %$input) {
		my $item = stringify ($obj, $k);
		$item .= ':';
		$item .= create_json_recursively ($obj, $input->{$k});
		push @v, $item;
	    }
	    $output .= join (',', @v);
	    $output .= '}';
	}
	elsif ($ref eq 'ARRAY') {
	    $output .= '[';
	    my @v;
	    for my $k (@$input) {
		push @v, create_json_recursively ($obj, $k);
	    }
	    $output .= join (',', @v);
	    $output .= ']';
	}
	elsif ($ref eq 'SCALAR') {
	    $output .= stringify ($obj, $$input);
	}
	else {
	    if (blessed ($input)) {
		my $handler = $obj->{_handlers}{$ref};
		if ($handler) {
		    $output .= &{$handler} ($input);
		}
		else {
		    die "$ref cannot be serialized.\n";
		}
	    }
	    else {
		if ($obj->{_type_handler}) {
		    $output .= &{$obj->{_type_handler}} ($input);
		}
		else {
		    die "$ref cannot be serialized.\n";
		}
	    }	    
	}	
    }
    else {
	if (looks_like_number ($input)) {
	    if (isnan ($input)) {
		$output .= '"nan"';
	    }
	    elsif (isinf ($input)) {
		if ($input < 0) {
		    $output .= '"-inf"';
		}
		else {
		    $output .= '"inf"';
		}
	    }
	    else {
		$output .= $input;
	    }
	}
	elsif (! defined $input) {
	    $output .= 'null';
	}
	else {
	    $output .= stringify ($obj, $input);
	}
    }
    return $output;
}

sub create_json
{
    my ($input, %options) = @_;
    my $output;
    eval {
	$output = create_json_recursively ({}, $input);
    };
    if ($@) {
	warn $@;
	return undef;
    }
    return $output;
}

sub new
{
    return bless {
	_handlers => {},
    };
}

sub get_handlers
{
    my ($obj) = @_;
    return $obj->{_handlers};
}

sub obj
{
    my ($obj, %things) = @_;
    my $handlers = $obj->get_handlers ();
    for my $k (keys %things) {
	$handlers->{$k} = $things{$k};
    }
}

sub bool
{
    my ($obj, @list) = @_;
    my $handlers = $obj->get_handlers ();
    for my $k (@list) {
	$handlers->{$k} = 'bool';
    }
}

sub run
{
    my ($obj, $input) = @_;
    my $json = create_json_recursively ($obj, $input);
    return $json;
}

sub type_handler
{
    my ($obj, $handler) = @_;
    $obj->{_type_handler} = $handler;
}

1;
