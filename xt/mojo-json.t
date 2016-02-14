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
use JSON::Create;

my $jsonin = '{"hocus":true,"pocus":false}';

SKIP: {
    eval {
        require Mojo::JSON;

        # It seems like "require Mojo::JSON" can succeed but then
        # "Mojo::JSON::decode_json" can fail:

        # http://www.cpantesters.org/cpan/report/aecc9dff-6bf5-1014-9443-909d2cb3cc3d

        # Old versions don't have "decode_json" at all:

        # https://metacpan.org/pod/release/SRI/Mojolicious-4.27/lib/Mojo/JSON.pm

        Mojo::JSON::decode_json ('[true]');
    };
    if ($@) {
        skip "Mojo::JSON is not installed.\n", 4;
    }
    my $sri = JSON::Create->new ();
    $sri->bool ('JSON::PP::Boolean', 'Mojo::JSON::_Bool');
    my $mojo = {
        'mojolicio' => Mojo::JSON::true (),
        'us' => Mojo::JSON::false (),
    };
    my $austinpowers = $sri->run ($mojo);
    like ($austinpowers, qr/"mojolicio":true/, "Mojo::JSON::true");
    like ($austinpowers, qr/"us":false/, "Mojo::JSON::false");
    # Test round trip
    my $mjhp = $sri->run (Mojo::JSON::decode_json ($jsonin));
    like ($mjhp, qr/"hocus":true/);
    like ($mjhp, qr/"pocus":false/);
};

done_testing ();
