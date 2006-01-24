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
    if ($value =~ /([\da-fA-F]{2})([\da-fA-F]{2})([\da-fA-F]{2})$/)
    {
        print("# DELETE $currentGroup$key\n");
        printf("$key=%d,%d,%d\n",hex($1),hex($2),hex($3));
    }
}
