#!/bin/sh
#
# - Compiles the executable Linux for DEBIAN
# - Creates the distribuable DEBIAN for the executable
# - Compiles the executable Linux for TAR.GZ
# - Creates the TAR.GZ distribuable Linux
# - Creates the Window distribuable ZIP in French
# - Creates the Window distribuable ZIP in English
# - Creates the MsDos distribuable ZIP in French
# - Creates the MsDos distribuable ZIP in English
# - Creates the ZIP for the sources
# - Creates the TAR.GZ for the sources

if [ `expr match $PWD '.*/teo$'` = 0 ]
   then
      echo "Usage: ./misc/pack/pack.sh"
      exit
fi

version='1.8.1'
packDir="teo/misc/pack"
zipOptions="-q -9"
gzipOptions="-9"

# ----------------- Convert files from DOS to UNIX

echo "Convert files from DOS to UNIX..."
./fixunix.sh

cd ..

# ----------------- Clean all

rm -f $packDir/teo-*.tar* $packDir/teo-*.zip $packDir/teo-*.deb
rm ./teo/teo.cfg
cp ./teo/misc/teo_init.cfg teo/teo.cfg

# ----------------- Linux DEBIAN package for executable

echo "Creating DEBIAN package for Linux executable..."

# Compile Linux DEBIAN
cd teo
make veryclean
make DEBIAN=1
make depend
cd ..
cd teo/sap
make clean
make
cd ../..
cp teo/sap/sap2  teo/
cp teo/sap/sapfs teo/
cd teo/k7tools
make clean
make
cd ../..
cp teo/k7tools/getrom   teo/
cp teo/k7tools/getmemo7 teo/
cp teo/k7tools/wav2k7   teo/
# Transfert DEBIAN file structure
sudo rm -r -f ~/teo-$version-i586
cp -r $packDir/debian/teo ~
mv -f ~/teo ~/teo-$version-i586
# Create missing folders
mkdir ~/teo-$version-i586/usr/games/
mkdir ~/teo-$version-i586/usr/share/teo
mkdir ~/teo-$version-i586/usr/share/doc
mkdir ~/teo-$version-i586/usr/share/doc/teo
mkdir ~/teo-$version-i586/usr/share/doc/teo/html
mkdir ~/teo-$version-i586/usr/share/doc/teo/html/images
# Copy files into file structure
cp teo/teo      ~/teo-$version-i586/usr/games/
cp teo/sap2     ~/teo-$version-i586/usr/games/
cp teo/sapfs    ~/teo-$version-i586/usr/games/
cp teo/getrom   ~/teo-$version-i586/usr/games/
cp teo/getmemo7 ~/teo-$version-i586/usr/games/
cp teo/wav2k7   ~/teo-$version-i586/usr/games/
cp teo/teo.cfg  ~/teo-$version-i586/usr/share/teo
cp teo/*-fr.txt ~/teo-$version-i586/usr/share/teo
cp teo/*-en.txt ~/teo-$version-i586/usr/share/teo
cp teo/*.rom    ~/teo-$version-i586/usr/share/teo
cp teo/doc/doc.css        ~/teo-$version-i586/usr/share/doc/teo/html
cp teo/doc/teo_linux*.htm ~/teo-$version-i586/usr/share/doc/teo/html
cp teo/doc/images/home.gif     ~/teo-$version-i586/usr/share/doc/teo/html/images
cp teo/doc/images/logo*.jpg    ~/teo-$version-i586/usr/share/doc/teo/html/images
cp teo/doc/images/teo_x*fr.png ~/teo-$version-i586/usr/share/doc/teo/html/images
# Update permissions
sudo chmod -R 755  ~/teo-$version-i586
sudo chmod -R 644  ~/teo-$version-i586/usr/share/applications/teo.desktop
sudo chmod -R 644  ~/teo-$version-i586/usr/share/teo/*.*
sudo chmod -R 644  ~/teo-$version-i586/usr/share/doc/teo/html/*.*
sudo chmod -R 644  ~/teo-$version-i586/usr/share/doc/teo/html/images/*.*
sudo chown -R root ~/teo-$version-i586
sudo chgrp -R root ~/teo-$version-i586
sudo chown $USER   ~/teo-$version-i586
sudo chgrp $USER   ~/teo-$version-i586
# Create DEBIAN package
sudo dpkg-deb --build ~/teo-$version-i586
sudo cp ~/teo-$version-i586.deb $packDir
# Clean DEBIAN file and DEBIAN folder
sudo rm -r -f ~/teo-$version-i586
sudo rm -r -f ~/teo-$version-i586.deb

# ----------------- Linux executable package

echo "Creating TAR.GZ package for Linux executable..."

# Compile Linux executable
cd teo
make veryclean
make
make depend
cd ..
# Create executable package
packFile="$packDir/teo-$version-i586.tar"
files="
teo/doc/images/teo_x*.png
teo/doc/teo_linux*.htm
teo/doc/images/home.gif
teo/doc/images/logo*.jpg
teo/doc/*.css
teo/*-??.*
teo/teo
teo/sap2
teo/sapfs
teo/getrom
teo/getmemo7
teo/wav2k7
teo/*.rom"

tar -cf $packFile $files
gzip $gzipOptions $packFile

# ----------------- Create TAR.GZ package for sources

srcfiles="
teo/src
teo/include
teo/doc/*.htm
teo/doc/images
teo/tests
teo/obj/linux/*.dep
teo/obj/djgpp/*.dep
teo/obj/mingw32/*.dep
teo/misc/*.*
$packDir/*.txt
$packDir/*.bat
$packDir/*.sh
$packDir/inno/*.iss
$packDir/inno/*.bmp
$packDir/debian
teo/*-??.*
teo/*.bat
teo/*.sh
teo/makefile*
teo/*.diff
teo/*.cfg
teo/*.rom"

echo "Creating TAR.GZ package for sources..."

packFile="$packDir/teo-$version-src.tar"
tar -cf $packFile $srcfiles
gzip $gzipOptions $packFile

# ----------------- Convert files from UNIX to DOS

cd teo
echo "Convert files from UNIX to DOS..."
./fixdoscr.sh

cd ..

# ----------------- MSDOS executable packages

commonFiles="
teo/teo.exe
teo/disks
teo/memo7
teo/k7
teo/language.dat
teo/keyboard.dat
teo/cwsdpmi.exe
teo/teo.cfg
teo/doc/images/home.gif
teo/doc/images/logo*.jpg
teo/doc/*.css
teo/*.rom"

echo "Creating ZIP packages for MSDOS executables in French..."

packFile="$packDir/teo-$version-dosexe-fr.zip"
rm teo/teo.exe
cp $packDir/msdos/teo-fr.exe teo/teo.exe
zip -r $zipOptions $packFile $commonFiles teo/*-fr.* teo/doc/images/teo_d?fr.gif teo/doc/teo_dos.htm

echo "Creating ZIP packages for MSDOS executables in English..."

packFile="$packDir/teo-$version-dosexe-en.zip"
rm teo/teo.exe
cp $packDir/msdos/teo-en.exe teo/teo.exe
zip -r $zipOptions $packFile $commonFiles teo/*-en.* teo/doc/images/teo_d?en.gif teo/doc/teo_dos_en.htm

# ----------------- Windows executable packages

commonFiles="
teo/teow.exe
teo/disks
teo/memo7
teo/k7
teo/language.dat
teo/keyboard.dat
teo/*.dll
teo/teo.cfg
teo/doc/images/home.gif
teo/doc/images/logo*.jpg
teo/doc/*.css
teo/*.rom"

echo "Creating ZIP packages for Windows executables in French..."

packFile="$packDir/teo-$version-winexe-fr.zip"
rm teo/teow.exe
cp $packDir/inno/teow-fr.exe teo/teow.exe
zip -r $zipOptions $packFile $commonFiles teo/*-fr.* teo/doc/images/teo_v?fr.gif teo/doc/images/teo_w?fr.gif teo/doc/teo_win.htm

echo "Creating ZIP packages for Windows executables in English..."

packFile="$packDir/teo-$version-winexe-en.zip"
rm teo/teow.exe
cp $packDir/inno/teow-en.exe teo/teow.exe
zip -r $zipOptions $packFile $commonFiles teo/*-en.* teo/doc/images/teo_v?en.gif teo/doc/images/teo_w?en.gif teo/doc/teo_win_en.htm

# ----------------- Create ZIP package for sources

echo "Creating ZIP package for sources..."

packFile="$packDir/teo-$version-src.zip"
zip -r $zipOptions $packFile $srcfiles

cd teo

# ----------------- Convert files from DOS to UNIX

echo "Convert files from DOS to UNIX..."
./fixunix.sh

echo "Packages created in ./misc/pack/!"

