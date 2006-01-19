#!/usr/bin/perl

my $currentGroup = "";

while (<>)
{
    chomp;
    if ( /^\[/ )
    {
        $currentGroup = $_;
        next;
    }
    ($key, $value) = ($_ =~ /([^=]+)=[ \t]*([^\n]+)/);
    if ($_ =~ /TabPlacement/)
    {
        print("# DELETE $currentGroup$key\n");
        if ($value eq "0")
        {
            print("$key=Top\n");
        }
    }
}
