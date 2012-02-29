#!/bin/sh
#
# - Compiles the executable Linux for DEBIAN
# - Creates the distribuable DEBIAN for the executable
# - Creates le distribuable DEBIAN for the ROMs
# - Compiles the executable Linux for TAR.GZ
# - Creates the TAR.GZ distribuable Linux
# - Creates the Window distribuable ZIP in French
# - Creates the Window distribuable ZIP in English
# - Creates the MsDos distribuable ZIP in French
# - Creates the MsDos distribuable ZIP in English
# - Creates the ZIP for the ROMs
# - Creates the TAR.GZ for the ROMs
# - Creates the ZIP for the sources
# - Creates the TAR.GZ for the sources

if [ `expr match $PWD '.*/teo$'` = 0 ]
   then
      echo "Usage: ./misc/pack/pack.sh"
      exit
fi

version='1.8.0'
packDir="teo/misc/pack"
zipOptions="-q -9"
gzipOptions="-9"
cd ..

# ----------------- Clean all

rm -f $packDir/teo-*.tar* $packDir/teo-*.zip $packDir/teo-*.deb

# ----------------- Linux DEBIAN package for executable

echo "Creating DEBIAN package for Linux executable..."

# Compile Linux DEBIAN
cd teo
make clean
make DEBIAN=1
make depend
cd ..
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
cp teo/teo ~/teo-$version-i586/usr/games/
cp teo/teo.cfg ~/teo-$version-i586/usr/share/teo
cp teo/*-fr.txt ~/teo-$version-i586/usr/share/teo
cp teo/*-en.txt ~/teo-$version-i586/usr/share/teo
cp teo/doc/doc.css ~/teo-$version-i586/usr/share/doc/teo/html
cp teo/doc/images/home.gif ~/teo-$version-i586/usr/share/doc/teo/html/images
cp teo/doc/images/logo*.jpg ~/teo-$version-i586/usr/share/doc/teo/html/images
cp teo/doc/teo_linux*.htm ~/teo-$version-i586/usr/share/doc/teo/html
cp teo/doc/images/teo_x*fr.png ~/teo-$version-i586/usr/share/doc/teo/html/images
# Update permissions
sudo chmod -R 755 ~/teo-$version-i586
sudo chmod -R 644 ~/teo-$version-i586/usr/share/applications/teo.desktop
sudo chmod -R 644 ~/teo-$version-i586/usr/share/teo/*.*
sudo chmod -R 644 ~/teo-$version-i586/usr/share/doc/teo/html/*.*
sudo chmod -R 644 ~/teo-$version-i586/usr/share/doc/teo/html/images/*.*
sudo chown -R root ~/teo-$version-i586
sudo chgrp -R root ~/teo-$version-i586
sudo chown $USER ~/teo-$version-i586
sudo chgrp $USER ~/teo-$version-i586
# Create DEBIAN package
sudo dpkg-deb --build ~/teo-$version-i586
sudo cp ~/teo-$version-i586.deb $packDir
# Clean DEBIAN file and DEBIAN folder
sudo rm -r -f ~/teo-$version-i586
sudo rm -r -f ~/teo-$version-i586.deb


# ----------------- Linux DEBIAN package for ROMs

echo "Creating DEBIAN package for ROMs..."

# Transfert DEBIAN file structure
sudo rm -r -f ~/teo-rom
cp -r $packDir/debian/teo-rom ~
# Create missing folders
mkdir ~/teo-rom/usr
mkdir ~/teo-rom/usr/share
mkdir ~/teo-rom/usr/share/teo
# Copy files into file structure
cp teo/b512_b0.rom ~/teo-rom/usr/share/teo
cp teo/b512_b1.rom ~/teo-rom/usr/share/teo
cp teo/basic1.rom ~/teo-rom/usr/share/teo
cp teo/fichier.rom ~/teo-rom/usr/share/teo
cp teo/to8mon1.rom ~/teo-rom/usr/share/teo
cp teo/to8mon2.rom ~/teo-rom/usr/share/teo
# Update permissions
sudo chmod -R 755 ~/teo-rom
sudo chmod -R 644 ~/teo-rom/usr/share/teo/*.*
sudo chown -R root ~/teo-rom
sudo chgrp -R root ~/teo-rom
sudo chown $USER ~/teo-rom
sudo chgrp $USER ~/teo-rom
# Create DEBIAN package
sudo dpkg-deb --build ~/teo-rom
cp ~/teo-rom.deb $packDir
# Clean DEBIAN file and DEBIAN folder
sudo rm -r -f ~/teo-rom
sudo rm -r ~/teo-rom.deb

# ----------------- Linux executable package

echo "Creating TAR.GZ package for Linux executable..."

# Compile Linux executable
cd teo
make clean
make
make depend
cd ..
# Create executable package
packFile="$packDir/teo-$version-i586.tar"
files="teo/doc/images/teo_x*.png teo/doc/teo_linux*.htm teo/doc/images/home.gif teo/doc/images/logo*.jpg teo/doc/*.css"
files=$files" teo/*-??.* teo/teo"
tar -cf $packFile $files
gzip $gzipOptions $packFile

# ----------------- MSDOS executable packages

commonFiles="teo/teo.exe teo/disks teo/memo7 teo/k7 teo/language.dat teo/keyboard.dat teo/cwsdpmi.exe teo/teo.cfg"
commonFiles="$commonFiles teo/doc/images/home.gif teo/doc/images/logo*.jpg teo/doc/*.css"

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

commonFiles="teo/teow.exe teo/disks teo/memo7 teo/k7 teo/language.dat teo/keyboard.dat teo/alleg40.dll teo/teo.cfg"
commonFiles="$commonFiles teo/doc/images/home.gif teo/doc/images/logo*.jpg teo/doc/*.css"

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

# ----------------- ROMs packages

echo "Creating ZIP package for ROMs..."

files="teo/b512_b0.rom teo/b512_b1.rom teo/basic1.rom teo/fichier.rom teo/to8mon1.rom teo/to8mon2.rom"

packFile="$packDir/teo-rom.zip"
zip -r $zipOptions $packFile $files

echo "Creating TAR.GZ package for ROMs..."

packFile="$packDir/teo-rom.tar"
tar -cf $packFile $files
gzip $gzipOptions $packFile

# ----------------- Sources packages

echo "Creating ZIP package for sources..."

files="teo/src teo/include teo/doc/*.htm teo/doc/images teo/tests"
files=$files" teo/obj/linux/*.dep teo/obj/djgpp/*.dep teo/obj/mingw32/*.dep"
files=$files" teo/misc/*.* $packDir/*.txt $packDir/*.bat $packDir/*.sh"
files=$files" $packDir/inno/*.iss $packDir/inno/*.bmp" 
files=$files" $packDir/debian" 
files=$files" teo/*-??.* teo/fix* teo/makefile* teo/*.diff teo/*.cfg"

packFile="$packDir/teo-$version-src.zip"
zip -r $zipOptions $packFile $files

echo "Creating TAR.GZ package for sources..."

packFile="$packDir/teo-$version-src.tar"
tar -cf $packFile $files
gzip $gzipOptions $packFile

cd teo

echo "Done!"

