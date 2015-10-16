# This is a test for module JSON::Create.

use warnings;
use strict;
use Test::More;
use JSON::Create 'create_json';
use JSON::Parse 'valid_json';
my %hash = ('a' => 'b');
my $json_hash = create_json (\%hash);
is ($json_hash, '{"a":"b"}');
my $hashhash = {a => {b => 'c'}, d => {e => 'f'}};
my $json_hashhash = create_json ($hashhash);
ok (valid_json ($json_hashhash));
like ($json_hashhash, qr/"a":{"b":"c"}/);
like ($json_hashhash, qr/"d":{"e":"f"}/);
done_testing ();
# Local variables:
# mode: perl
# End:
