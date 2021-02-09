package JSON::Create::Bool;

use warnings;
use strict;

our @ISA = qw!Exporter!;
our @EXPORT = qw!true false!;
our $VERSION = $JSON::Create::VERSION;

my $t = 1;
my $f = 0;
my $true  = bless \$t, __PACKAGE__;
my $false = bless \$f, __PACKAGE__;

sub true
{
    return $true;
}

sub false
{
    return $false;
}

1;
