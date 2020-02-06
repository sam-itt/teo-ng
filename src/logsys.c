#include "logsys.h"

#include <stdio.h>
#include <stdbool.h>


#if PLATFORM_OGXBOX
#include <SDL.h>
#include <hal/debug.h>
#include <hal/video.h>
#include <hal/xbox.h>
#include <windows.h>
#include <string.h>
#include <hal/audio.h>
#include <xboxkrnl/xboxkrnl.h>
#include <assert.h>
#else
#include <SDL2/SDL.h> //Make YouCompleteMe Happy
#endif

#if PLATFORM_OGXBOX
#define TIMING_FILENAME "D:\\xbox-timings.raw"
#else
#define TIMING_FILENAME "pc-timings.raw"
#endif


int logLevel = TRACE;

FILE *logfile;

#define USE_POSIX 1

#if defined(PLATFORM_OGXBOX) && !USE_POSIX
HANDLE timings = INVALID_HANDLE_VALUE;
#else
FILE *timings = NULL;
#endif


char *levelStr[7] = {
	"[ALL]",
	"[TRACE]",
	"[DEBUG]",
	"[INFO]",
	"[WARNING]",
	"[ERROR]",
	"[FATAL]"
};

void log_open(const char *filename) {
	logfile = fopen(filename, "w");
	if(logfile == NULL) {
		log_msgf(ERROR, "Unable to create log \"%s\".\n", filename);
	}
}

void log_close() {
	if(logfile == NULL) return;
	log_msgf(DEBUG, "Closing log file.\n");
	fclose(logfile);
}
/*
void log_msg(int level, const char *msg) {
	if(logfile == NULL || logLevel > level) return;
	fprintf(logfile, "%s %s", levelStr[level], msg);
}
*/
void log_msgf(int level, const char *format, ...) {
	if(logfile == NULL || logLevel > level) return;
	fprintf(logfile, "%s ", levelStr[level]);
	va_list args;
	va_start(args, format);
	vfprintf(logfile, format, args);
	va_end(args);
}

static void write_data(void *file, void *data, size_t size, size_t nmemb)
{
#if defined(PLATFORM_OGXBOX) && !USE_POSIX
    if((HANDLE)file != INVALID_HANDLE_VALUE){
        DWORD written;
        WriteFile((HANDLE)file, data, size*nmemb, &written, NULL);
    }
#else
        if((FILE*)file != NULL)
            fwrite(data, size, nmemb, (FILE*)file);
#endif
}


static void log_write_simple_event(enum event_type etype)
{
    Uint32 stamp;

    stamp = SDL_GetTicks();
    write_data(timings, &stamp, sizeof(Uint32), 1);
    write_data(timings, &etype, sizeof(enum event_type), 1);
}

void log_event_start(void)
{
#if defined(PLATFORM_OGXBOX) && !USE_POSIX
    timings = CreateFileA(TIMING_FILENAME, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                              CREATE_ALWAYS, FILE_FLAG_RANDOM_ACCESS, NULL);
    if(timings == INVALID_HANDLE_VALUE)
        debugPrint("Couldn't open timings file !\n");
    else
        debugPrint("Successfuly opened timings file !\n");
#else
    timings = fopen(TIMING_FILENAME,"wb");
    if(!timings)
        printf("Couldn't open timings file !\n");
    else
        printf("Successfuly opened timings file !\n");
#endif
}

void log_event_stop(void)
{
#if defined(PLATFORM_OGXBOX) && !USE_POSIX
    if(timings != INVALID_HANDLE_VALUE)
        CloseHandle(timings);
#else
    if(timings)
        fclose(timings);
#endif
}

void log_event(enum event_type etype, ...)
{
	va_list args;

    bool rv;
    Uint32 qlen;
    int buffer_size;

    log_write_simple_event(etype);

	va_start(args, etype);


    switch(etype){
#if 1
    case MAINLOOP_START:
    case MAINLOOP_END:
    case MAINLOOP_DOING_TEOFRAME:
    case MAINLOOP_DONE_TEOFRAME:
    case MAINLOOP_DOING_SDL:
    case MAINLOOP_DONE_SDL:
    case MAINLOOP_DOING_SDL_GFX:
    case MAINLOOP_DONE_SDL_GFX:
    case MAINLOOP_DOING_SDL_EVENTS:
    case MAINLOOP_DONE_SDL_EVENTS:
    case MAINLOOP_DOING_SOUND:
    case MAINLOOP_DONE_SOUND:
    case MAINLOOP_DOING_DISK:
    case MAINLOOP_DONE_DISK:
    case SOUND_DONE_SYNC:
        break;
#endif
    case SOUND_BEFORE_PUSH:
    case SOUND_DOING_SYNC:
        qlen = va_arg(args, Uint32);
        write_data(timings, &qlen, sizeof(Uint32), 1);
        break;
    case SOUNDSYNC_RETURN:
        rv = va_arg(args, int);
        qlen = va_arg(args, Uint32);
        write_data(timings, &rv, sizeof(int), 1);
        write_data(timings, &qlen, sizeof(Uint32), 1);
        break;

    case SOUND_PUSHED_SAMPLES:
    case SOUND_PUSHED_BUFFER_SAMPLES:
        buffer_size = va_arg(args, int);
        qlen = va_arg(args, Uint32);
        write_data(timings, &buffer_size, sizeof(int), 1);
        write_data(timings, &qlen, sizeof(Uint32), 1);
        break;

    case SOUND_BUFFERING:
        buffer_size = va_arg(args, int);
        write_data(timings, &buffer_size, sizeof(int), 1);
        break;
    }
	va_end(args);
}

