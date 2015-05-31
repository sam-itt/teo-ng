@echo off

REM  !!!! NEEDS SED UNIX COMMAND !!!!

REM  Configuring Teo sources for the Windows (MinGW) platform...

echo # generated by mgwmake.bat > makefile
echo MAKEFILE_INC = makefile.mgw >> makefile
echo include makefile.all >> makefile

REM Creates folders

mkdir misc\pack\mingw
mkdir misc\pack\mingw\en
mkdir misc\pack\mingw\fr

echo -------------------------------------------------------
echo Creating Teo exec files for Windows setup in English...
echo -------------------------------------------------------

mingw32-make veryclean
mingw32-make ENGLISH=1
mingw32-make depend
if exist misc\pack\mingw\en\teow.exe del misc\pack\mingw\en\teow.exe
copy teow.exe misc\pack\mingw\en\teow.exe
if exist misc\pack\mingw\en\teo.dep del misc\pack\mingw\en\teo.dep
copy obj\mingw32\makefile.dep misc\pack\mingw\en\teo.dep

echo ------------------------------------------------------
echo Creating Teo exec files for Windows setup in French...
echo ------------------------------------------------------

mingw32-make veryclean
mingw32-make FRENCH=1
mingw32-make depend
if exist misc\pack\mingw\fr\teow.exe del misc\pack\mingw\fr\teow.exe
copy teow.exe misc\pack\mingw\fr\teow.exe
if exist misc\pack\mingw\fr\teo.dep del misc\pack\mingw\fr\teo.dep
copy obj\mingw32\makefile.dep misc\pack\mingw\fr\teo.dep

echo -----------------
echo Clean Teo objects
echo -----------------

mingw32-make veryclean

REM Configuring CC90HFE sources for the Windows (MinGW) platform...

cd tools
cd cc90hfe
echo # generated by fixmingw.bat > makefile
echo MAKEFILE_INC = makefile.mgw >> makefile
echo include makefile.all >> makefile
cd ..
cd ..

echo --------------------------------------------------------------------
echo Creating windowed CC90HFE exec files for Windows setup in English...
echo --------------------------------------------------------------------

cd tools
cd cc90hfe
mingw32-make veryclean
mingw32-make ENGLISH=1
mingw32-make depend
cd ..
cd ..
if exist misc\pack\mingw\en\cc90hfe.exe del misc\pack\mingw\en\cc90hfe.exe
copy tools\cc90hfe\cc90hfe.exe misc\pack\mingw\en\cc90hfe.exe
if exist misc\pack\mingw\en\cc90hfe.dep del misc\pack\mingw\en\cc90hfe.dep
copy tools\cc90hfe\obj\mingw32\makefile.dep misc\pack\mingw\en\cc90hfe.dep

echo -------------------------------------------------------------------
echo Creating windowed CC90HFE exec files for Windows setup in French...
echo -------------------------------------------------------------------

cd tools
cd cc90hfe
mingw32-make veryclean
mingw32-make FRENCH=1
mingw32-make depend
cd ..
cd ..
if exist misc\pack\mingw\fr\cc90hfe.exe del misc\pack\mingw\fr\cc90hfe.exe
copy tools\cc90hfe\cc90hfe.exe misc\pack\mingw\fr\cc90hfe.exe
if exist misc\pack\mingw\fr\cc90hfe.dep del misc\pack\mingw\fr\cc90hfe.dep
copy tools\cc90hfe\obj\mingw32\makefile.dep misc\pack\mingw\fr\cc90hfe.dep

echo -------------------------------------------------------------------
echo Creating CC90HFE-console exec files for Windows setup in English...
echo -------------------------------------------------------------------

cd tools
cd cc90hfe
mingw32-make clean
mingw32-make ENGLISH=1 CONSOLEMODE=1
cd ..
cd ..
if exist misc\pack\mingw\en\cc90hfe-com.exe del misc\pack\mingw\en\cc90hfe-com.exe
copy tools\cc90hfe\cc90hfe.exe misc\pack\mingw\en\cc90hfe-com.exe

echo ------------------------------------------------------------------
echo Creating CC90HFE-console exec files for Windows setup in French...
echo ------------------------------------------------------------------

cd tools
cd cc90hfe
mingw32-make clean
mingw32-make FRENCH=1 CONSOLEMODE=1
cd ..
cd ..
if exist misc\pack\mingw\fr\cc90hfe-com.exe del misc\pack\mingw\fr\cc90hfe-com.exe
copy tools\cc90hfe\cc90hfe.exe misc\pack\mingw\fr\cc90hfe-com.exe


echo ---------------------
echo Clean Cc90hfe objects
echo ---------------------
cd tools
cd cc90hfe
mingw32-make veryclean
cd ..
cd ..

echo ----------------------------------------------
echo Create self-install files with Inno Setup now.
echo -
