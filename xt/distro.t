use warnings;
use strict;
use Test::More;
use FindBin '$Bin';
use Perl::Build qw/get_info $badfiles/;
my $file = "$Bin/../Makefile.PL";
open my $in, "<", $file or die $!;
while (<$in>) {
    if (/-Wall/) {
	like ($_, qr/^\s*#/, "Commented out -Wall in Makefile.PL");
    }
    if (/CC/) {
	like ($_, qr/^\s*#/, "Commented out CC in Makefile.PL");
    }
}
close $in or die $!;

my $info = get_info (base => "$Bin/..");
my $name = $info->{name};
my $version = $info->{version};
my $distrofile = "$Bin/../$name-$version.tar.gz";
if (! -f $distrofile) {
    die "No $distrofile";
}
my @tgz = `tar tfz $distrofile`;
my $has_collide_pl;
my %badfiles;
my %files;
for (@tgz) {
    if (/collide\.pl/) {
	$has_collide_pl = 1;
    }
    if (/$badfiles/) {
	$files{$1} = 1;
	$badfiles{$1} = 1;
    }
}
ok (! $files{".tmpl"}, "no templates in distro");
ok (! $files{"-out.txt"}, "no out.txt in distro");
ok (! $files{"make-pod.pl"}, "no make-pod.pl in distro");
ok (! $files{"build.pl"}, "no build.pl in distro");
ok (keys %badfiles == 0, "no bad files");

done_testing ();
