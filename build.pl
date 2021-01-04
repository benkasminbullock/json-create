#!/home/ben/software/install/bin/perl
use warnings;
use strict;
use FindBin '$Bin';
use File::Copy;
use lib "$Bin/copied/lib";
use Perl::Build;

my %build = (
    make_pod => "$Bin/make-pod.pl",
#    pod => ['lib/JSON/Create.pod',],
    c => [
    {
	dir => '/home/ben/projects/unicode-c',
	stems => ['unicode',],
    },
    ],
);

if ($ENV{CI}) {
    delete $build{c};
    $build{verbose} = 1;
    $build{no_make_examples} = 1;
    for my $e (qw!c h!) {
        my $file = "unicode.$e";
        my $ofile = "$Bin/$file";
        if (-f $ofile) {
            chmod 0644, $ofile or die $!;
        }
        copy ("$Bin/copied/unicode/$file", $ofile);
        chmod 0444, $ofile or die $!;
    }
}

perl_build (%build);
exit;
