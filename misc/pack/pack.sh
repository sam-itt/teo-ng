#!/bin/bash

set -e

#----------------------------------------------------------------
#
#              Create packages for Teo and Cc90hfe
#
#----------------------------------------------------------------

if [ `expr match $PWD '.*/teoemulator-code$'` = 0 ]
   then
      echo "Usage: ./misc/pack/pack.sh from teoemulator-code/ folder"
      exit 1
fi

# always set TMPDIR to something writable into!
if [ -z $TMPDIR ]; then
   TMPDIR="/tmp"
fi

teo_version='1.8.3'
cc90hfe_version='0.5.0'
pack_dir="teoemulator-code/misc/pack"
tmp_pack_dir="teoemulator-code/misc/pack"
zip_options="-q -9"
gzip_options="-9"

# list of system files
system_files="
teo/system/rom/*.rom
teo/system/printer/042/*.txt
teo/system/printer/055/*.txt
teo/system/printer/582/*.txt
teo/system/printer/600/*.txt
teo/system/printer/612/*.txt"

# list of common files for executable packages + users directories
common_exec="
$system_files
teo/disk
teo/memo
teo/cass"

# list of source files
source_files="
$system_files
teo/doc/wiki/*
teo/src
teo/include
teo/tests
teo/tools/sap
teo/tools/k7tools
teo/tools/cc90hfe/include
teo/tools/cc90hfe/obj/linux/makefile.dep
teo/tools/cc90hfe/obj/mingw32/makefile.dep
teo/tools/cc90hfe/src
teo/tools/cc90hfe/makefile.*
teo/tools/cc90hfe/fix*.*
teo/obj/linux/makefile.dep
teo/obj/djgpp/makefile.dep
teo/obj/mingw32/makefile.dep
teo/misc/fix*.sh
teo/misc/fixver/*.sh
teo/misc/pack/*.txt
teo/misc/pack/*.bat
teo/misc/pack/*.sh
teo/misc/pack/inno/*.iss
teo/misc/pack/inno/*.bmp
teo/misc/pack/debian
teo/cc90.*
teo/empty.hfe
teo/change-*.log
teo/licence-*.txt
teo/readme-*.txt
teo/fix*.*
teo/makefile.*
teo/allegro.cfg
teo/language.dat
teo/keyboard.dat
teo/alleg40.dll
teo/zlib1.dll
teo/libpng3.dll"



######################################################################
echo "Preparing working directories..."
#---------------------------------------------------------------------
#   Copy to temporary folder
#---------------------------------------------------------------------
cd ..
current_folder=$PWD
pack_storage="$current_folder/$pack_dir"
rm -f -r $TMPDIR/teo
mkdir $TMPDIR/teo
cp -r ./teoemulator-code/* $TMPDIR/teo
cd $TMPDIR

#---------------------------------------------------------------------
#   Convert files from DOS to UNIX
#---------------------------------------------------------------------
cd teo
chmod +x ./fixunix.sh
./fixunix.sh
cd ..

#---------------------------------------------------------------------
#   Clean user directories
#---------------------------------------------------------------------
rm -f -r ./disk
rm -f -r ./memo
rm -f -r ./cass
#---------------------------------------------------------------------
#   Clean package files
#---------------------------------------------------------------------
rm -f $pack_storage/*.tar*
rm -f $pack_storage/*.zip
rm -f $pack_storage/*.deb
#---------------------------------------------------------------------
#   Clean object files
#---------------------------------------------------------------------
rm -f -r ./teo/obj/mingw32
rm -f -r ./teo/obj/djgpp
rm -f -r ./teo/obj/linux
mkdir ./teo/obj/mingw32
mkdir ./teo/obj/djgpp
mkdir ./teo/obj/linux
#---------------------------------------------------------------------
#   Force MsDos file and directory names to lowercase
#---------------------------------------------------------------------
name_to_lowercase()
{
    if [ $1 != ${1,,*} ]
        then
            mv $1 ${1,,*}
    fi
}

if [ -e "teo/misc/pack/MSDOS" ]
    then
        name_to_lowercase "teo/misc/pack/MSDOS"
fi
if [ -e "teo/misc/pack/msdos/EN" ]
    then
        name_to_lowercase "teo/misc/pack/msdos/EN"
fi
if [ -e "teo/misc/pack/msdos/FR" ]
    then
        name_to_lowercase "teo/misc/pack/msdos/FR"
fi
for i in teo/misc/pack/msdos/fr/* teo/misc/pack/msdos/en/*
    do
         name_to_lowercase $i
    done
#---------------------------------------------------------------------
#   Copy makefile.dep's
#---------------------------------------------------------------------
if [ -e ./teo/misc/pack/mingw/fr/teo.dep ]
    then
        cp -f ./teo/misc/pack/mingw/fr/teo.dep ./teo/obj/mingw32/makefile.dep
fi
if [ -e ./teo/misc/pack/msdos/fr/teo.dep ]
    then
        cp -f ./teo/misc/pack/msdos/fr/teo.dep ./teo/obj/djgpp/makefile.dep
fi
if [ -e ./teo/misc/pack/mingw/fr/cc90hfe.dep ]
    then
        cp -f ./teo/misc/pack/mingw/fr/cc90hfe.dep ./teo/tools/cc90hfe/obj/mingw32/makefile.dep
fi



######################################################################
#---------------------------------------------------------------------
#   Function to build DEBIAN packages
#---------------------------------------------------------------------
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



######################################################################
echo "Preparing executable for Teo DEBIAN..."
#---------------------------------------------------------------------
#   Compile Teo
#---------------------------------------------------------------------
cd teo
make veryclean
make DEBIANBUILD=1
make depend
cd ..
#---------------------------------------------------------------------
#   Compile SapTools
#---------------------------------------------------------------------
cd teo/tools/sap
make clean
make
cd ../../..
cp teo/tools/sap/sap2  teo/
cp teo/tools/sap/sapfs teo/
#---------------------------------------------------------------------
#   Compile K7Tools
#---------------------------------------------------------------------
cd teo/tools/k7tools
make clean
make
cd ../../..
cp teo/tools/k7tools/wav2k7 teo/



######################################################################
echo "Creating Teo DEBIAN package..."
prog_name=teo-$teo_version-i586
#---------------------------------------------------------------------
#   Transfert DEBIAN file structure
#---------------------------------------------------------------------
sudo rm -r -f ~/$prog_name
cp -r $pack_storage/debian/teo ~
mv -f ~/teo ~/$prog_name
#---------------------------------------------------------------------
#   Create missing folders
#---------------------------------------------------------------------
mkdir ~/$prog_name/usr/games/
mkdir ~/$prog_name/usr/share/doc
mkdir ~/$prog_name/usr/share/doc/teo
mkdir ~/$prog_name/usr/share/teo
mkdir ~/$prog_name/usr/share/teo/system
mkdir ~/$prog_name/usr/share/teo/doc
mkdir ~/$prog_name/usr/share/teo/doc/images
#---------------------------------------------------------------------
#   Copy files into file structure
#---------------------------------------------------------------------
cp teo/teo            ~/$prog_name/usr/games/
cp teo/sap2           ~/$prog_name/usr/games/
cp teo/sapfs          ~/$prog_name/usr/games/
cp teo/wav2k7         ~/$prog_name/usr/games/
cp -r teo/system      ~/$prog_name/usr/share/teo
cp teo/empty.hfe      ~/$prog_name/usr/share/teo
cp teo/doc/images/*.* ~/$prog_name/usr/share/teo/doc/images
cp teo/doc/doc.css    ~/$prog_name/usr/share/teo/doc
cp teo/doc/*.htm      ~/$prog_name/usr/share/teo/doc
cp teo/readme-en.txt  ~/$prog_name/usr/share/doc/teo/README
cp teo/licence-en.txt ~/$prog_name/usr/share/doc/teo/copyright
cp teo/change-en.log  ~/$prog_name/usr/share/doc/teo/changelog

#---------------------------------------------------------------------
#   Build DEBIAN package
#---------------------------------------------------------------------
build_debian_package $prog_name $pack_storage



######################################################################
echo "Preparing executable for Cc90hfe DEBIAN..."
#---------------------------------------------------------------------
#   Compile Cc90hfe
#---------------------------------------------------------------------
cd teo/tools/cc90hfe
./fixunix.sh
make veryclean
make DEBIANBUILD=1
make depend
cd ../../..



######################################################################
echo "Creating Cc90hfe DEBIAN package..."
prog_name="cc90hfe-$cc90hfe_version-i586"
#---------------------------------------------------------------------
#   Transfert DEBIAN file structure
#---------------------------------------------------------------------
sudo rm -r -f ~/$prog_name
cp -r $pack_storage/debian/cc90hfe ~
mv -f ~/cc90hfe ~/$prog_name
#---------------------------------------------------------------------
#   Create missing folders
#---------------------------------------------------------------------
mkdir ~/$prog_name/usr/games/
mkdir ~/$prog_name/usr/share/cc90hfe
mkdir ~/$prog_name/usr/share/cc90hfe/doc
mkdir ~/$prog_name/usr/share/cc90hfe/doc/images
#---------------------------------------------------------------------
#   Copy files into file structure
#---------------------------------------------------------------------
cp teo/tools/cc90hfe/cc90hfe ~/$prog_name/usr/games/
cp teo/cc90.sap              ~/$prog_name/usr/share/cc90hfe
cp teo/cc90.fd               ~/$prog_name/usr/share/cc90hfe
cp teo/cc90.hfe              ~/$prog_name/usr/share/cc90hfe
cp teo/doc/doc.css           ~/$prog_name/usr/share/cc90hfe/doc
cp teo/doc/images/*.*        ~/$prog_name/usr/share/cc90hfe/doc/images
cp teo/doc/cc90*.htm         ~/$prog_name/usr/share/cc90hfe/doc
move_dir="$prog_name/usr/share/cc90hfe/doc"
mv ~/$move_dir/cc90hfe_en.htm ~/$move_dir/index.htm
mv ~/$move_dir/cc90hfe_fr.htm ~/$move_dir/index_fr.htm
#---------------------------------------------------------------------
#   Build DEBIAN package
#---------------------------------------------------------------------
build_debian_package $prog_name $pack_storage



######################################################################
echo "Creating Teo TAR.GZ package..."
#---------------------------------------------------------------------
#   Create media folders
#---------------------------------------------------------------------
mkdir teo/disk
mkdir teo/memo
mkdir teo/cass
#---------------------------------------------------------------------
#   Compile Teo
#---------------------------------------------------------------------
cd teo
make veryclean
make
make depend
cd ..
#---------------------------------------------------------------------
#   Compile saptools
#---------------------------------------------------------------------
cd teo/tools/sap
make clean
make
cd ../../..
cp teo/tools/sap/sap2  teo/
cp teo/tools/sap/sapfs teo/
#---------------------------------------------------------------------
#    Compile k7tools
#---------------------------------------------------------------------
cd teo/tools/k7tools
make clean
make
cd ../../..
cp teo/tools/k7tools/wav2k7 teo/
#---------------------------------------------------------------------
#   Compile cc90hfe
#---------------------------------------------------------------------
cd teo/tools/cc90hfe
./fixunix.sh
make veryclean
make
make depend
cd ../../..
cp teo/tools/cc90hfe/cc90hfe teo/
#---------------------------------------------------------------------
#   Create executable package
#---------------------------------------------------------------------
exec_list="
teo/teo
teo/sap2
teo/sapfs
teo/wav2k7
teo/cc90hfe
teo/cc90.sap
teo/cc90.fd
teo/cc90.hfe
teo/empty.hfe
teo/doc/images/*.*
teo/doc/*.htm
teo/doc/*.css
teo/change-*.log
teo/licence-*.txt
teo/readme-*.txt"
pack_file="$pack_storage/teo-$teo_version-i586.tar"
tar -cf $pack_file $common_exec $exec_list
gzip $gzip_options $pack_file



######################################################################
echo "Clean Linux executables..."
#---------------------------------------------------------------------
#   Clean Teo
#---------------------------------------------------------------------
cd teo
make clean
cd ..
#---------------------------------------------------------------------
#   Clean saptools
#---------------------------------------------------------------------
cd teo/tools/sap
make clean
cd ../../..
rm teo/sap2
rm teo/sapfs
#---------------------------------------------------------------------
#   Clean k7tools
#---------------------------------------------------------------------
cd teo/tools/k7tools
make clean
cd ../../..
rm teo/wav2k7
#---------------------------------------------------------------------
#   Clean cc90hfe
#---------------------------------------------------------------------
cd teo/tools/cc90hfe
make clean
cd ../../..
rm teo/cc90hfe

echo "Creating TAR.GZ package for sources..."
pack_doc="
teo/doc/images/*.*
teo/doc/*.htm
teo/doc/*.css"
pack_file="$pack_storage/teo-$teo_version-src.tar"
if [ -e "teo/obj/djgpp/makefile.dep" ]
    then
        tar -cf $pack_file $source_files $pack_doc
        gzip $gzip_options $pack_file
fi



######################################################################
#---------------------------------------------------------------------
#   Convert files from UNIX to DOS / functions for docs
#---------------------------------------------------------------------
cd teo
./fixdoscr.sh
cd ..

open_doc()
{
    local i
    rm -f -r teo/doc_tmp
    cp -r teo/doc teo/doc_tmp

    for i in teo/doc/*.htm
    do
        if [ ! ${i##*_} = "$1.htm" ]
          then
            rm "$i"
        fi
    done
    mv teo/doc/welcome_$1.htm teo/doc/index.htm
}

close_doc()
{
    rm -f -r teo/doc
    mv teo/doc_tmp/ teo/doc
}



######################################################################
#---------------------------------------------------------------------
#   List of files to add to MsDos package
#---------------------------------------------------------------------
exec_list="
teo/language.dat
teo/keyboard.dat
teo/teo.exe
teo/sap2.exe
teo/sapfs.exe
teo/wav2k7.exe
teo/cwsdpmi.exe
teo/allegro.cfg
teo/alleg40.dll
teo/CHANGES.TXT
teo/LICENCE.TXT
teo/README.TXT
teo/empty.hfe
teo/doc/*.htm
teo/doc/images/*.*
teo/doc/*.css"



######################################################################
echo "Creating ZIP packages for MSDOS executables in French..."
pack_file="$pack_storage/teo-$teo_version-dosexe-fr.zip"
if [ -e "teo/misc/pack/msdos/fr/teo.exe" ]
    then
        open_doc "fr"
        cp teo/misc/pack/msdos/fr/teo.exe    teo/
        cp teo/misc/pack/msdos/fr/sap2.exe   teo/
        cp teo/misc/pack/msdos/fr/sapfs.exe  teo/
        cp teo/misc/pack/msdos/fr/wav2k7.exe teo/
        cp teo/misc/pack/msdos/fr/wav2k7.exe teo/
        cp teo/change-fr.log       teo/CHANGES.TXT
        cp teo/licence-fr.txt      teo/LICENCE.TXT
        cp teo/readme-fr.txt       teo/README.TXT
        zip -r $zip_options $pack_file $common_exec $exec_list
        rm teo/teo.exe
        rm teo/sap2.exe
        rm teo/sapfs.exe
        rm teo/wav2k7.exe
        rm teo/CHANGES.TXT
        rm teo/LICENCE.TXT
        rm teo/README.TXT
        close_doc
fi



######################################################################
echo "Creating ZIP packages for MSDOS executables in English..."
pack_file="$pack_storage/teo-$teo_version-dosexe-en.zip"
if [ -e "teo/misc/pack/msdos/en/teo.exe" ]
    then
        open_doc "en"
        cp teo/misc/pack/msdos/en/teo.exe    teo/
        cp teo/misc/pack/msdos/en/sap2.exe   teo/
        cp teo/misc/pack/msdos/en/sapfs.exe  teo/
        cp teo/misc/pack/msdos/en/wav2k7.exe teo/
        cp teo/change-en.log       teo/CHANGES.TXT
        cp teo/licence-en.txt      teo/LICENCE.TXT
        cp teo/readme-en.txt       teo/README.TXT
        zip -r $zip_options $pack_file $common_exec $exec_list
        rm teo/teo.exe
        rm teo/sap2.exe
        rm teo/sapfs.exe
        rm teo/wav2k7.exe
        rm teo/CHANGES.TXT
        rm teo/LICENCE.TXT
        rm teo/README.TXT
        close_doc
fi



######################################################################
#---------------------------------------------------------------------
#   List of files to add to Windows package
#---------------------------------------------------------------------
exec_list="
teo/language.dat
teo/keyboard.dat
teo/teow.exe
teo/sap2.exe
teo/sapfs.exe
teo/wav2k7.exe
teo/cc90hfe.exe
teo/cc90hfe-com.exe
teo/cc90.sap
teo/cc90.fd
teo/cc90.hfe
teo/empty.hfe
teo/allegro.cfg
teo/alleg40.dll
teo/zlib1.dll
teo/libpng3.dll
teo/CHANGES.TXT
teo/LICENCE.TXT
teo/README.TXT
teo/doc/*.htm
teo/doc/images/*.*
teo/doc/*.css"



######################################################################
echo "Creating ZIP packages for Windows executables in French..."
packFile="$pack_storage/teo-$teo_version-winexe-fr.zip"
if [ -e "teo/misc/pack/mingw/fr/teow.exe" ]
    then
        open_doc "fr"
        cp teo/misc/pack/mingw/fr/teow.exe        teo/
        cp teo/misc/pack/mingw/fr/cc90hfe.exe     teo/
        cp teo/misc/pack/mingw/fr/cc90hfe-com.exe teo/
        cp teo/misc/pack/msdos/fr/sap2.exe        teo/
        cp teo/misc/pack/msdos/fr/sapfs.exe       teo/
        cp teo/misc/pack/msdos/fr/wav2k7.exe      teo/
        cp teo/change-fr.log  teo/CHANGES.TXT
        cp teo/licence-fr.txt teo/LICENCE.TXT
        cp teo/readme-fr.txt  teo/README.TXT
        zip -r $zip_options $packFile $common_exec $exec_list
        rm teo/teow.exe
        rm teo/sap2.exe
        rm teo/sapfs.exe
        rm teo/wav2k7.exe
        rm teo/cc90hfe.exe
        rm teo/cc90hfe-com.exe
        rm teo/CHANGES.TXT
        rm teo/LICENCE.TXT
        rm teo/README.TXT
        close_doc
fi



######################################################################
echo "Creating ZIP packages for Windows executables in English..."
packFile="$pack_storage/teo-$teo_version-winexe-en.zip"
if [ -e "teo/misc/pack/mingw/en/teow.exe" ]
    then
        open_doc "en"
        cp teo/misc/pack/mingw/en/teow.exe        teo/
        cp teo/misc/pack/mingw/en/cc90hfe.exe     teo/
        cp teo/misc/pack/mingw/en/cc90hfe-com.exe teo/
        cp teo/misc/pack/msdos/en/sap2.exe        teo/
        cp teo/misc/pack/msdos/en/sapfs.exe       teo/
        cp teo/misc/pack/msdos/en/wav2k7.exe      teo/
        cp teo/change-en.log  teo/CHANGES.TXT
        cp teo/licence-en.txt teo/LICENCE.TXT
        cp teo/readme-en.txt  teo/README.TXT
        zip -r $zip_options $packFile $common_exec $exec_list
        rm teo/teow.exe
        rm teo/sap2.exe
        rm teo/sapfs.exe
        rm teo/wav2k7.exe
        rm teo/cc90hfe.exe
        rm teo/cc90hfe-com.exe
        rm teo/CHANGES.TXT
        rm teo/LICENCE.TXT
        rm teo/README.TXT
        close_doc
fi



######################################################################
echo "Creating ZIP package for sources..."
packFile="$pack_storage/teo-$teo_version-src.zip"
doc_files="
teo/doc/*.htm
teo/doc/images/*.*
teo/doc/*.css"
zip -r $zip_options $packFile $source_files $doc_files



######################################################################
echo "Clean working directories..."
rm -r ./teo
cd $current_folder/teoemulator-code



######################################################################
echo "Packages created in ./misc/pack/!"

