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
    if ($_ =~ /UseKdeDefault/)
    {
        print("# DELETE $currentGroup$key\n");
        if ($value eq "false")
        {
            print("UseCustomBrowser=true\n");
        }
    }
}
