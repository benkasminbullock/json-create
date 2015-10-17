# This is a test for module JSON::Create.

use warnings;
use strict;
use Test::More;
use JSON::Create 'create_json';
use JSON::Parse 'valid_json';
my %hash = ('a' => 'b');
my $json_hash = create_json (\%hash);
is ($json_hash, '{"a":"b"}', "json simple hash OK");
my $hashhash = {a => {b => 'c'}, d => {e => 'f'}};
my $json_hashhash = create_json ($hashhash);
ok (valid_json ($json_hashhash), "json nested hash valid");
like ($json_hashhash, qr/"a":{"b":"c"}/, "json nested hash OK part 1");
like ($json_hashhash, qr/"d":{"e":"f"}/, "json nested hash OK part 2");

# Arrays

my $array = ['there', 'is', 'no', 'other', 'day'];
my $json_array = create_json ($array);
is ($json_array, '["there","is","no","other","day"]', "flat array JSON correct");
my $nested_array = ['let\'s', ['try', ['it', ['another', ['way']]]]];
my $json_nested_array = create_json ($nested_array);
ok (valid_json ($json_nested_array), "Nested array JSON valid");
is ($json_nested_array, '["let\'s",["try",["it",["another",["way"]]]]]', "Nested array JSON correct");

my $rx = qr/See+ Emily play/;
my $rx_json = create_json ($rx);
ok (valid_json ($rx_json), "regex JSON valid");
# "stringified" regexes are different on different Perls.
# http://www.cpantesters.org/cpan/report/bf3447c2-744c-11e5-9c9f-1c89e0bfc7aa
like ($rx_json, qr/See\+ Emily play/, "regex JSON as expected");

my $numbers = [1,2,3,4,5,6];
my $numbers_json = create_json ($numbers);
is ($numbers_json, '[1,2,3,4,5,6]', "simple integers");
my $fnumbers = [0.5,0.25];
my $fnumbers_json = create_json ($fnumbers);
is ($fnumbers_json, '[0.5,0.25]', "round floating point numbers");

#my $code = sub {print "She's often inclined to borrow somebody's dreams till tomorrow"};
#print $code;
#my $json_code = create_json ($code);
#print $json_code;

done_testing ();
# Local variables:
# mode: perl
# End:
