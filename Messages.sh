#! /usr/bin/env bash
$EXTRACTRC `find . -name \*.ui -o -name \*.rc -o -name \*.kcfg` >> rc.cpp
$XGETTEXT `find . -name \*.cpp -o -name \*.h` -o $podir/konversation.pot
$XGETTEXT --language=Python --join data/scripts/cmd -o $podir/konversation.pot
$XGETTEXT --language=Python --join data/scripts/media -o $podir/konversation.pot
rm -f rc.cpp
