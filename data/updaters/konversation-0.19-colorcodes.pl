#! /usr/bin/perl

use strict;

my $currentGroup = "";
my $key;
my $value;

while (<>)
{
    chomp;
    if ( /^\[/ )
    {
        $currentGroup = $_;
        next;
    }
    ($key, $value) = ($_ =~ /([^=]+)=[ \t]*([^\n]+)/);
    if ($_ =~ /FilterColorCodes/)
    {
        print("# DELETE $currentGroup$key\n");
        if ($value eq "true")
        {
            print("AllowColorCodes=false\n");
        }
    }
}
