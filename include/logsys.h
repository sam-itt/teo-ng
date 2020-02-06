#ifndef TETRIS_LOGSYS
#define TETRIS_LOGSYS

#include <stdarg.h>

// Log levels
enum {
	ALL,     // Everything
	TRACE,   // Excessive debugging
	DEBUG,   // Verbose information
	INFO,    // Information
	WARNING, // Potential problem
	ERROR,   // Something went wrong
	FATAL    // Crash
};

enum event_type{
    MAINLOOP_START,
    MAINLOOP_END,
    MAINLOOP_DOING_TEOFRAME,
    MAINLOOP_DONE_TEOFRAME,
    MAINLOOP_DOING_SDL,
    MAINLOOP_DONE_SDL,
    MAINLOOP_DOING_SOUND,
    MAINLOOP_DONE_SOUND,
    MAINLOOP_DOING_DISK,
    MAINLOOP_DONE_DISK,
    SOUNDSYNC_RETURN,
    SOUND_DOING_SYNC,
    SOUND_DONE_SYNC,
    SOUND_PUSHED_SAMPLES,
    SOUND_PUSHED_BUFFER_SAMPLES,
    SOUND_BUFFERING,
    SOUND_BEFORE_PUSH,
    MAINLOOP_DOING_SDL_GFX,
    MAINLOOP_DONE_SDL_GFX,
    MAINLOOP_DOING_SDL_EVENTS,
    MAINLOOP_DONE_SDL_EVENTS,
    TYPE_LAST
};


// Opens a log file to write to
void log_open(const char *filename);

// Closes log file if one is open
void log_close();

// Log simple message
//void log_msg(int level, const char *msg);

// Log with formatting, syntax like fprintf
void log_msgf(int level, const char *format, ...);



void log_event_start(void);
void log_event_stop(void);
void log_event(enum event_type etype, ...);

#endif
