use strict;
use warnings;
use ExtUtils::MakeMaker;

my $pm = 'lib/JSON/Create.pm';
my $pod = 'lib/JSON/Create.pod';
my $github = 'github.com/benkasminbullock/json-create';
my $repo = "https://$github";

# This methodology was poached from
#
# https://metacpan.org/source/MIYAGAWA/App-cpanminus-1.7039/Makefile.PL#L1
# 
# due to
#
# http://www.cpantesters.org/cpan/report/bbe5eb4b-6bf3-1014-b983-90639bcee8ff
#

my $tr = {
    'Test::More' => '0',
    'JSON::Parse' => '0.35',
};

my %WriteMakefileArgs = (
    NAME => 'JSON::Create',
    VERSION_FROM => $pm,
    ABSTRACT_FROM => $pod,
    AUTHOR => 'Ben Bullock <bkb@cpan.org>',
    LICENSE => 'perl',
    TEST_REQUIRES => $tr,
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
    OBJECT => 'Create.o unicode.o',
);

my %FallbackPrereqs = (
    "ExtUtils::MakeMaker" => 0,
    %$tr,
);
 
unless ( eval { ExtUtils::MakeMaker->VERSION(6.63_03) } ) {
    delete $WriteMakefileArgs{TEST_REQUIRES};
    $WriteMakefileArgs{PREREQ_PM} = \%FallbackPrereqs;
}
 
WriteMakefile(%WriteMakefileArgs);
