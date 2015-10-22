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

done_testing ();
exit;
