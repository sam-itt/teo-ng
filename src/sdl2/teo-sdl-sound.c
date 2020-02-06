#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include <SDL.h>

#include "teo.h"
#include "defs.h" //MIN/MAX
#include "sdl2/teo-sdl-sound.h"
#include "sdl2/teo-sdl-log.h"

//#define ENABLE_BINLOG 1
#undef ENABLE_BINLOG
#include "logsys.h"

#if PLATFORM_OGXBOX
#include <hal/debug.h>
#include <hal/video.h>
#include <hal/xbox.h>
#include <windows.h>
#include <string.h>
#include <hal/audio.h>
#include <xboxkrnl/xboxkrnl.h>
#include <assert.h>
#endif

#ifdef PLATFORM_OGXBOX
#define printf debugPrint
#endif
/*Half-polling test results:
 * -no effect on Xbox (but useless as we are
 *  the only running process)
 * -induces noise when fullscreen on Linux*/
#define ENABLE_HALF_POLL 0
/*Back buffer where we store data 
 * incoming from the virtual TO8
 * */
static int sound_freq;
static int sound_buffer_size;
static unsigned char *sound_buffer;
static unsigned char last_data;
static int last_index;

#define BUFFER_NFRAMES 10
static unsigned char *frame_holder[BUFFER_NFRAMES];
int frames_ahead = 0;
bool need_buffer = true;

SDL_AudioSpec native_spec;
static SDL_AudioSpec spec;
static SDL_AudioDeviceID dev_id = 0;

#define ENABLE_SOUND_RECORDER 0
#if ENABLE_SOUND_RECORDER
#if PLATFORM_OGXBOX
#define PCM_FILENAME "D:\\xbox-dump.raw"
#else
#define PCM_FILENAME "pc-dump.raw"
#endif //PLATFORM_OGXBOX
FILE *output = NULL;
static bool dump_pcm = false;


void teoSDL_SoundRecordFrame(void)
{
    if(!dump_pcm) return;
    if(output){
        fwrite(sound_buffer, sizeof(uint8_t), sound_buffer_size, output);
    }
}

void teoSDL_SoundInitRecorder(void)
{
    if(dump_pcm){
        output = fopen(PCM_FILENAME,"wb");
        if(!output)
            printf("Couldn't open sound dump file !\n");
        else
            printf("Successfuly opened sound dump file !\n");
    }
}

void teoSDL_SoundShutdownRecorder(void)
{
    if(output)
        fclose(output);
}
#endif //ENABLE_SOUND_RECORDER


static void teoSDL_SoundDumpSpec(SDL_AudioSpec *spec)
{
    printf("Spec %p:\n", spec);
    printf("\tfreq: %d\n",spec->freq);
    printf("\tformat: %d\n",spec->format);
    printf("\tchannels: %d\n",spec->channels);
    printf("\tsilence: %d\n",spec->silence);
    printf("\tsamples: %d\n",spec->samples);
    printf("\tsize: %d\n",spec->size);
    printf("\tcallback: %p\n",spec->callback);
    printf("\tuserdata: %p\n",spec->userdata);
}


/* silence_sound:
 *  Silence the sound.
 */
static void teoSDL_SoundSilence(void)
{
    /* We only do native U8/44100 in the
     * buffer
     * */
    last_data = 128;
}

/* Called by the core
 * @param clock Number of cycles of the emulated CPU since program start
 * (int64 - will loop in roughly 600.000 years at 1Mhz). The emulator adds as
 * much cycles as needed per instruction it executes. i.e an instruction that 
 * takes 3 cycles on the real CPU will add 3 to the current clock. It has no unit
 * it's just a counter. 
 *
 * @param the byte that represent the sound (freq) to emit
 *
 *   */
static void teoSDL_SoundPutByte(unsigned long long int clock, unsigned char data)
{
    /*This code has been taken nearly verbatim from asound.c*/
    int index=(clock%TEO_CYCLES_PER_FRAME)*sound_freq/TEO_CPU_FREQ;
    int n_bytes;

    if (index < last_index)
        index=sound_buffer_size;

    n_bytes = index-last_index;
    if(n_bytes){
        memset (&sound_buffer[last_index], last_data, n_bytes);
    }
    last_index=index;
    last_data = data;
}

/**
 * Fills the buffer with the last put sound byte
 * to close the current frame.
 *
 */
static void teoSDL_SoundFinishFrame(void)
{
    /* Fill the buffer with the last pending byte set by the previous call to put_sound_byte*/
    memset (&sound_buffer[last_index], last_data, sound_buffer_size-last_index);
    last_index=0;
}


static void teoSDL_SoundSync(void)
{
#if 1 
    log_event(SOUND_DOING_SYNC, SDL_GetQueuedAudioSize(dev_id)); 
    while(SDL_GetQueuedAudioSize(dev_id) > 960 * (BUFFER_NFRAMES+1))
        ;
//    printf("%d\n", SDL_GetQueuedAudioSize(dev_id));
#else
    Uint32 qlen = 0;
//    qlen = SDL_GetQueuedAudioSize(dev_id);
    do{
        qlen = SDL_GetQueuedAudioSize(dev_id);
#if ENABLE_HALF_POLL
        SDL_Delay(USEC_TO_MSEC(TEO_MICROSECONDS_PER_FRAME)/2); //Go easy on the CPU
#endif //ENABLE_HALF_POLL
    }while(qlen > 960);
#endif
    log_event(SOUND_DONE_SYNC); 
}

