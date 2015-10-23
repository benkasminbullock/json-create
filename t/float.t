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
use JSON::Parse 'valid_json';

# http://www.perlmonks.org/?node_id=703222

sub double_from_hex
{
    unpack 'd', scalar reverse pack 'H*', $_[0] 
}

use constant POS_INF => double_from_hex '7FF0000000000000';
use constant NEG_INF => double_from_hex 'FFF0000000000000';

use constant qNaN    => double_from_hex '7FF8000000000000';
use constant NaN     => qNaN;

use JSON::Create 'create_json';

#my $nan = 'nan';
my $nan = NaN;
my $bread = {
    'curry' => NaN,
    'rice' => POS_INF,
    'lice' => NEG_INF,
};
my $nanbread = create_json ($bread);
note ($nanbread);
ok (valid_json ($nanbread));
done_testing ();
