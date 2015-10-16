use strict;
use warnings;
use ExtUtils::MakeMaker;

my $pm = 'lib/JSON/Create.pm';
my $github = 'github.com/benkasminbullock/json-create';
my $repo = "https://$github";

WriteMakefile (
    NAME => 'JSON::Create',
    VERSION_FROM => $pm,
    ABSTRACT_FROM => $pm,
    AUTHOR => 'Ben Bullock <bkb@cpan.org>',
    LICENSE => 'perl',
    PREREQ_PM => {
	'JSON::Parse' => '0.33',
    },
    META_MERGE => {
	'meta-spec' => {
	    version => 2,
	},
        resources => {
            repository => {
		type => 'git',
		url => "git://$github.git",
		web => $repo,
	    },
            bugtracker => {
		web => "$repo/issues",
	    },
#	    homepage => 'http://',
        },
    },
    MIN_PERL_VERSION => '5.006001',
    OBJECT => 'Create.o',
);
