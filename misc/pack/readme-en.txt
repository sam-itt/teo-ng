
                        CREATING THE PACKAGES

The folowing scripts allow to create easily distributables packages
for Teo and Cc90hfe.
'pack.sh' must be the last to be launched.

djmake.bat (under MsDos)
------------------------

Run 'misc\pack\djmake.bat' from the folder '\teo'.
'djmake.bat' compile the MSDOS executables in the folders 'misc\pack\msdos\en'
and 'misc\pack\msdos\fr' (the folders are created if it doesn't exist).

mgwmake.bat (under Mingw)
-------------------------

Run 'misc\pack\mgwmake.bat' from the folder '\teo'.
'mgwmake.bat' compile the executables in the folders 'misc/pack/mingw/en'
and 'misc/pack/mingw/fr'  (the folders are created if they don't exist).
The self-extract package will be created by double-clicking and compiling
the '.iss' file. Inno Setup must be installed (http://www.innosetup.com/).

'inno\teo-big-img.bmp' and 'inno\teo-small-img.bmp' are used by the
'*.iss' files to decorate the installer window.

pack.sh (under Linux)
---------------------

Run './misc/pack/pack.sh' from the folder 'teo/'.

All the packages are then in the folder 'misc/pack/'.

