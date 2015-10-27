#!/home/ben/software/install/bin/perl
use warnings;
use strict;
use JSON::Create 'create_json';
my %example = (
    down => {
	and => {
	    down => {
		and => {
		    down => {
			and => {
			    down => 'we go'
			}
		    }
		}
	    }
	}
    },
);
print create_json (\%example);
