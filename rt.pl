#!/home/ben/software/install/bin/perl

# Experimental "round-trip" script.

use warnings;
use strict;
use utf8;
use FindBin '$Bin';
use Path::Tiny;
use JSON::Parse qw/parse_json assert_valid_json/;
use JSON::Create 'create_json';
for my $file (@ARGV) {
    my $in = path($file);
    my $j;
    eval {
	$j = parse_json ($in->slurp_utf8);
    };
    if ($@) {
	warn "parse_json $file failed: $@\n";
    }
    else {
	my $out = create_json ($j);
	assert_valid_json ($out);
	print $out;
    }
}
