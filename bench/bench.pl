#!/home/ben/software/install/bin/perl
use warnings;
use strict;
use utf8;
use Benchmark ':all';
use lib '/home/ben/projects/json-create/blib/lib';
use lib '/home/ben/projects/json-create/blib/arch';
use JSON::Create 'create_json';
use JSON::XS;

use FindBin '$Bin';
my $stuff = {
    captain => 'planet',
    he => "'s",
    a => 'hero',
    gonna => 'take',
    pollution => 'down',
    to => 'zero',
    "he's" => 'our',
    powers => 'magnified',
    and => "he's",
    fighting => 'on',
    the => "planet's",
    side => "Captain Planet!",
};

my $count = 1000000;

cmpthese (
    $count,
    {
	'JC' => sub {
	    create_json ($stuff);
	},
	'JX' => sub {
	    my $json = JSON::XS->new ();
	    $json->encode ($stuff);
	},
    },    
);
