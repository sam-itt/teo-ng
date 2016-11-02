#!/bin/bash
#
# Bash script to adjust the version number and dates of files.


sed_file()
{
    cp $1 $TMPDIR/fixver.tmp
    sed -f $TMPDIR/fixver.sed $TMPDIR/fixver.tmp > $1
}


recurse_bump()
{
   local ent

   for ent in $1/* ; do
      if [ -d $ent ]; then
         echo " opening" $ent
         recurse_bump $ent
      fi
   done

   for ent in $1/*.*[ch] ; do
      sed_file $ent
   done
}


# a basic sanity check
if [ $# -lt 2 -o $# -gt 3 ]; then
   echo "Usage: misc/fixver/cc90hfe.sh major_num minor_num [sub_num]" 1>&2
   echo "Example: misc/fixver/cc90hfe.sh 1 6 3" 1>&2
   exit 1
fi

# always set TMPDIR to something writable into!
if [ -z $TMPDIR ]; then
   TMPDIR=/tmp
fi

# get the version string in a nice format
if [ $# -eq 3 ]; then
   verstr="$1.$2.$3"
   win_verstr="$1, $2, $3, 0"
else
   verstr="$1.$2"
   win_verstr="$1, $2, 0, 0"
fi

todayYear=$(date +%Y)
todayMonth=$(LC_ALL="en_EN.UTF-8" date +%B)

# patch defs.h
echo "Patching include/defs.h ..."
echo "s/\#define PROG_VERSION_MAJOR .*/\#define PROG_VERSION_MAJOR \"$1\"/" > $TMPDIR/fixver.sed
echo "s/\#define PROG_VERSION_MINOR .*/\#define PROG_VERSION_MINOR \"$2\"/" >> $TMPDIR/fixver.sed
echo "s/\#define PROG_VERSION_MICRO .*/\#define PROG_VERSION_MICRO \"$3\"/" >> $TMPDIR/fixver.sed
echo "s/\#define PROG_CREATION_YEAR .*/\#define PROG_CREATION_YEAR \"$todayYear\"/" >> $TMPDIR/fixver.sed
echo "s/\#define PROG_CREATION_MONTH .*/\#define PROG_CREATION_MONTH \"$todayMonth\"/" >> $TMPDIR/fixver.sed
sed_file tools/cc90hfe/include/defs.h

# patch wdialog.rc
echo "Patching src/win/resource.rc ..."
echo "s/FILEVERSION .*/FILEVERSION $win_verstr/" > $TMPDIR/fixver.sed
echo "s/PRODUCTVERSION .*/PRODUCTVERSION $win_verstr/" >> $TMPDIR/fixver.sed
sed_file tools/cc90hfe/src/win/resource.rc

# patch pack scripts
echo "Patching pack scripts..."
echo "s/^cc90hfe_version=[ \t]*\(['\"]\)[0-9\.]*['\"]\(.*\)/cc90hfe_version=\1$verstr\1\2/" > $TMPDIR/fixver.sed
sed_file misc/pack/pack.sh
echo "s/^Version[ \t]*=[ \t]*[0-9\.]*/Version=$verstr/" > $TMPDIR/fixver.sed
sed_file misc/pack/debian/teo/usr/share/applications/teo.desktop
echo "s/^Version[ \t]*:[ \t]*[0-9\.]*/Version: $verstr/" > $TMPDIR/fixver.sed
sed_file misc/pack/debian/teo/DEBIAN/control

# bump file version number
echo "Bumping file version number..."
echo "s/Version    : [0-9.]*/Version    : $verstr/" > $TMPDIR/fixver.sed
echo "s/Copyright \(.* [0-9]*\)\-[0-9]*/Copyright \1\-$todayYear/" >> $TMPDIR/fixver.sed

recurse_bump tools/cc90hfe/include
recurse_bump tools/cc90hfe/src
unset recurse_bump
unset sed_file

# clean up after ourselves
rm $TMPDIR/fixver.sed $TMPDIR/fixver.tmp

