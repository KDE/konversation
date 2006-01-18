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
    $value =~ /(..)(..)(..)/;
    print("# DELETE $currentGroup$key\n");
    printf("$key=%d,%d,%d\n",hex($1),hex($2),hex($3));
}