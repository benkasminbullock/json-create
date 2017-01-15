#!/usr/bin/env perl
use warnings;
use strict;
use Perl::Build;
use FindBin '$Bin';
perl_build (
    make_pod => "$Bin/make-pod.pl",
#    pod => ['lib/JSON/Create.pod',],
    c => [
    {
	dir => '/home/ben/projects/unicode-c',
	stems => ['unicode',],
    },
    ],
);
exit;
