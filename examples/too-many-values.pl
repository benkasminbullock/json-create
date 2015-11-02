#!/home/ben/software/install/bin/perl
use warnings;
use strict;
use JSON::Create;
my $jc = JSON::Create->new ();
$jc->validate (1);
$jc->type_handler (sub {
		       return (1, 2, 3);
		   });
print $jc->run ({ x => *STDOUT }); 

