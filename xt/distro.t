use warnings;
use strict;
use Test::More;
use FindBin '$Bin';
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

done_testing ();
