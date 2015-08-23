
            TTTTTTTTTTTTTT  EEEEEEEEEEEEEE  OOOOOOOOOOOOOO
            TTTTTTTTTTTTTT  EEEEEEEEEEEEEE  OOOOOOOOOOOOOO
                  TT        EE              OO          OO
                  TT        EE              OO          OO
                  TT        EE              OO          OO
                  TT        EEEEEEEEEE      OO          OO
                  TT        EEEEEEEEEE      OO          OO
                  TT        EE              OO          OO
                  TT        EE              OO          OO
                  TT        EE              OO          OO
                  TT        EEEEEEEEEEEEEE  OOOOOOOOOOOOOO
                  TT        EEEEEEEEEEEEEE  OOOOOOOOOOOOOO

                        The Thomson TO8 emulator
                              version 1.8.4

    Copyright (C) 1997-2015 Gilles Fétis, Eric Botcazou, Alexandre Pukall,
                            Jérémie Guillaume, François Mouret,
                            Samuel Devulder


Introduction
------------
Teo is an emulator of the Thomson TO8 microcomputer for PC, running on MSDOS,
Windows and Linux. It has been initiated by Gilles Fétis and developed by
Gilles Fétis, Eric Botcazou, Alexandre Pukall, Jérémie Guillaume,
François Mouret and Samuel Devulder.


How to get it ?
---------------
Download it at the page :

   http://sourceforge.net/projects/teoemulator/

The main archive contains the executable of the emulator and the full
documentation.


Compilation
--------------------
See makefile.all file for compilation options

In a console under Windows/MsDos systems (djgpp) :
1. Run ./fixdoscr.sh under Unix systems
2. Run fixmingw for MinGw version, fixdjgpp for MsDos version
3. Run make (ad libitum)

In a terminal under Unix systems :
1. Run ./fixunix.sh
2. Run make (ad libitum)


Packages compilation
--------------------
Packages compilation needs SED unix command.

In a terminal under Unix systems :
1. Run ./fixdoscr.sh
In a console under Windows/MsDos systems (djgpp) :
2. Delete misc\pack\msdos folder if exists
3. Run misc\pack\djmake
In a console under Windows systems (mingw) :
4. Run misc\pack\mgwmake
5. Compile misc\pack\inno\teo-setup.iss with Inno Setup
In a terminal under Unix systems :
6. Run ./misc/pack/pack.sh
Packages are created in misc/pack/.


Compatibility with the TO8
--------------------------
The compatibility is close to 100% for softwares which don't use non
emulated peripherals and don't hold physical protections. In other words,
if a software doesn't work on Teo, so probably :
- it needs the presence of another peripheral than mouse, light-pen,
  joystick, tape recorder and disks (so it won't run as long as this
  peripheral is not emulated),
- or its physical protection makes it fail.

We keep a list of the softwares which run on Teo; if you have one uhat
causes!a!psoclem- seod!iu uo!us,!we'll try to identify the cause of the
malfunction and tell you if it's possible to remedy it.


Known problems
--------------
- the automatic detection of the sound card on MSDOS version could fail;
  in this case, you can specify manually the characteristics of the card
  (kind of card, port address, DMA channel and IRQ number) by editing the
  file teo.cfg in the main folder.

