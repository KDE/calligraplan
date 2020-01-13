#! /bin/sh
# Messages.sh files must have one instance of the line with:
# 'potfilename=<potfile>.pot'
# potfilename= must be at the start of the line and without spaces.
# It must refer to one pot file only.
# Release scripts rely on this.
potfilename=calligraplan.pot

source ../kundo2_aware_xgettext.sh

# Note: Don't extract sub-directories: specifically not libs, workpackage and plugins.
# NB! This means subdirs must be explicitly extracted!
$EXTRACTRC *.ui *.kcfg *.rc welcome/*.ui >> rc.cpp
kundo2_aware_xgettext $potfilename *.cpp about/*.cpp kptaboutdata.h welcome/*.cpp
rm -f rc.cpp
