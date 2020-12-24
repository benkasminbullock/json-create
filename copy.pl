#!/home/ben/software/install/bin/perl

# The build process of this module relies on lots of modules which are
# not on CPAN. To make it possible for people to use the repository
# from github, and in order to allow continuous integration, copy the
# local modules into the git repo.

use warnings;
use strict;
use utf8;
use FindBin '$Bin';
use Sys::Hostname;
use File::Slurper 'read_text';
my $host = hostname ();
if ($host ne 'mikan') {
    exit;
}

my $pbdir = "/home/ben/projects/perl-build";
my $ddir = "/home/ben/projects/deploy";
my $cmdir = "/home/ben/projects/c-maker";
my @libs = ($pbdir, $ddir, $cmdir);
my $copied = "$Bin/copied";
my $lib = "$copied/lib";
my $verbose;
#my $verbose = 1;
if ($verbose) {
    warn "Verbose messages are on";
}

# This is not a smart thing to do, if $lib happens to contain a typo
# we may end up using rm from $HOME, but it is necessary in some
# cases.

do_system ("cd $copied || exit;rm -rf ./lib", $verbose);
do_system ("mkdir -p $lib", $verbose);
for my $dir (@libs) {
    die unless -d $dir;
    copy_those_files ($dir, $verbose);
}
#do_system ("git add $lib; git commit -m 'copied files'", $verbose);
exit;

sub copy_those_files
{
    my ($dir, $verbose) = @_;
    my $files = `cd $dir/lib; find . -name "*"`;
    my @files = split /\n/, $files;
    @files = grep !/\.pod(\.tmpl)?$/, @files;
    @files = grep !m!(/|^\.+)$!, @files;
    @files = grep !m!\.~[0-9+]~!, @files;
    @files = map {s!^\./!!r} @files;
    print "@files\n";

    for my $file (@files) {
#	print "$file\n";
#	next;
	my $infile = "$dir/lib/$file";
	my $ofile = "$lib/$file";
	if (-d "$dir/lib/$file") {
	    do_system ("mkdir -p $ofile", $verbose);
	    next;
	}
	if (-f $ofile) {
	    chmod 0644, $ofile or die $!;
	}
	if ($verbose) {
	    print "Copying $infile to $ofile.\n";
	}
	my $text = read_text ($infile);
	open my $out, ">:encoding(utf8)", $ofile or die "Can't open $ofile: $!";
	if ($file =~ /\.pm$/) {
	    if ($verbose) {
		print "Adding header.\n";
	    }
	    print $out <<EOF;
# Copied from $dir/lib/$file
EOF
	}
	print $out $text;
	close $out or die $!;
	chmod 0444, $ofile;
    }
}


sub do_system
{
    my ($command, $verbose) = @_;
    if ($verbose) {
	print "$command\n";
    }
    my $status = system ($command);
    if ($status != 0) {
	die "Error doing '$command'";
    }
}
