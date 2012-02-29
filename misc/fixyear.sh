#!/bin/bash
#
# Bash script to adjust the year date in files.


recurse_year()
{
   local ent

   for ent in $1/* ; do
      if [ -d $ent ]; then
         if [ "`basename $ent`" != "mc68xx" ]; then
            echo " opening" $ent
            recurse_year $ent $2
         fi
      fi
   done

   for ent in $1/*.*[ch] ; do
      cp $ent $TMPDIR/fixver.tmp
      sed -e "s/1997-[0-9.]*/1997-$2/" $TMPDIR/fixver.tmp > $ent
   done
}


# always set TMPDIR to something writable into!
if [ -z $TMPDIR ]; then
   TMPDIR=/tmp
fi

# get the year
year=$(date +%Y)

# bump file version number
echo "Updating files year..."
# shopt -s nullglob
recurse_year include $year
recurse_year src $year
unset recurse_year

# clean up after ourselves
rm $TMPDIR/fixver.tmp

echo "Done!"

