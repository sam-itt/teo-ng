Teo-ng is a continuation of TEO, a Thomson TO8 emulator.

It brings more features such as:

-Allegro support on Linux
-Editable symbolic keyboard mapping using text files (all platforms)
-Better integration within Linux filesystem
-Now using autotools instead of the old static makefiles based buildsystem
-Cross-compiling support from Linux to Win32(Mingw) and MS-DOS (DJGPP)
-Fixed a bug (couldn't load disk images) on the MS-DOS version
-Now using NSIS install system for Win32

Compiling:
---------
$ ./configure --prefix=/usr --sysconfdir=/etc
Allegro: --with-gfx-backend=allegro
GTK/X11: --with-gfx-backend=gtk-x11
$ make
# make install

Cross-compile for MS-DOS or Win32 using --host= and your cross compiler
basename



Known issues
----------------
- MS-DOS: Automatic sound card detection *can* fail. If that occurs
you can still manually configure correct settings (card type, port,
DMA channel and IRQ #) in the allegro.cfg config file.
