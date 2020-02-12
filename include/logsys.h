#ifndef TEO_LOGSYS_H
#define TEO_LOGSYS_H

#include <stdarg.h>

#ifdef DEBUG
#undef DEBUG
#endif
// Log levels
enum {
	LOG_ALL,     // Everything
	LOG_TRACE,   // Excessive debugging
	LOG_DEBUG,   // Verbose information
	LOG_INFO,    // Information
	LOG_WARNING, // Potential problem
	LOG_ERROR,   // Something went wrong
	LOG_FATAL,   // Crash

    LOG_NONE
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
    SOUND_QLEN_SPEED,
    SOUND_QLEN_SPEED_FULL,


    TYPE_LAST
};

/*Standard text loggging*/
void log_open(const char *filename);
void log_close();

void log_msgf(int level, const char *format, ...);
void log_vamsgf(int level, const char *format, va_list ap);

/*Binary log timing of specific portion of code*/
void log_event_start(void);
void log_event_stop(void);

#ifdef ENABLE_BINLOG
void log_event(enum event_type etype, ...);
#else
#define log_event (void)sizeof
#endif

#endif
