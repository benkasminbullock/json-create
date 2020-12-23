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
my $x = "$Bin/test-qsort-r";
my $c = "$x.c";
die unless -f $c;
rmfile ();
my $compile = "cc -o $x $c";
my $status = system ($compile);
cmp_ok ($status, '==', 0, "Compiled test file");
my $out = `$x`;
ok (length ($out) == 0, "no output from test file");
done_testing ();
rmfile ();
exit;
sub rmfile 
{
    if (-f $x) {
	unlink $x or die $!;
    }
}
