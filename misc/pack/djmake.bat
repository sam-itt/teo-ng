@echo off

REM ---- Creates 'misc\pack\msdos' folder if not exists ----

echo test > misc\pack\msdos\test.txt
if exist misc\pack\msdos\test.txt goto SkipCreate
mkdir misc\pack\msdos
goto StartCompile
:SkipCreate
del misc\pack\msdos\test.txt
:StartCompile

REM ---- Cleans executable files ----

if exist misc\pack\msdos\*-??.exe del misc\pack\msdos\*-??.exe

REM ---- Compiles Teo English version ----

echo "Creating Teo executable for MSDOS in English..."

make veryclean
make EN_LANG=1
copy teo.exe misc\pack\msdos\teo.exe
ren misc\pack\msdos\teo.exe teo-en.exe

REM ---- Compiles saptools English version ----

echo "Creating Saptools executables for MSDOS in English..."

cd sap
make clean
make EN_LANG=1
cd ..
copy sap\sap2.exe misc\pack\msdos\sap2.exe
ren misc\pack\msdos\sap2.exe sap2-en.exe
copy sap\sapfs.exe misc\pack\msdos\sapfs.exe
ren misc\pack\msdos\sapfs.exe sapfs-en.exe

REM ---- Compiles k7tools English version ----

echo "Creating K7tools executables for MSDOS in English..."

cd k7tools
make clean
make EN_LANG=1
cd ..
copy k7tools\getmemo7.exe misc\pack\msdos\getmemo7.exe
ren misc\pack\msdos\getmemo7.exe getmemo7-en.exe
copy k7tools\getrom.exe misc\pack\msdos\getrom.exe
ren misc\pack\msdos\getrom.exe getrom-en.exe
copy k7tools\wav2k7.exe misc\pack\msdos\wav2k7.exe
ren misc\pack\msdos\wav2k7.exe wav2k7-en.exe

REM ---- Compiles Teo French version ----

echo "Creating executable for MSDOS in French..."

make clean
make FR_LANG=1
copy teo.exe misc\pack\msdos\teo.exe
ren misc\pack\msdos\teo.exe teo-fr.exe
make depend

REM ---- Compiles saptools French version ----

echo "Creating Saptools executables for MSDOS in French..."

cd sap
make clean
make FR_LANG=1
cd ..
copy sap\sap2.exe misc\pack\msdos\sap2.exe
ren misc\pack\msdos\sap2.exe sap2-fr.exe
copy sap\sapfs.exe misc\pack\msdos\sapfs.exe
ren misc\pack\msdos\sapfs.exe sapfs-fr.exe

REM ---- Compiles k7tools French version ----

echo "Creating K7tools executables for MSDOS in French..."

cd k7tools
make clean
make FR_LANG=1
cd ..
copy k7tools\getmemo7.exe misc\pack\msdos\getmemo7.exe
ren misc\pack\msdos\getmemo7.exe getmemo7-fr.exe
copy k7tools\getrom.exe misc\pack\msdos\getrom.exe
ren misc\pack\msdos\getrom.exe getrom-fr.exe
copy k7tools\wav2k7.exe misc\pack\msdos\wav2k7.exe
ren misc\pack\msdos\wav2k7.exe wav2k7-fr.exe


echo Done!

