#AM_CPPFLAGS = @AM_CPPFLAGS@

bin_PROGRAMS = getmemo7 getrom wav2k7

getmemo7_SOURCES = getmemo7.c
if PLATFORM_WIN32
getmemo7_LDFLAGES=--subsystem,console 
getmemo7_LDADD=-lkernel32 -lcomctl32 -luser32 -lgdi32 -lcomdlg32 -lole32 -lshell32
endif #PLATFORM_WIN32


getrom_SOURCES = getrom.c
if PLATFORM_WIN32
getrom_LDFLAGES=--subsystem,console 
getrom_LDADD=-lkernel32 -lcomctl32 -luser32 -lgdi32 -lcomdlg32 -lole32 -lshell32
endif #PLATFORM_WIN32

wav2k7_SOURCES = wav2k7.c
if PLATFORM_WIN32
wav2k7_LDFLAGES=--subsystem,console 
wav2k7_LDADD=-lkernel32 -lcomctl32 -luser32 -lgdi32 -lcomdlg32 -lole32 -lshell32
endif #PLATFORM_WIN32

