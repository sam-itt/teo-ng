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

if exist misc\pack\msdos\teo-??.exe del misc\pack\msdos\teo-??.exe

REM ---- Compiles English version ----

echo "Creating executable for MSDOS in English..."

make veryclean
make EN_LANG=1
copy teo.exe misc\pack\msdos\teo.exe
ren misc\pack\msdos\teo.exe teo-en.exe

REM ---- Compiles French version ----

echo "Creating executable for MSDOS in French..."

make clean
make FR_LANG=1
copy teo.exe misc\pack\msdos\teo.exe
ren misc\pack\msdos\teo.exe teo-fr.exe
make depend

echo Done!
