#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "logsys.h"

#define FREQ 48000.0

//#define sizeToTime(a) ((double)((a)/(FREQ*1000.0)))

double sizeToTime(int a)
{
    return a/FREQ*1000;
}

int main(int argc, char **argv)
{
    FILE *fp;
    int i;

    int type;
    uint32_t stamp;
    int rv;
    uint32_t qlen;
    int len;

    uint32_t last_event[TYPE_LAST];

    if(argc < 2){
        printf("Usage: %s dump-log.raw\n",argv[0]);
        return -1;
    }

    fp = fopen(argv[1],"rb");
    i = 0;
    do{
        fread(&stamp, sizeof(uint32_t), 1, fp);
        fread(&type, sizeof(enum event_type), 1, fp);
        last_event[type] = stamp;

        switch(type){
        case MAINLOOP_START:
            printf("%d: Mainloop start\n",stamp);
            break;
        case MAINLOOP_END:
            printf("%d: Mainloop end: Duration: %d (ms)\n",stamp, stamp - last_event[MAINLOOP_START]);
            break;
        case MAINLOOP_DOING_TEOFRAME:
            printf("%d: Mainloop - doing teoframe\n",stamp);
            break;
        case MAINLOOP_DONE_TEOFRAME:
            printf("%d: Mainloop - done teoframe, duration: %d\n",stamp, stamp - last_event[MAINLOOP_DOING_TEOFRAME]);
            break;
        case MAINLOOP_DOING_SDL:
            printf("%d: Mainloop - doing SDL stuff (gfx, events, etc.)\n",stamp);
            break;
        case MAINLOOP_DONE_SDL:
            printf("%d: Mainloop - done SDL stuff (gfx, events, etc.), duration: %d\n",stamp, stamp - last_event[MAINLOOP_DOING_SDL]);
            break;
        case MAINLOOP_DOING_SDL_GFX:
            printf("%d: Mainloop - doing SDL Graphics\n",stamp);
            break;
        case MAINLOOP_DONE_SDL_GFX:
            printf("%d: Mainloop - done SDL Graphics, duration: %d\n",stamp, stamp - last_event[MAINLOOP_DOING_SDL_GFX]);
            break;
        case MAINLOOP_DOING_SDL_EVENTS:
            printf("%d: Mainloop - doing SDL Events\n",stamp);
            break;
        case MAINLOOP_DONE_SDL_EVENTS:
            printf("%d: Mainloop - done SDL Events, duration: %d\n",stamp, stamp - last_event[MAINLOOP_DOING_SDL_EVENTS]);
            break;
        case MAINLOOP_DOING_SOUND:
            printf("%d: Mainloop - doing sound\n",stamp);
            break;
        case MAINLOOP_DONE_SOUND:
            printf("%d: Mainloop - done sound, duration: %d\n",stamp, stamp - last_event[MAINLOOP_DOING_SOUND]);
            break;
        case MAINLOOP_DOING_DISK:
            printf("%d: Mainloop - doing disk\n",stamp);
            break;
        case MAINLOOP_DONE_DISK:
            printf("%d: Mainloop - done disk, duration: %d\n", stamp, stamp - last_event[MAINLOOP_DONE_DISK]);
            break;
        case SOUND_DOING_SYNC:
            fread(&qlen, sizeof(uint32_t), 1, fp);
            printf("%d: Sound - doing sync, qlen is %d (%0.2f ms) before entering waiting loop\n",stamp, qlen, sizeToTime(qlen));
            break;
        case SOUND_DONE_SYNC:
            printf("%d: Sound - done sync, duration: %d\n", stamp, stamp - last_event[SOUND_DOING_SYNC]);
            break;
        case SOUNDSYNC_RETURN:
            fread(&rv, sizeof(int), 1, fp);
            fread(&qlen, sizeof(uint32_t), 1, fp);
            printf("%d: SoundSync - returning %s, %d bytes remained in SQLQueue (%0.2f ms)\n", stamp, rv == 0 ? "false" : "true", qlen, sizeToTime(qlen));
            break;
        case SOUND_BEFORE_PUSH:
            fread(&qlen, sizeof(uint32_t), 1, fp);
            printf("%d: Sound - Just before pushing SDLQueue was %d (%0.2f ms) - %d ms since last push\n",stamp, qlen, sizeToTime(qlen), last_event[SOUND_PUSHED_SAMPLES]);
            break;
        case SOUND_PUSHED_SAMPLES:
            fread(&len, sizeof(int), 1, fp);
            fread(&qlen, sizeof(uint32_t), 1, fp);
            printf("%d: Sound - pushed %d bytes (%0.2f ms). SDLQueue size reports %d (%0.2f ms)\n", 
                    stamp, len, sizeToTime(len), qlen, sizeToTime(qlen));
            break;
        case SOUND_PUSHED_BUFFER_SAMPLES:
            fread(&len, sizeof(int), 1, fp);
            fread(&qlen, sizeof(uint32_t), 1, fp);
            printf("%d: Sound - de-buffering: pushed %d bytes from buffer (%0.2f ms). SDLQueue size reports %d (%0.2f ms)\n", 
                   stamp, len, sizeToTime(len), qlen, sizeToTime(qlen));
            break;
        case SOUND_BUFFERING:
            fread(&len, sizeof(int), 1, fp);
            printf("%d: Sound - buffering (not playing) %d bytes (%0.2f ms)\n", stamp, len, sizeToTime(len));
            break;
        }
    }while(!feof(fp));
}
