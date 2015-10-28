use warnings;
use strict;
use utf8;
use FindBin '$Bin';
use Test::More;
my $builder = Test::More->builder;
binmode $builder->output,         ":utf8";
binmode $builder->failure_output, ":utf8";
binmode $builder->todo_output,    ":utf8";
binmode STDOUT, ":encoding(utf8)";
binmode STDERR, ":encoding(utf8)";
use JSON::Create 'create_json';
use JSON::Parse 'parse_json';

# A name unlikely to clash with a real module.

package Ba::Bi::Bu::Be::Bo;

sub new
{
    my $lion = 'ライオン';
    return bless \$lion;
}

# Bogus false object for testing.

sub false
{
    my $lion;
    return bless \$lion;
}

package main;

my $babibubebo = Ba::Bi::Bu::Be::Bo->new ();
my $z80 = Ba::Bi::Bu::Be::Bo->false ();
my $jc = JSON::Create->new ();
$jc->bool (qw/Ba::Bi::Bu::Be::Bo/);
my $thing = {monkey => $babibubebo, zilog => $z80,};
my $stuff = $jc->run ($thing);

like ($stuff, qr/"monkey":true\b/, "Self-created true");
like ($stuff, qr/"zilog":false\b/, "Self-created false");

SKIP: {
    # https://metacpan.org/source/GRIAN/Storable-AMF-1.08/t/67-boolean-real.t#L1
    eval {
	require boolean;
	boolean->import(":all");
    };
    if ($@) {
	skip "boolean is not installed.\n", 2;
    }
    my $ingy = JSON::Create->new ();
    $ingy->bool ('boolean');

    # We cannot use "true" and "false" if we don't "use boolean;", so
    # we need to add these () after the values. I got this from
    # https://metacpan.org/source/GRIAN/Storable-AMF-1.08/t/67-boolean-real.t#L39

    my $dotnet = {
	'Peter' => boolean::false(),
	'Falk' => boolean::true(),
    };
    my $ingyout = $ingy->run ($dotnet);
    like ($ingyout, qr/"Peter":false\b/, "boolean false");
    like ($ingyout, qr/"Falk":true\b/, "boolean true");
};


SKIP: {
    eval {
	require JSON::Tiny;
    };
    if ($@) {
	skip "JSON::Tiny is not installed.\n", 2;
    }
    my $davido = JSON::Create->new ();
    $davido->bool ('JSON::Tiny::_Bool');
    my $minij = {
	'salt' => JSON::Tiny::true(),
	'lake' => JSON::Tiny::false(),
    };
    my $saltlake = $davido->run ($minij);
    like ($saltlake, qr/"salt":true/, "JSON::Tiny true");
    like ($saltlake, qr/"lake":false/, "JSON::Tiny false");
};

SKIP: {
    eval {
	require JSON::PP;
    };
    if ($@) {
	skip "JSON::PP is not installed.\n", 2;
    }
    my $makamaka = JSON::Create->new ();
    $makamaka->bool ('JSON::PP::Boolean');
    my $pp = {
	'don' => $JSON::PP::true,
	'zoko' => $JSON::PP::false,
    };
    my $ppout = $makamaka->run ($pp);
    like ($ppout, qr/"don":true/, "JSON::PP true");
    like ($ppout, qr/"zoko":false/, "JSON::PP false");
};


# my $guff = '[true,false,true,false]';
# my $in = parse_json ($guff);
# my $out = create_json ($in);
# print "$out\n";
# my $jaycee = JSON::Create->new ();
# $jaycee->bool ('JSON::Parse');
# print $jaycee->run ($in), "\n";


done_testing ();
