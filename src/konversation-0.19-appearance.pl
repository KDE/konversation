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
    if ($_ =~ /IRCColors/)
    {
        print("# DELETE $currentGroup$key\n");
        $i = 0;
        @irccolors = split(/,/, $value);
        foreach $out (@irccolors)
        {
            $out =~ /([\da-fA-F]{2})([\da-fA-F]{2})([\da-fA-F]{2})$/;
            printf("IrcColorCode$i=%d,%d,%d\n",hex($1),hex($2),hex($3));
            $i++;
        }
    }
    elsif ($_ =~ /NickColors/)
    {
        print("# DELETE $currentGroup$key\n");
        $i = 0;
        @nickcolors = split(/,/, $value);
        foreach $out (@nickcolors)
        {
            $out =~ /([\da-fA-F]{2})([\da-fA-F]{2})([\da-fA-F]{2})$/;
            printf("NickColor$i=%d,%d,%d\n",hex($1),hex($2),hex($3));
            $i++;
        }
    }
}
