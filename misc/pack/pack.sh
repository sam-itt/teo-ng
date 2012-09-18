#!/bin/bash
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
      echo "Usage: ./misc/pack/pack.sh from teo/ folder"
      exit
fi

# always set TMPDIR to something writable into!
if [ -z $TMPDIR ]; then
   TMPDIR=/tmp
fi

version='1.8.2'
packDir="teo/misc/pack"
zipOptions="-q -9"
gzipOptions="-9"

# list of source files
source_files="
teo/src
teo/sap
teo/k7tools/
teo/include
teo/system
teo/doc
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
teo/system/rom/*.rom
teo/system/printer/*.txt
teo/*.dll"

# list of common files for executable packages
common_exec="
teo/disks
teo/memo7
teo/k7
teo/system
teo/allegro.cfg
teo/doc/*.htm
teo/doc/*.css
teo/system/rom/*.rom
teo/system/printer/*.txt"

# copy to temporary folder
cd ..
current_folder=$PWD
cp -r ./teo $TMPDIR
cd $TMPDIR


# ----------------- convert files from DOS to UNIX

cd teo
./fixunix.sh
cd ..

# ----------------- clean all

rm -r -f ./disks
rm -r -f ./memo7
rm -r -f ./k7
rm -f $current_folder/$packDir/teo-*.tar* $current_folder/$packDir/teo-*.zip $current_folder/$packDir/teo-*.deb

# ----------------- Linux DEBIAN package for executable

echo "Creating DEBIAN package for Linux executable..."

# Compile Teo
cd teo
make veryclean
make DEBIAN=1
make depend
cd ..
# Compile saptools
cd teo/sap
make clean
make
cd ../..
cp teo/sap/sap2  teo/
cp teo/sap/sapfs teo/
# Compile k7tools
cd teo/k7tools
make clean
make
cd ../..
cp teo/k7tools/wav2k7 teo/
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
# Copy files into file structure
cp teo/teo      ~/teo-$version-i586/usr/games/
cp teo/sap2     ~/teo-$version-i586/usr/games/
cp teo/sapfs    ~/teo-$version-i586/usr/games/
cp teo/wav2k7   ~/teo-$version-i586/usr/games/
cp teo/allegro.cfg  ~/teo-$version-i586/usr/share/teo
cp -r teo/system   ~/teo-$version-i586/usr/share/teo/system
cp teo/*-fr.txt ~/teo-$version-i586/usr/share/teo
cp teo/*-en.txt ~/teo-$version-i586/usr/share/teo
cp teo/doc/doc.css ~/teo-$version-i586/usr/share/doc/teo/html
cp teo/doc/*.htm ~/teo-$version-i586/usr/share/doc/teo/html
# Update permissions
sudo chmod -R 755  ~/teo-$version-i586
sudo chmod -R 644  ~/teo-$version-i586/usr/share/applications/teo.desktop
sudo chmod -R 644  ~/teo-$version-i586/usr/share/teo/*.*
sudo chmod -R 644  ~/teo-$version-i586/usr/share/doc/teo/html/*.*
sudo chown -R root ~/teo-$version-i586
sudo chgrp -R root ~/teo-$version-i586
sudo chown $USER   ~/teo-$version-i586
sudo chgrp $USER   ~/teo-$version-i586
# Create DEBIAN package
sudo dpkg-deb --build ~/teo-$version-i586
sudo cp ~/teo-$version-i586.deb $current_folder/$packDir
# Clean DEBIAN file and DEBIAN folder
sudo rm -r -f ~/teo-$version-i586
sudo rm -r -f ~/teo-$version-i586.deb

# ----------------- Create media folders

mkdir teo/disks
mkdir teo/memo7
mkdir teo/k7

# ----------------- Linux executable package

echo "Creating TAR.GZ package for Linux executable..."

# Compile Teo
cd teo
make veryclean
make DEBIAN=1
make depend
cd ..
# Compile saptools
cd teo/sap
make clean
make
cd ../..
cp teo/sap/sap2  teo/
cp teo/sap/sapfs teo/
# Compile k7tools
cd teo/k7tools
make clean
make
cd ../..
cp teo/k7tools/wav2k7 teo/
# Create executable package
linux_exec="
teo/teo
teo/sap2
teo/sapfs
teo/wav2k7"
packFile="$current_folder/$packDir/teo-$version-i586.tar"
tar -cf $packFile $common_exec $linux_exec teo/*-??.*
gzip $gzipOptions $packFile

# ----------------- clean Linux executables

# Clean Teo
cd teo
make clean
cd ..
# Clean saptools
cd teo/sap
make clean
cd ../..
rm teo/sap2
rm teo/sapfs
# Clean k7tools
cd teo/k7tools
make clean
cd ../..
rm teo/wav2k7

# ----------------- create TAR.GZ package for sources

echo "Creating TAR.GZ package for sources..."

packFile="$current_folder/$packDir/teo-$version-src.tar"
tar -cf $packFile $source_files
gzip $gzipOptions $packFile

# ----------------- convert files from UNIX to DOS

cd teo
./fixdoscr.sh
cd ..

# ----------------- MS-DOS executable packages

msdos_exec="
teo/language.dat
teo/keyboard.dat
teo/teo.exe
teo/sap2.exe
teo/sapfs.exe
teo/wav2k7.exe
teo/cwsdpmi.exe"

echo "Creating ZIP packages for MSDOS executables in French..."

packFile="$current_folder/$packDir/teo-$version-dosexe-fr.zip"
cp $packDir/msdos/teo-fr.exe teo/teo.exe
cp $packDir/msdos/sap2-fr.exe teo/sap2.exe
cp $packDir/msdos/sapfs-fr.exe teo/sapfs.exe
cp $packDir/msdos/wav2k7-fr.exe teo/wav2k7.exe
zip -r $zipOptions $packFile $common_exec $msdos_exec teo/*-fr.*
rm teo/teo.exe teo/sap2.exe teo/sapfs.exe teo/wav2k7.exe

echo "Creating ZIP packages for MSDOS executables in English..."

packFile="$current_folder/$packDir/teo-$version-dosexe-en.zip"
cp $packDir/msdos/teo-en.exe teo/teo.exe
cp $packDir/msdos/sap2-en.exe teo/sap2.exe
cp $packDir/msdos/sapfs-en.exe teo/sapfs.exe
cp $packDir/msdos/wav2k7-en.exe teo/wav2k7.exe
zip -r $zipOptions $packFile $common_exec $msdos_exec teo/*-en.*
rm teo/teo.exe teo/sap2.exe teo/sapfs.exe teo/wav2k7.exe

# ----------------- Windows executable packages

windows_exec="
teo/language.dat
teo/keyboard.dat
teo/teow.exe
teo/sap2.exe
teo/sapfs.exe
teo/wav2k7.exe
teo/*.dll"

echo "Creating ZIP packages for Windows executables in French..."

packFile="$current_folder/$packDir/teo-$version-winexe-fr.zip"
cp $packDir/inno/teow-fr.exe teo/teow.exe
cp $packDir/msdos/sap2-fr.exe teo/sap2.exe
cp $packDir/msdos/sapfs-fr.exe teo/sapfs.exe
cp $packDir/msdos/wav2k7-fr.exe teo/wav2k7.exe
zip -r $zipOptions $packFile $common_exec $windows_exec teo/*-fr.*
rm teo/teow.exe teo/sap2.exe teo/sapfs.exe teo/wav2k7.exe

echo "Creating ZIP packages for Windows executables in English..."

packFile="$current_folder/$packDir/teo-$version-winexe-en.zip"
cp $packDir/inno/teow-en.exe teo/teow.exe
cp $packDir/msdos/sap2-en.exe teo/sap2.exe
cp $packDir/msdos/sapfs-en.exe teo/sapfs.exe
cp $packDir/msdos/wav2k7-en.exe teo/wav2k7.exe
zip -r $zipOptions $packFile $common_exec $windows_exec teo/*-en.*
rm teo/teow.exe teo/sap2.exe teo/sapfs.exe teo/wav2k7.exe

# ----------------- create ZIP package for sources

echo "Creating ZIP package for sources..."

packFile="$current_folder/$packDir/teo-$version-src.zip"
zip -r $zipOptions $packFile $source_files

# ----------------- delete temporary folders and files

rm -r ./teo

# ----------------- return to current folder

cd $current_folder/teo

echo "Packages created in ./misc/pack/!"

