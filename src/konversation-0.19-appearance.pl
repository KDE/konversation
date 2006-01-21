#! /usr/bin/perl

use strict;

my $currentGroup = "";
my $key;
my $value;
my $i;
my $out;
my @irccolors;
my @nickcolors;

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
        if ($value ne "#ffffff,#000000,#000080,#008000,#ff0000,#a52a2a,#800080,#ff8000,#808000,#00ff00,#008080,#00ffff,#0000ff,#ffc0cb,#a0a0a0,#c0c0c0")
        {
            $i = 0;
            @irccolors = split(/,/, $value);
            foreach $out (@irccolors)
            {
                $out =~ /([\da-fA-F]{2})([\da-fA-F]{2})([\da-fA-F]{2})$/;
                printf("IrcColorCode$i=%d,%d,%d\n",hex($1),hex($2),hex($3));
                $i++;
            }
        }
    }
    elsif ($_ =~ /NickColors/)
    {
        print("# DELETE $currentGroup$key\n");
        if ($value ne "#E90E7F,#8E55E9,#B30E0E,#18B33C,#58ADB3,#9E54B3,#0FB39A,#3176B3,#000000")
        {
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
}
