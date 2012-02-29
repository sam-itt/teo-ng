#!/bin/bash
#
# Bash script to adjust the timestamp of files.


recurse_main() {
   local ent

   for ent in $1/* ; do
      if [ -d $ent ]; then
         echo "Opening" $ent
         touch --date=$2 $ent
         recurse_main $ent $2
      else
         touch --date=$2 $ent
      fi
   done
}


if [ -z "$1" ]; then
   echo "Usage: misc/fixtime time" 1>&2
   exit 1
fi

shopt -s nullglob
recurse_main . $1
unset recurse_main

echo "Done!"
