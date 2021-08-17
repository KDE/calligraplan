#! /usr/bin/env bash
# Messages.sh files must have one instance of the line with:
# 'potfilename=<potfile>.pot'
# potfilename= must be at the start of the line and without spaces.
# It must refer to one pot file only.
# Release scripts rely on this.
potfilename=calligraplanportfolio.pot

source ../../kundo2_aware_xgettext.sh

$EXTRACTRC `find . -name \*.ui -o -name \*.kcfg -o -name \*.rc | grep -v '/tests/'` >> rc.cpp
kundo2_aware_xgettext $potfilename `find . -name \*.cpp -o -name \*.h | grep -v '/tests/'`
rm -f rc.cpp
