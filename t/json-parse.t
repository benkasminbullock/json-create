# This tests the boolean round-trip behaviour of JSON::Parse and
# JSON::Create.

use FindBin '$Bin';
use lib "$Bin";
use JCT;
use JSON::Parse '0.60', 'parse_json';

my $skip_true_test;
if ($ENV{JSONCreatePP} && "$]" =~ m!5\.016!) {
    note ("Skipping tests of true with PP and Perl $]");
    $skip_true_test = 1;
}

my $jsonin = '{"hocus":true,"pocus":false,"focus":null}';
my $p = parse_json ($jsonin);
my $jc = JSON::Create->new ();
my $out = $jc->create ($p);
like ($out, qr/"pocus":false/);
like ($out, qr/"focus":null/);
my $json_array = '[true,false,null]';
my $q = parse_json ($json_array);
my $outq = $jc->create ($q);

# http://matrix.cpantesters.org/?dist=JSON-Create+0.30_05

SKIP: {
    if ($skip_true_test) {
	skip "Unknown problem with Perl 5.16 & JSON::Parse & true & PP ", 2;
    }
    like ($out, qr/"hocus":true/); # true bad
    is ($outq, $json_array, "in = out"); # true bad
};

done_testing ();
