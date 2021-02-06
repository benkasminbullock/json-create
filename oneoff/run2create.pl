#!/home/ben/software/install/bin/perl
use Z;
chdir $Bin or die $!;
use File::Versions 'make_backup';
for my $file (<*.t>) {
my $text = read_text ($file);
$text =~ s!->run!->create!g;
make_backup ($file);
write_text ($file, $text);
}
