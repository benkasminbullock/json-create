#!/home/ben/software/install/bin/perl
use warnings;
use strict;
use Perl::Build;
perl_build (
    pod => ['lib/JSON/Create.pod',],
    c => [
    {
	dir => '/home/ben/projects/unicode-c',
	stems => ['unicode',],
    },
    ],
);
exit;
