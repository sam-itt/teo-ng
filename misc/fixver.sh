#!/bin/bash
#
# Bash script to adjust the version number of files.


recurse_bump()
{
   local ent

   for ent in $1/* ; do
      if [ -d $ent ]; then
         if [ "`basename $ent`" != "mc68xx" ]; then
            echo " opening" $ent
            recurse_bump $ent $2
         fi
      fi
   done

   for ent in $1/*.*[ch] ; do
      cp $ent $TMPDIR/fixver.tmp
      sed -e "s/Version    : [0-9.]*/Version    : $2/" $TMPDIR/fixver.tmp > $ent
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

# patch to8.h
echo "Patching include/to8.h ..."
cp include/to8.h $TMPDIR/fixver.tmp
sed -e "s/\#define TO8_VERSION_STR .*/\#define TO8_VERSION_STR \"$verstr\"/" $TMPDIR/fixver.tmp > include/to8.h

# patch wdialog.rc
echo "Patching src/win/wdialog.rc ..."
echo "s/FILEVERSION .*/FILEVERSION $win_verstr/" > $TMPDIR/fixver.sed
echo "s/PRODUCTVERSION .*/PRODUCTVERSION $win_verstr/" >> $TMPDIR/fixver.sed
cp src/win/wdialog.rc $TMPDIR/fixver.tmp
sed -f $TMPDIR/fixver.sed $TMPDIR/fixver.tmp > src/win/wdialog.rc

# patch readme texts
echo "Patching readme texts ..."
echo "s/^\([ ]\{16,\}\)version [0-9.]*/\1version $verstr/" > $TMPDIR/fixver.sed
cp readme-fr.txt $TMPDIR/fixver.tmp
sed -f $TMPDIR/fixver.sed $TMPDIR/fixver.tmp > readme-fr.txt
cp readme-en.txt $TMPDIR/fixver.tmp
sed -f $TMPDIR/fixver.sed $TMPDIR/fixver.tmp > readme-en.txt
cp src/readme.txt $TMPDIR/fixver.tmp
sed -f $TMPDIR/fixver.sed $TMPDIR/fixver.tmp > src/readme.txt

# patch docs
todayYear=$(date +%Y)
echo "Patching docs..."
echo "s/version [0-9.]*<BR>/version $verstr<BR>/" > $TMPDIR/fixver.sed
cp doc/teo.htm $TMPDIR/fixver.tmp
sed -f $TMPDIR/fixver.sed $TMPDIR/fixver.tmp > doc/teo.htm
cp doc/teo_en.htm $TMPDIR/fixver.tmp
sed -f $TMPDIR/fixver.sed $TMPDIR/fixver.tmp > doc/teo_en.htm
echo "s/version [0-9.]* \([(A-Za-z0-9)\/]*\)<BR>/version $verstr \1<BR>/" > $TMPDIR/fixver.sed
echo "s/\([teow?]\)\-[0-9\.]*\-\([a-z0-9\.]*\)/\1\-$verstr\-\2/g" >> $TMPDIR/fixver.sed
echo "s/Copyright \&copy\; 199\([79]\)\-[0-9]* /Copyright \&copy\; 199\1\-$todayYear /" >> $TMPDIR/fixver.sed
cp doc/teo_dos.htm $TMPDIR/fixver.tmp
sed -f $TMPDIR/fixver.sed $TMPDIR/fixver.tmp > doc/teo_dos.htm
cp doc/teo_dos_en.htm $TMPDIR/fixver.tmp
sed -f $TMPDIR/fixver.sed $TMPDIR/fixver.tmp > doc/teo_dos_en.htm
cp doc/teo_win.htm $TMPDIR/fixver.tmp
sed -f $TMPDIR/fixver.sed $TMPDIR/fixver.tmp > doc/teo_win.htm
cp doc/teo_win_en.htm $TMPDIR/fixver.tmp
sed -f $TMPDIR/fixver.sed $TMPDIR/fixver.tmp > doc/teo_win_en.htm
cp doc/teo_linux.htm $TMPDIR/fixver.tmp
sed -f $TMPDIR/fixver.sed $TMPDIR/fixver.tmp > doc/teo_linux.htm
cp doc/teo_linux_en.htm $TMPDIR/fixver.tmp
sed -f $TMPDIR/fixver.sed $TMPDIR/fixver.tmp > doc/teo_linux_en.htm

# patch pack scripts
echo "Patching pack scripts..."
echo "s/^\#define APPVERSION\([ \t]*\)\(['\"]\)[0-9\.]*['\"]\(.*\)/\#define APPVERSION\1\2$verstr\2\3/" > $TMPDIR/fixver.sed
cp misc/pack/teow-roms-setup.iss $TMPDIR/fixver.tmp
sed -f $TMPDIR/fixver.sed $TMPDIR/fixver.tmp > misc/pack/teow-roms-setup.iss
cp misc/pack/teow-setup.iss $TMPDIR/fixver.tmp
sed -f $TMPDIR/fixver.sed $TMPDIR/fixver.tmp > misc/pack/teow-setup.iss
echo "s/^version=[ \t]*\(['\"]\)[0-9\.]*['\"]\(.*\)/version=\1$verstr\1\2/" > $TMPDIR/fixver.sed
cp misc/pack/pack.sh $TMPDIR/fixver.tmp
sed -f $TMPDIR/fixver.sed $TMPDIR/fixver.tmp > misc/pack/pack.sh

# bump file version number
echo "Bumping file version number..."
# shopt -s nullglob
recurse_bump include $verstr
recurse_bump src $verstr
unset recurse_bump

# clean up after ourselves
rm $TMPDIR/fixver.sed $TMPDIR/fixver.tmp

echo "Done!"

