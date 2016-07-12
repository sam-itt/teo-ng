#!/bin/sh
#
# Convert files from UNIX to DOS

echo "Convert files from UNIX to DOS..."
find . -type d "(" \
      -name ".hg" -prune \
      -name "autogen.sh" -prune \
      ")" -o \
      -type f "(" \
      -name "*.c" -o -name "*.h" -o -name "*.rc" -o -name "*.rh" -o \
      -name "*.xpm" -o -name "makefile.*" -o \
      -name "*.txt" -o -name "*.log" -o -name "*.bat" -o \
      -name "*.BAT"  -o -name "*.htm*"\
      ")" \
      -exec sh -c "echo -n '\r                                                     \r{}';
                   mv {} _tmpfile;
                   sed 's/\x0d$//' _tmpfile > _tmpfile2;
                   sed 's/$/\r/' _tmpfile2 > {};
                   touch -r _tmpfile {};
                   rm _tmpfile _tmpfile2" \;
echo

