# Teo-ng - Thomson TO8 emulator
-------------------------------
Teo-ng is a continuation of [Teo](https://sourceforge.net/projects/teoemulator/). It brings more features such as:

- Original Xbox port using [nxdk](https://github.com/XboxDev/nxdk)
- Allegro support on Linux (brings joystick suppport and fullscreen)
- SDL2 port(Win32, Linux, Xbox) with virtual keyboard and full joystick control
- Editable symbolic keyboard mapping using text files (all platforms)
- Better integration within Linux filesystem
- Cross-compiling support from Linux to Win32(Mingw) and MS-DOS (DJGPP)

# Getting Teo-ng
---------------
Head over to the [release](https://github.com/sam-itt/teo-ng/releases) page and pick binaries for your platform.

Otherwise build from source using instructions below.

# Compiling:
Although binaries packages are provided, Teo-ng can be built from a Linux host to target either:
- Linux
- OS X / macOS
- MS-DOS
- Win32
It can also be built to target the original Xbox (og-xbox).

Cross compiling can be tricky. Users that really want to build from source will need a cross-compiler and "cross dependencies" for the target they'll choose (SDL2, Allegro, etc).

### Dependencies:
- libpng
- -zlib
- glib-2.0 (Unix)
- Allegro4 (When building using Allegro)
- SDL2 (When building using SDL)
- Gettext (Except for xbox)
- When building for gtk-x11:
    - gtk+-3.X 
    - libX11
    - libXext
    - alsa-lib

### Frontend select:
- SDL2(default, Win32, Linux, Xbox, OS X): **--with-gfx-backend=sdl2**
- Allegro(MS-DOS,Win32,Linux): **--with-gfx-backend=allegro**
- GTK/X11(Linux): **--with-gfx-backend=gtk-x11**

## Linux

Here is an [ebuild](http://www.teo-ng.com/dist/teo-ng-0.9.0.ebuild) for Gentoo Linux. We accept contributions for other distros.

```sh
$ ./configure --prefix=/usr --sysconfdir=/etc
$ make
# make install
```
## OS X
To successfuly build the OS X version, you'll need [dylibbundler](https://github.com/auriamg/macdylibbundler). 
Using the prebuilt binary is recommended.

**Install dependencies using [homebrew](https://brew.sh/) before the build.**
```sh
$ ./configure --prefix=/usr --sysconfdir=/etc
$ make
$ make macos-bundle
$ open teo-ng.dmg
```

## Win32 and MS-DOS
Cross-compile for MS-DOS or Win32 using **--host=** and your cross compiler triplet.

### MS-DOS
```sh
$ ./configure --prefix=/usr --sysconfdir=/etc --with-gfx-backend=allegro --host=i586-pc-msdosdjgpp
$ make
$ make msdos-bindist
$ ls *.zip
```

### Win32
If you want to build a setup.exe you'll need [Nullsoft's NIS](https://sourceforge.net/projects/nsis/)

```sh
$ ./configure --prefix=/usr --sysconfdir=/etc --host=i686-w64-mingw32
$ make
$ make win32-bindist # Or make win32-setup that will produce setup.exe
$ ls *.zip # Or ls *.exe
```

### Original Xbox
To build the original xbox version, you'll need the [nxdk](https://github.com/XboxDev/nxdk/) installed. Make the NXDK_DIR env variable point to the location where you installed nxdk.
```sh
export NXDK_DIR=/path/to/nxdk
```
Then proceed with the build:
```sh
 make -f Makefile.ogxbox config
 make -f Makefile.ogxbox
 ```

Known issues
----------------
- MS-DOS: Automatic sound card detection *can* fail. If that occurs
you can still manually configure correct settings (card type, port,
DMA channel and IRQ #) in the allegro.cfg config file.
