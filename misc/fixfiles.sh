#!/bin/bash
#
# Bash script to convert the cr/lf into lf for text files
# and reset teo.cfg.


# always set TMPDIR to something writable into!
if [ -z $TMPDIR ]; then
   TMPDIR=/tmp
fi

# adjust texts
echo "Adjust texts ..."
echo "s/\r//" > $TMPDIR/fixver.sed

cp change-fr.log $TMPDIR/fixver.tmp
sed -f $TMPDIR/fixver.sed $TMPDIR/fixver.tmp > change-fr.log
cp change-en.log $TMPDIR/fixver.tmp
sed -f $TMPDIR/fixver.sed $TMPDIR/fixver.tmp > change-en.log

cp licence-fr.txt $TMPDIR/fixver.tmp
sed -f $TMPDIR/fixver.sed $TMPDIR/fixver.tmp > licence-fr.txt
cp licence-en.txt $TMPDIR/fixver.tmp
sed -f $TMPDIR/fixver.sed $TMPDIR/fixver.tmp > licence-en.txt

cp readme-fr.txt $TMPDIR/fixver.tmp
sed -f $TMPDIR/fixver.sed $TMPDIR/fixver.tmp > readme-fr.txt
cp readme-en.txt $TMPDIR/fixver.tmp
sed -f $TMPDIR/fixver.sed $TMPDIR/fixver.tmp > readme-en.txt

cp src/change.log $TMPDIR/fixver.tmp
sed -f $TMPDIR/fixver.sed $TMPDIR/fixver.tmp > src/change.log

cp src/readme.txt $TMPDIR/fixver.tmp
sed -f $TMPDIR/fixver.sed $TMPDIR/fixver.tmp > src/readme.txt

cp src/license.txt $TMPDIR/fixver.tmp
sed -f $TMPDIR/fixver.sed $TMPDIR/fixver.tmp > src/license.txt

# reset teo.cfg
echo "Reset teo.cfg ..."
cp misc/teo_init.cfg teo.cfg

# clean up after ourselves
rm $TMPDIR/fixver.sed $TMPDIR/fixver.tmp

echo "Done!"

