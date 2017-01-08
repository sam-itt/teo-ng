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
         if [ "`basename $ent`" != "mc68xx" ]; then
            echo " opening" $ent
            recurse_bump $ent
         fi
      fi
   done

   for ent in $1/*.*[ch] ; do
      sed_file $ent
   done
}

recurse_bump_doc()
{
   local ent

   for ent in $1/*.*[h] ; do
      sed_file $ent
   done
}


# a basic sanity check
if [ $# -lt 2 -o $# -gt 3 ]; then
   echo "Usage: misc/fixver major_num minor_num [sub_num]" 1>&2
   echo "Example: misc/fixver 1 6 3" 1>&2
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

# patch teo.h
echo "Patching include/teo.h ..."
echo "s/\#define TEO_VERSION_STR .*/\#define TEO_VERSION_STR \"$verstr\"/" > $TMPDIR/fixver.sed
echo "s/\#define TEO_YEAR_STRING .*/\#define TEO_YEAR_STRING \"$todayYear\"/" >> $TMPDIR/fixver.sed
sed_file include/teo.h

# patch wdialog.rc
echo "Patching src/win/wdialog.rc ..."
echo "s/FILEVERSION .*/FILEVERSION $win_verstr/" > $TMPDIR/fixver.sed
echo "s/PRODUCTVERSION .*/PRODUCTVERSION $win_verstr/" >> $TMPDIR/fixver.sed
echo "s/Copyright © \([0-9]*\)\-[0-9]*/Copyright © \1\-$todayYear/g" >> $TMPDIR/fixver.sed
sed_file src/win/wdialog.rc

# patch readme texts
echo "Patching readme texts ..."
echo "s/^\([ ]\{16,\}\)version [0-9.]*/\1version $verstr/" > $TMPDIR/fixver.sed
echo "s/Copyright (C) \([0-9]*\)\-[0-9]*/Copyright (C) \1\-$todayYear/g" >> $TMPDIR/fixver.sed
sed_file LISEZMOI.txt
sed_file README.txt
sed_file src/readme.txt

# patch docs
echo "Patching docs..."
echo "s/\([teow?]\)\-[0-9\.]*\-\([a-z0-9\.]*\)/\1\-$verstr\-\2/g" > $TMPDIR/fixver.sed
sed_file doc/wiki/teo_dos_fr
sed_file doc/wiki/teo_dos_en
sed_file doc/wiki/teo_windows_fr
sed_file doc/wiki/teo_windows_en
sed_file doc/wiki/teo_linux_fr
sed_file doc/wiki/teo_linux_en

# patch pack scripts
echo "Patching pack scripts..."
echo "s/^\#define TEOVERSION\([ \t]*\)\(['\"]\)[0-9\.]*['\"]\(.*\)/\#define TEOVERSION\1\2$verstr\2\3/" > $TMPDIR/fixver.sed
sed_file misc/pack/repo/windows/inno/teo-setup.iss
echo "s/^teo_version=[ \t]*\(['\"]\)[0-9\.]*['\"]\(.*\)/teo_version=\1$verstr\1\2/" > $TMPDIR/fixver.sed
sed_file misc/pack/pack.sh
echo "s/^Version[ \t]*=[ \t]*[0-9\.]*/Version=$verstr/" > $TMPDIR/fixver.sed
sed_file misc/pack/repo/linux/debian/teo/usr/share/applications/teo.desktop
echo "s/^Version[ \t]*:[ \t]*[0-9\.]*/Version: $verstr/" > $TMPDIR/fixver.sed
sed_file misc/pack/repo/linux/debian/teo/DEBIAN/control

# bump file version number
echo "Bumping file version number..."
echo "s/Version    : [0-9.]*/Version    : $verstr/" > $TMPDIR/fixver.sed
echo "s/Copyright \(.* [0-9]*\)\-[0-9]* /Copyright \1-$todayYear /" >> $TMPDIR/fixver.sed
# shopt -s nullglob
recurse_bump include
recurse_bump src
unset recurse_bump

# clean up after ourselves
rm $TMPDIR/fixver.sed $TMPDIR/fixver.tmp

