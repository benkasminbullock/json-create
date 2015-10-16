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

# Arrays

my $array = ['there', 'is', 'no', 'other', 'day'];
my $json_array = create_json ($array);
is ($json_array, '["there","is","no","other","day"]');
my $nested_array = ['let\'s', ['try', ['it', ['another', ['way']]]]];
my $json_nested_array = create_json ($nested_array);
ok (valid_json ($json_nested_array));
is ($json_nested_array, '["let\'s",["try",["it",["another",["way"]]]]]');

my $rx = qr/See+ Emily play/;
my $rx_json = create_json ($rx);
is ($rx_json, '"(?^:See+ Emily play)"');

my $numbers = [1,2,3,4,5,6];
my $numbers_json = create_json ($numbers);
is ($numbers_json, '[1,2,3,4,5,6]');

done_testing ();
# Local variables:
# mode: perl
# End:
