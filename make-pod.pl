#!/usr/bin/env perl
use warnings;
use strict;
use Template;
use FindBin '$Bin';
use Perl::Build qw/get_version get_info get_commit/;
use Perl::Build::Pod ':all';
use Getopt::Long;
use File::Slurper qw!read_text write_text!;
my $ok = GetOptions (
    'force' => \my $force,
    'verbose' => \my $verbose,
);
if (! $ok) {
    usage ();
    exit;
}
my %pbv = (base => $Bin);
my $info = get_info (%pbv);
my $version = get_version (%pbv);
my $commit = get_commit (%pbv);
# Names of the input and output files containing the documentation.

my $mod = 'Create';
my $pod = "$mod.pod";
my $input = "$Bin/lib/JSON/$pod.tmpl";
my $output = "$Bin/lib/JSON/$pod";

my $pm = "$Bin/lib/JSON/$mod.pm";

my $setvar = setvar ($pm);

# Template toolkit variable holder

my %vars = (
    commit => $commit,
    info => $info,
    setvar => $setvar,
    version => $version,
);

my $tt = Template->new (
    ABSOLUTE => 1,
    INCLUDE_PATH => [
	$Bin,
	pbtmpl (),
	"$Bin/examples",
    ],
    ENCODING => 'UTF8',
    FILTERS => {
        xtidy => [
            \& xtidy,
            0,
        ],
    },
    STRICT => 1,
);

make_examples ("$Bin/examples", $verbose, $force);

$tt->process ($input, \%vars, $output, binmode => 'utf8')
    or die '' . $tt->error ();

exit;

sub usage
{
print <<EOF;
--verbose
--force
EOF
}

sub setvar
{
    my ($pm) = @_;
    my $text = read_text ($pm);
    my $set = $text;
    $set =~ s/^.*(sub\s+set)/$1/gs;
    $set =~ s/^}.*$//gsm;
    my @setvar;
    while ($set =~ m!if \(\$k eq '(\w+)'!g) {
	push @setvar, $1;
    }
    my @s = sort @setvar;
    for my $i (0..$#s) {
	die "Unsorted set variable $setvar[$i]" unless $s[$i] eq $setvar[$i];
    }
    return \@setvar;
}
