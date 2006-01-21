#! /usr/bin/perl

use strict;

my($out);
my($group);
my($gotgroup);
my(%group2groupno);
my(%key2value);
my(@split);
my(%saw);

while (<>)
{
    if ($_ =~ /^\[ServerGroup ([0-9]+)\]/)
    { 
        $group = $1;
        $gotgroup = 1;
    }
    elsif ($_ =~ /^Name=(.+)/ && $gotgroup)
    {  
        $group2groupno{$group} = $1;
        $gotgroup = 0;
    }
    elsif ($_ =~ /^(.+)=(.+)/)
    {  
        $key2value{$1} = $2;
    }
}

foreach $out (keys %group2groupno)
{
    @split = split(" ",$key2value{$group2groupno{$out}});

    if (@split)
    {
        undef %saw;
        @saw{@split} = ();
        @split = keys %saw;

        print "[ServerGroup $out]\nNotifyList=@split\n"; 
    }
}
