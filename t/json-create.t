# This is a test for module JSON::Create.

use warnings;
use strict;
use Test::More;
use JSON::Create 'create_json';
my %hash = ('a' => 'b');
my $json_hash = create_json (\%hash);
is ($json_hash, '["a":"b"]');
done_testing ();
# Local variables:
# mode: perl
# End:
