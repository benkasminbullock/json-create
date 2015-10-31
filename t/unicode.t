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

# Test it with Perl's unicode flag switched on everywhere.

use utf8;

my %unihash = (
    'う' => '雨',
    'あ' => '亜',
);

my $out = create_json (\%unihash);
ok ($out, "Got output from Unicode hash");
ok (utf8::is_utf8 ($out), "Output is marked as Unicode");
# Key/value pairs may be in either order, so we have to use "like"
# to test the key / value pairs.
like ($out, qr/"う":"雨"/, "key / value pair u");
like ($out, qr/"あ":"亜"/, "key / value pair a");

# Now test the other option, switch off the Perl unicode flag and
# check that it still works.

no utf8;

my %nonunihash = (
    'う' => '雨',
    'あ' => '亜',
);

my $nonuout = create_json (\%nonunihash);
ok ($nonuout, "Got output from unmarked Unicode hash");
ok (! utf8::is_utf8 ($nonuout), "Output is not marked as Unicode");
# Key/value pairs may be in either order, so we have to use "like"
# to test the key / value pairs.
like ($nonuout, qr/"う":"雨"/, "key / value pair u");
like ($nonuout, qr/"あ":"亜"/, "key / value pair a");

# There is a bug here, that we must actually validate all the UTF-8 in
# the code when the Perl flag is switched off, otherwise we may
# produce invalid JSON.

#TODO: {
#    local $TODO = 'implement default escaping of U+2028 and U+2029';
{
    my $jc = JSON::Create->new ();

    my $in2028 = "\x{2028}";
    my $out2028 = $jc->run ($in2028);
    is ($out2028, '"\u2028"', "default JS protection");

    my $in2029 = "\x{2029}";
    my $out2029 = $jc->run ($in2029);
    is ($out2029, '"\u2029"', "default JS protection");

    $jc->no_javascript_safe (1);

    my $out2028a = $jc->run ($in2028);
    is ($out2028a, "\"\x{2028}\"", "switch off JS protection");

    my $out2029a = $jc->run ($in2029);
    is ($out2029a, "\"\x{2029}\"", "switch off JS protection");

    $jc->no_javascript_safe (0);

    my $out2028b = $jc->run ($in2028);
    is ($out2028b, '"\u2028"', "switch on JS protection");

    my $out2029b = $jc->run ($in2029);
    is ($out2029b, '"\u2029"', "switch on JS protection");
};

#TODO: {
#    local $TODO = 'implement unicode_escape_all';
{
    my $jc = JSON::Create->new ();
    $jc->unicode_escape_all (1);
    $jc->unicode_upper (0);

    use utf8;

    my $in = '赤ブöＡↂϪ';
    my $out = $jc->run ($in);
    is ($out, '"\u8d64\u30d6\u00f6\uff21\u2182\u03ea"', "Unicode escaping");

    $jc->unicode_upper (1);

    my $out2 = $jc->run ($in);
    is ($out2, '"\u8D64\u30D6\u00F6\uFF21\u2182\u03EA"',
	"Upper case hex unicode");
};

#TODO: {
#    local $TODO = 'correctly generate surrogate pairs';
{
    my $jc = JSON::Create->new ();
    $jc->unicode_escape_all (1);
    $jc->unicode_upper (0);

    # These are exactly the same examples as in "unicode.c", please
    # see that code for links to where the examples come from.

    my $wikipedia_1 = "\x{10437}";
    my $out_1 = $jc->run ($wikipedia_1);
    is ($out_1, '"\ud801\udc37"', "surrogate pair wiki 1");

    my $wikipedia_2 = "\x{24b62}";
    my $out_2 = $jc->run ($wikipedia_2);
    is ($out_2, '"\ud852\udf62"', "surrogate pair wiki 2");

    my $json_spec = "\x{1D11E}";
    my $out_3 = $jc->run ($json_spec);
    is ($out_3, '"\ud834\udd1e"', "surrogate pair json spec");
};

done_testing ();

exit;
