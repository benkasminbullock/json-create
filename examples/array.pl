#!/home/ben/software/install/bin/perl
use warnings;
use strict;
use utf8;
use JSON::Create 'create_json';
my @array = (1, 2, 2.5, 'Acme', 
	 {
	     cats => [qw/mocha dusty milky/], 
	     dogs => [qw/Tico Rocky Pinky/],
	 });
print create_json (\@array);
