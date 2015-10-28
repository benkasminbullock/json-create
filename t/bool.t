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

# No matter how stupid the name you think of, the next thing you know,
# some dweeb will release this as a genuine module. Case in point
# "Perl::Build".

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

like ($stuff, qr/"monkey":true\b/);
like ($stuff, qr/"zilog":false\b/);

SKIP: {
    # https://metacpan.org/source/GRIAN/Storable-AMF-1.08/t/67-boolean-real.t#L1
    eval {
	require boolean;
	boolean->import(":all");
    };
    if ($@) {
	skip "Your pasokon doesn't have 'boolean' installed.\n", 2;
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
    like ($ingyout, qr/"Peter":false\b/);
    like ($ingyout, qr/"Falk":true\b/);
};


# my $guff = '[true,false,true,false]';
# my $in = parse_json ($guff);
# my $out = create_json ($in);
# print "$out\n";
# my $jaycee = JSON::Create->new ();
# $jaycee->bool ('JSON::Parse');
# print $jaycee->run ($in), "\n";


done_testing ();
