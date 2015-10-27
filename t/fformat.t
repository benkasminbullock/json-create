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

my $jc = JSON::Create->new ();
my @array = (10_000_000.0, 1_000_000.0, 100_000.0, 10_000.0, 1000.0, 100.0,10.0,1.0,0.1,0.01,0.001,0.0001,0.000_01,0.000_001,0.000_000_1,0.000_000_01);
$jc->set_fformat ('%f');
my $fout = $jc->run (\@array);
print "$fout\n";
$jc->set_fformat ('%e');
my $eout = $jc->run (\@array);
print "$eout\n";
$jc->set_fformat ('%2.4g');
my $gout = $jc->run (\@array);
print "$gout\n";
$jc->set_fformat ('%2.10g');
my $glout = $jc->run (\@array);
print "$glout\n";

done_testing ();