void teoSDL_SoundPlay(void)
{
    SDL_assert(dev_id != 0);

    teoSDL_SoundFinishFrame();

#if ENABLE_SOUND_RECORDER
    teoSDL_SoundRecordFrame();
#endif //ENABLE_SDL2_SOUND_RECORDER
    if(need_buffer){
        log_event(SOUND_BUFFERING, sound_buffer_size); /*qlen is */
        memcpy(frame_holder[frames_ahead], sound_buffer, sound_buffer_size);
        frames_ahead++;
        if(frames_ahead == BUFFER_NFRAMES)
            need_buffer = false;
        return;
    }
    if(frames_ahead){
        for(int i = 0; i < frames_ahead; i++){
            SDL_QueueAudio(dev_id, frame_holder[i], sound_buffer_size);
            log_event(SOUND_PUSHED_BUFFER_SAMPLES, sound_buffer_size, SDL_GetQueuedAudioSize(dev_id)); /*qlen is */
        }
        frames_ahead = 0;
    }
    SDL_QueueAudio(dev_id, sound_buffer, sound_buffer_size);
    teoSDL_SoundSync();
    log_event(SOUND_BEFORE_PUSH, SDL_GetQueuedAudioSize(dev_id)); /*qlen is */
    SDL_QueueAudio(dev_id, sound_buffer, sound_buffer_size);
    log_event(SOUND_PUSHED_SAMPLES, sound_buffer_size, SDL_GetQueuedAudioSize(dev_id)); /*qlen is */
}

void teoSDL_SoundClear(void)
{
    SDL_ClearQueuedAudio(dev_id);
}

void teoSDL_SoundPause(bool flag)
{
    SDL_PauseAudioDevice(dev_id, flag ? 1 : 0);
}

/*Thanks SO!*/
int next_pow2(int v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;

    return v;
}

bool teoSDL_SoundInit(int freq)
{
    if(!SDL_WasInit(SDL_INIT_AUDIO)){
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0){
            Log_Printf(LOG_WARN, "Could not init audio: %s\n", SDL_GetError());
            return false;
        }
    }

    native_spec.freq = freq;
	native_spec.format = AUDIO_U8; 
	native_spec.channels = 1; 
	native_spec.samples = next_pow2(freq/TEO_FRAME_FREQ); 
	native_spec.callback = NULL;
    native_spec.userdata = NULL;

    for(int i = 0; i < SDL_GetNumAudioDrivers(); ++i) {
        printf("Audio driver %d: %s\n", i, SDL_GetAudioDriver(i));
    }

#ifdef PLATFORM_OGXBOX
    int rv;
    rv = SDL_OpenAudio(&native_spec, NULL);
    if(rv < 0){
        debugPrint("Couldn't open audio\n");
        Sleep(4000);
    }
    dev_id = 1; /* The device from SDL_OpenAudio() is always device #1. */
#else
    dev_id = SDL_OpenAudioDevice(NULL, 0, &native_spec, &spec, 0);
#endif // PLATFORM_OGXBOX

    if(!dev_id){
		printf("ERROR: can't open audio. Error is: %s\n",SDL_GetError());
		return false;
	}
    printf("dev_id is: %d\n", dev_id);

    printf("Asked:\n");
    teoSDL_SoundDumpSpec(&native_spec);
    printf("Got:\n");
    teoSDL_SoundDumpSpec(&spec);


    sound_freq = freq;
    /*One video frame / emulator loop worth of audio data, in samples*/
    sound_buffer_size = sound_freq/TEO_FRAME_FREQ;
    sound_buffer = malloc(sizeof(uint8_t)*sound_buffer_size);
    printf("Sound buffer size is %d (bytes)\n",sizeof(uint8_t)*sound_buffer_size);

    for(int i = 0; i < BUFFER_NFRAMES; i++)
        frame_holder[i] = malloc(sizeof(uint8_t)*sound_buffer_size);
    frames_ahead = 0;
    need_buffer = BUFFER_NFRAMES ? true : false;

    teo_PutSoundByte=teoSDL_SoundPutByte;
    teo_SilenceSound=teoSDL_SoundSilence;

    teoSDL_SoundSilence();
    last_index = 0;

    SDL_PauseAudioDevice(dev_id, 0);
#if ENABLE_SOUND_RECORDER
    teoSDL_SoundInitRecorder();
#endif //ENABLE_SOUND_RECORDER
    return true;
}

void teoSDL_SoundShutdown(void)
{
    if(dev_id){
        SDL_PauseAudioDevice(dev_id, 1);
    }
    if(SDL_WasInit(SDL_INIT_AUDIO)){
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }
#if ENABLE_SOUND_RECORDER
    teoSDL_SoundShutdownRecorder();
#endif //ENABLE_SOUND_RECORDER
}
