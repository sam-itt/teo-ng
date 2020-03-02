## [0.0.9b] - 2020-03-XX

### Added
- Allegro support on Linux (joystick support and fullscreen enabled)
- Cross-compiling support from Linux to Win32(Mingw) and MS-DOS (DJGPP)
- SDL2 port (Linux, og-xbox, Win32, OS X) with virtual keyboard and full joystick control
- Original Xbox port
- OS X Bundle
- Fixed a bug (couldn't load disk images) on the 
- Logging facility (compile time activation)
- Make target to build binary dist for: Win32, OS X and MS-DOS

### Changed
- Editable symbolic keyboard mapping using text files (all platforms)
- Better integration within Linux filesystem
- Use autotools instead of static makefiles
- Use NSIS install system for Win32 instead of Inno Setup
- Use Gettext instead of static strings with compile-time language select

### Fixed
- MS-DOS: Couldn't load disk images
- Various segfaults found by libefence in hfe.c and printer.c

