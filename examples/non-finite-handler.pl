#!/home/ben/software/install/bin/perl
use warnings;
use strict;
use JSON::Create;
my $bread = { 'curry' => -sin(9**9**9) };
my $jcnfh = JSON::Create->new ();
$jcnfh->non_finite_handler(sub { return 'null'; });
my $jcout = $jcnfh->run ($bread);
print "$jcout\n";
