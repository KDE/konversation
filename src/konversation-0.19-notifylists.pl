#!/usr/bin/perl

use strict;

my($out);
my($out2);
my($group);
my($server);
my($list);
my($si);
my(@split);
my(@servers);
my(%si2group);
my(%lists);
my(%saw);
my (@lines) = (<>);

foreach $out (@lines)
{
    if ($out =~ /^\[ServerGroup ([0-9]+)\]/)
    { 
        $group = $1;
    }
    if ($out =~ /^ServerList=(.+)/)
    {  
        @split = split(",",$1);
        foreach $out2 (@split)
        {
            $out2 =~ /^Server ([0-9]+)/;
            $si2group{$1} = $group;
        }
    }
}

foreach $out (@lines)
{
    if ($out =~ /^\[Server ([0-9]+)\]/)
    { 
        $si = $1;
    }
    if ($out =~ /^Server=(.+)/)
    {  
        $group = $si2group{$si};
        push @servers,"$group=$1";
    }
}

foreach $out (@servers)
{
    ($group,$server) = split("=",$out);

    foreach $out2 (@lines)
    {
        if ($out2 =~ /^$server=(.+)/)
        {
            $lists{$group} .= "$1 ";
        }
    }

}

while (my($key,$val) = each(%lists)) 
{
    $val =~ s/\s+$//;
    @split = split(" ",$val);
    undef %saw;
    @saw{@split} = ();
    @split = keys %saw;
    print "[ServerGroup $key]\nNotifyList=@split\n"; 
}
