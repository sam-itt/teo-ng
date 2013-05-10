#!/bin/bash
#
# Convert wiki files into html doc files
# 'markdown' should be installed : sudo apt-get install markdown

if [ `expr match $PWD '.*/doc$'` = 0 ]
   then
      echo "Usage: ./markhtml.sh from doc/ folder"
      exit
fi

todayYear=$(date +%Y)
echo -n "Convert Markdown to HTML"

rm *.htm
cd wiki

for i in *;
{
   echo -n ".";
   echo "<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">" > _tmpfile;
   echo "<html>" >> _tmpfile;
   echo "<head>" >> _tmpfile;
   echo "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\">" >> _tmpfile;
   echo "<title>Teo</title>" >> _tmpfile;
   echo "<link rel=\"stylesheet\" type=\"text/css\" href=\"doc.css\">" >> _tmpfile;
   echo "</head>" >> _tmpfile;
   echo "<body>" >> _tmpfile;
   echo "<table width=\"100%\"><tr><td>" >> _tmpfile;
   echo "" >> _tmpfile;
   markdown $i >> _tmpfile
   echo "</td></tr></table>" >> _tmpfile;
   echo "</body>" >> _tmpfile;
   echo "</html>" >> _tmpfile;
   sed 's/\\//g' _tmpfile > _tmpfile2;
   sed 's/\&amp\;/\&/g' _tmpfile2 > _tmpfile;
   sed 's/<img src=\"\([a-zA-Z0-9_-\.]*\)\"/<img src=\"images\/\1\"/g' _tmpfile > _tmpfile2;
   sed 's/href=\"\.\.\/\([a-zA-Z0-9_-]*\)\">/href=\"\1.htm\">/g' _tmpfile2 > _tmpfile;
   sed 's/href=\"\.\/#/href=\"\#/g' _tmpfile > ../$i.htm;
   rm _tmpfile _tmpfile2;
}
cd ..

echo "Done!"

