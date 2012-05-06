@echo off

REM Compiles MINGW versions

if exist misc\pack\inno\teow-??.exe del misc\pack\inno\teow-??.exe

echo "Creating exec files for Windows setup in English..."

mingw32-make clean
mingw32-make EN_LANG=1
copy teow.exe misc\pack\inno
ren misc\pack\inno\teow.exe teow-en.exe

echo "Creating exec files for Windows setup in French..."

mingw32-make clean
mingw32-make FR_LANG=1
copy teow.exe misc\pack\inno
ren misc\pack\inno\teow.exe teow-fr.exe

mingw32-make clean
mingw32-make depend

echo "Create self-install files with Inno Setup now."

