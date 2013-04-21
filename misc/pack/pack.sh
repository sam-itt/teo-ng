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

build_debian_package()
{
    local unpacked_size

    cp ~/$1/DEBIAN/control control.tmp
    unpacked_size=$(echo $(du -ks ~/$1) | sed 's/\([0-9]*\).*/\1/');
    sed -e 's/Installed-Size:.*/Installed-Size: '$unpacked_size'/g' control.tmp > ~/$1/DEBIAN/control;
    rm control.tmp

    # Update permissions
    find ~/$1 -type d -exec chmod 755 {} \;
    find ~/$1 -type f -exec chmod 644 {} \;
    sudo chmod +x ~/$1/usr/games/*
    sudo chown -R root ~/$1
    sudo chgrp -R root ~/$1
    sudo chown $USER   ~/$1
    sudo chgrp $USER   ~/$1

    # Create DEBIAN package
    sudo dpkg-deb --build ~/$1
    sudo cp ~/$1.deb $2

    # Clean DEBIAN file and DEBIAN folder
    sudo rm -r -f ~/$1
    sudo rm -r -f ~/$1.deb
}


if [ `expr match $PWD '.*/teo$'` = 0 ]
   then
      echo "Usage: ./misc/pack/pack.sh from teo/ folder"
      exit
fi

# always set TMPDIR to something writable into!
if [ -z $TMPDIR ]; then
   TMPDIR=/tmp
fi

teoversion='1.8.2'
cc90hfeversion='0.5.0'
packDir="teo/misc/pack"
zipOptions="-q -9"
gzipOptions="-9"

# list of source files
source_files="
teo/src
teo/tools/sap
teo/tools/k7tools/
teo/tools/cc90hfe/include/
teo/tools/cc90hfe/obj/linux/blank.txt
teo/tools/cc90hfe/obj/win/blank.txt
teo/tools/cc90hfe/src/
teo/tools/cc90hfe/makefile.*
teo/tools/cc90hfe/fix*.*
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
teo/*.dll"

# list of common files for executable packages
common_exec="
teo/disks
teo/memo7
teo/k7
teo/system
teo/allegro.cfg
teo/doc/images/*.*
teo/doc/*.htm
teo/doc/*.css
teo/system/rom/*.rom
teo/system/printer/042/*.txt
teo/system/printer/055/*.txt
teo/system/printer/582/*.txt
teo/system/printer/600/*.txt
teo/system/printer/612/*.txt"


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
rm -f $current_folder/$packDir/cc90hfe-*.tar* $current_folder/$packDir/cc90hfe-*.zip $current_folder/$packDir/cc90hfe-*.deb

# ----------------- Linux DEBIAN package for executable

echo "Creating DEBIAN package for Linux executable..."

# Compile Teo
cd teo
make veryclean
make DEBIAN=1
make depend
cd ..

# Compile saptools
cd teo/tools/sap
make clean
make
cd ../../..
cp teo/tools/sap/sap2  teo/
cp teo/tools/sap/sapfs teo/

# Compile k7tools
cd teo/tools/k7tools
make clean
make
cd ../../..
cp teo/tools/k7tools/wav2k7 teo/

# ---------------- Teo Debian ----------------------

deb_name=teo-$teoversion-i586
deb_dir=$deb_name/usr/share/teo

# Transfert DEBIAN file structure
sudo rm -r -f ~/$deb_name
cp -r $packDir/debian/teo ~
mv -f ~/teo ~/$deb_name

# Create missing folders
mkdir ~/$deb_name/usr/games/
mkdir ~/$deb_dir
mkdir ~/$deb_dir/system
mkdir ~/$deb_dir/doc
mkdir ~/$deb_dir/doc/images

# Copy files into file structure
cp teo/teo            ~/$deb_name/usr/games/
cp teo/sap2           ~/$deb_name/usr/games/
cp teo/sapfs          ~/$deb_name/usr/games/
cp teo/wav2k7         ~/$deb_name/usr/games/
cp teo/allegro.cfg    ~/$deb_dir
cp -r teo/system      ~/$deb_dir
cp teo/*-fr.txt       ~/$deb_dir
cp teo/*-en.txt       ~/$deb_dir
cp teo/doc/images/*.* ~/$deb_dir/doc/images
cp teo/doc/doc.css    ~/$deb_dir/doc
cp teo/doc/*.htm      ~/$deb_dir/doc

# Build DEBIAN package
build_debian_package $deb_name $current_folder/$packDir

# ---------------- cc90hfe Debian ----------------------

deb_name=cc90hfe-$cc90hfeversion-i586
deb_dir=$deb_name/usr/share/cc90hfe

# Compile cc90hfe
cd teo/tools/cc90hfe
./fixunix.sh
make clean
make DEBIAN=1
cd ../../..

# Transfert DEBIAN file structure
sudo rm -r -f ~/$deb_name
cp -r $packDir/debian/cc90hfe ~
mv -f ~/cc90hfe ~/$deb_name

# Create missing folders
mkdir ~/$deb_name/usr/games/
mkdir ~/$deb_dir
mkdir ~/$deb_dir/doc
mkdir ~/$deb_dir/doc/images

# Copy files into file structure
cp teo/tools/cc90hfe/cc90hfe    ~/$deb_name/usr/games/
# cp teo/tools/cc90hfe/*-fr.txt ~/$deb_dir
# cp teo/tools/cc90hfe/*-en.txt ~/$deb_dir
cp teo/doc/doc.css              ~/$deb_dir/doc
cp teo/doc/images/*.*           ~/$deb_dir/doc/images
cp teo/doc/cc90*.htm            ~/$deb_dir/doc
mv ~/$deb_dir/doc/cc90hfe_en.htm ~/$deb_dir/doc/index.htm
mv ~/$deb_dir/doc/cc90hfe_fr.htm ~/$deb_dir/doc/index_fr.htm

# Build DEBIAN package
build_debian_package $deb_name $current_folder/$packDir

# ----------------- Create media folders

mkdir teo/disks
mkdir teo/memo7
mkdir teo/k7

# ----------------- Linux executable package

echo "Creating TAR.GZ package for Linux executable..."

# Compile Teo
cd teo
make veryclean
make
make depend
cd ..

# Compile saptools
cd teo/tools/sap
make clean
make
cd ../../..
cp teo/tools/sap/sap2  teo/
cp teo/tools/sap/sapfs teo/

# Compile k7tools
cd teo/tools/k7tools
make clean
make
cd ../../..
cp teo/tools/k7tools/wav2k7 teo/

# Compile cc90hfe
cd teo/tools/cc90hfe
./fixunix.sh
make clean
make
cd ../../..
cp teo/tools/cc90hfe/cc90hfe teo/

# Create executable package
linux_exec="
teo/teo
teo/sap2
teo/sapfs
teo/wav2k7
teo/cc90hfe
teo/cc90.sap"
packFile="$current_folder/$packDir/teo-$teoversion-i586.tar"
tar -cf $packFile $common_exec $linux_exec teo/*-??.*
gzip $gzipOptions $packFile

# ----------------- clean Linux executables

# Clean Teo
cd teo
make clean
cd ..

# Clean saptools
cd teo/tools/sap
make clean
cd ../../..
rm teo/sap2
rm teo/sapfs

# Clean k7tools
cd teo/tools/k7tools
make clean
cd ../../..
rm teo/wav2k7

# Clean cc90hfe
cd teo/tools/cc90hfe
make clean
cd ../../..
rm teo/cc90hfe

# ----------------- create TAR.GZ package for sources

echo "Creating TAR.GZ package for sources..."

packFile="$current_folder/$packDir/teo-$teoversion-src.tar"
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

packFile="$current_folder/$packDir/teo-$teoversion-dosexe-fr.zip"
cp $packDir/msdos/teo-fr.exe    teo/teo.exe
cp $packDir/msdos/sap2-fr.exe   teo/sap2.exe
cp $packDir/msdos/sapfs-fr.exe  teo/sapfs.exe
cp $packDir/msdos/wav2k7-fr.exe teo/wav2k7.exe
zip -r $zipOptions $packFile $common_exec $msdos_exec teo/*-fr.*
rm teo/teo.exe teo/sap2.exe teo/sapfs.exe teo/wav2k7.exe

echo "Creating ZIP packages for MSDOS executables in English..."

packFile="$current_folder/$packDir/teo-$teoversion-dosexe-en.zip"
cp $packDir/msdos/teo-en.exe    teo/teo.exe
cp $packDir/msdos/sap2-en.exe   teo/sap2.exe
cp $packDir/msdos/sapfs-en.exe  teo/sapfs.exe
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
teo/cc90hfe.exe
teo/cc90hfe-com.exe
teo/*.dll"

echo "Creating ZIP packages for Windows executables in French..."

packFile="$current_folder/$packDir/teo-$teoversion-winexe-fr.zip"
cp $packDir/mingw/teow-fr.exe        teo/teow.exe
cp $packDir/mingw/cc90hfe-fr.exe     teo/cc90hfe.exe
cp $packDir/mingw/cc90hfe-com-fr.exe teo/cc90hfe-com.exe
cp $packDir/msdos/sap2-fr.exe        teo/sap2.exe
cp $packDir/msdos/sapfs-fr.exe       teo/sapfs.exe
cp $packDir/msdos/wav2k7-fr.exe      teo/wav2k7.exe
cp $packDir/msdos/wav2k7-fr.exe      teo/wav2k7.exe
zip -r $zipOptions $packFile $common_exec $windows_exec teo/*-fr.*
rm teo/teow.exe teo/sap2.exe teo/sapfs.exe teo/wav2k7.exe teo/cc90hfe.exe teo/cc90hfe-com.exe

echo "Creating ZIP packages for Windows executables in English..."

packFile="$current_folder/$packDir/teo-$teoversion-winexe-en.zip"
cp $packDir/mingw/teow-en.exe        teo/teow.exe
cp $packDir/mingw/cc90hfe-en.exe     teo/cc90hfe.exe
cp $packDir/mingw/cc90hfe-com-en.exe teo/cc90hfe-com.exe
cp $packDir/msdos/sap2-en.exe        teo/sap2.exe
cp $packDir/msdos/sapfs-en.exe       teo/sapfs.exe
cp $packDir/msdos/wav2k7-en.exe      teo/wav2k7.exe
zip -r $zipOptions $packFile $common_exec $windows_exec teo/*-en.*
rm teo/teow.exe teo/sap2.exe teo/sapfs.exe teo/wav2k7.exe teo/cc90hfe.exe teo/cc90hfe-com.exe

# ----------------- create ZIP package for sources

echo "Creating ZIP package for sources..."

packFile="$current_folder/$packDir/teo-$teoversion-src.zip"
zip -r $zipOptions $packFile $source_files

# ----------------- delete temporary folders and files

rm -r ./teo

# ----------------- return to current folder

cd $current_folder/teo

echo "Packages created in ./misc/pack/!"

