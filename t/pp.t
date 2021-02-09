# Test the pure-Perl version of the module using the same tests as the
# XS version.

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

if ($ENV{JSONCreatePP}) {
    # What this test does is to run all the tests in t/, except with
    # the XS switched off, so if the environment variable is true, we
    # are running this test from itself, so quit.
    plan skip_all => "Running pp.t as a sub-test of pp.t";
}

$ENV{JSONCreatePP} = 1;
my $dir = "$Bin/..";
chdir $dir or die $!;
system ("$dir/build.pl");
my $status = system ("prove -I $dir/blib/arch -I $dir/blib/lib $dir/t/*.t");
ok ($status == 0, "passed tests");
# This causes failures during ./build.pl -p
#system ("$dir/build.pl -c");
done_testing ();
