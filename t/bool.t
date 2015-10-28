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

package Ba::Bi::Bu::Be::Bo;

sub new
{
    my $lion = 'ライオン';
    return bless \$lion;
}

package main;

my $babibubebo = Ba::Bi::Bu::Be::Bo->new ();
my $jc = JSON::Create->new ();
$jc->set_handlers ({'Ba::Bi::Bu::Be::Bo' => 'bool', 'tosspot' => 1});
my $thing = {monkey => $babibubebo};
print $jc->run ($thing), "\n";
ok (1);
done_testing ();
