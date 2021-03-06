@echo off

REM  !!!! NEEDS SED UNIX COMMAND !!!!

echo Configuring Teo sources for the DOS(djgpp) platform...
echo ------------------------------------------------------

echo # generated by fixdjgpp.bat > makefile
echo MAKEFILE_INC = makefile.dj >> makefile
echo include makefile.all >> makefile

REM ---- Creates folders ----

mkdir misc\pack\repo\msdos\en
mkdir misc\pack\repo\msdos\fr

echo -----------------------------------------------
echo Creating Teo executable for MSDOS in English...
echo -----------------------------------------------

make veryclean
make ENGLISH=1
make depend
if exist misc\pack\repo\msdos\en\teo.exe del misc\pack\repo\msdos\en\teo.exe
copy teo.exe misc\pack\repo\msdos\en\teo.exe
if exist misc\pack\repo\msdos\en\teo.dep del misc\pack\repo\msdos\en\teo.dep
copy obj\djgpp\makefile.dep misc\pack\repo\msdos\en\teo.dep

echo ----------------------------------------------
echo Creating Teo executable for MSDOS in French...
echo ----------------------------------------------

make veryclean
make FRENCH=1
make depend
if exist misc\pack\repo\msdos\fr\teo.exe del misc\pack\repo\msdos\fr\teo.exe
copy teo.exe misc\pack\repo\msdos\fr\teo.exe
if exist misc\pack\repo\msdos\fr\teo.dep del misc\pack\repo\msdos\fr\teo.dep
copy obj\djgpp\makefile.dep misc\pack\repo\msdos\fr\teo.dep
make veryclean

echo -----------------------------------------------------
echo Creating Saptools executables for MSDOS in English...
echo -----------------------------------------------------

cd tools
cd sap
make clean
make ENGLISH=1
cd ..
cd ..
if exist misc\pack\repo\msdos\en\sap2.exe del misc\pack\repo\msdos\en\sap2.exe
copy tools\sap\sap2.exe misc\pack\repo\msdos\en\sap2.exe
if exist misc\pack\repo\msdos\en\sapfs.exe del misc\pack\repo\msdos\en\sapfs.exe
copy tools\sap\sapfs.exe misc\pack\repo\msdos\en\sapfs.exe

echo ----------------------------------------------------
echo Creating Saptools executables for MSDOS in French...
echo ----------------------------------------------------

cd tools
cd sap
make clean
make FRENCH=1
cd ..
cd ..
if exist misc\pack\repo\msdos\fr\sap2.exe del misc\pack\repo\msdos\fr\sap2.exe
copy tools\sap\sap2.exe misc\pack\repo\msdos\fr\sap2.exe
if exist misc\pack\repo\msdos\fr\sapfs.exe del misc\pack\repo\msdos\fr\sapfs.exe
copy tools\sap\sapfs.exe misc\pack\repo\msdos\fr\sapfs.exe

echo ----------------------
echo Clean Saptools objects
echo ----------------------

cd tools
cd sap
make clean
cd ..
cd ..

echo ----------------------------------------------------
echo Creating K7tools executables for MSDOS in English...
echo ----------------------------------------------------

cd tools
cd k7tools
make clean
make ENGLISH=1
cd ..
cd ..
if exist misc\pack\repo\msdos\en\wav2k7.exe del misc\pack\repo\msdos\en\wav2k7.exe
copy tools\k7tools\wav2k7.exe misc\pack\repo\msdos\en\wav2k7.exe

echo ---------------------------------------------------
echo Creating K7tools executables for MSDOS in French...
echo ---------------------------------------------------

cd tools
cd k7tools
make clean
make FRENCH=1
cd ..
cd ..
if exist misc\pack\repo\msdos\fr\wav2k7.exe del misc\pack\repo\msdos\fr\wav2k7.exe
copy tools\k7tools\wav2k7.exe misc\pack\repo\msdos\fr\wav2k7.exe

echo ---------------------
echo Clean K7Tools objects
echo ---------------------

cd tools
cd k7tools
make clean
cd ..
cd ..

echo -----
echo Done!
echo -
