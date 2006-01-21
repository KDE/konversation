#! /usr/bin/perl

use strict;

my($out);
my($out2);
my($group);
my($gotgroup);
my(%group2groupno);
my(@split);
my(%saw);
my (@lines) = (<>);

foreach $out (@lines)
{
    if ($out =~ /^\[ServerGroup ([0-9]+)\]/)
    { 
        $group = $1;
        $gotgroup = 1;
    }
    if ($out =~ /^Name=(.+)/ && $gotgroup)
    {  
        $group2groupno{$group} = $1;
        $gotgroup = 0;
    }
}

foreach $out (keys %group2groupno)
{
    foreach $out2 (@lines)
    {
        if ($out2 =~ /^$group2groupno{$out}=(.+)/)
        {
            @split = split(" ",$1);
            undef %saw;
            @saw{@split} = ();
            @split = keys %saw;
            print "[ServerGroup $out]\nNotifyList=@split\n"; 
        }
    }
}
