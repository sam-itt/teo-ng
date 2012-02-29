
                        CREATION OF THE PACKAGES

The folowing scripts allow to create easily distributables packages
for Teo.
'pack.sh' must be the last to be launched.

djmake.bat
----------

Run 'misc\pack\djmake.bat' from the folder '\teo'.
'djmake.bat' compile the MSDOS executables in French and in English
in the folder 'misc\pack\msdos' (the folder is created if it
doesn't exist).

mgwmake.bat
-----------

Run 'misc\pack\mgwmake.bat' from the folder '\teo'.
'mgwmake.bat' compile the executables 'teow-en.exe' and 'teow-fr.exe'
in the folder 'misc/pack/inno'. The self-extract packages will then be
created by double-clicking and compiling '*.iss' files. Inno Setup must
be installed (http://www.innosetup.com/).

'inno\teo-big-img.bmp' and 'inno\teo-small-img.bmp' are used by the
'*.iss' files to decorate the installer window.

pack.sh
-------

Run './misc/pack/pack.sh' from the folder 'teo/'.

At execution, 'pack.sh' :
- Compiles the executable Linux for DEBIAN
- Creates the distribuable DEBIAN for the executable
- Creates le distribuable DEBIAN for the ROMs
- Compiles the executable Linux for TAR.GZ
- Creates the TAR.GZ distribuable Linux
- Creates the Window distribuable ZIP in French
- Creates the Window distribuable ZIP in English
- Creates the MsDos distribuable ZIP in French
- Creates the MsDos distribuable ZIP in English
- Creates the ZIP for the ROMs
- Creates the TAR.GZ for the ROMs
- Creates the ZIP for the sources
- Creates the TAR.GZ for the sources

All the packages are then in the folder 'misc/pack/'.

