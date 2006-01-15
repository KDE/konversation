#!/usr/bin/perl -w

# This script is a conversion script.  It converts some of the settings in the old konversation 0.14 settings format to the current settings form.
# It is automatically called from the konversation.upd conversion file.

use strict;

my $currentGroup = "";

while ( <> ) {
  chomp;
  if ( /^\[/ ) { # group begin
    $currentGroup = $_;
    next;
  }
  if ( /^Geometry/ ) {
    my ($key,$value) = split /=/;
    my ($width,$height) = split(/,/, $value);
    print("# DELETE $currentGroup$key\n");
    print("[MainWindow]\n");
    print("Height=$height\n");
    print("Width=$width\n");
  }
  if ( /^ServerWindowToolBarIconSize/ ) {
    my ($key,$value) = split /=/;
    print("# DELETE $currentGroup$key\n");
    print("[MainWindow Toolbar mainToolBar]\n");
    print("IconSize=$value\n");
  }
  if ( /^ServerWindowToolBarIconText/ ) {
    my ($key,$value) = split /=/;
    my @position = ("IconOnly", "IconTextRight", "TextOnly", "IconTextBottom");
    print("# DELETE $currentGroup$key\n");
    print("[MainWindow Toolbar mainToolBar]\n");
    print("IconText=".$position[$value]."\n");
  }
  if ( /^ServerWindowToolBarPos/ ) {
    my ($key,$value) = split /=/;
    my @position = ("Unmanaged", "Floating", "Top", "Bottom", "Right", "Left", "Flat");
    print("# DELETE $currentGroup$key\n");
    print("[MainWindow Toolbar mainToolBar]\n");
    print("Position=".$position[$value]."\n");
  }
}
