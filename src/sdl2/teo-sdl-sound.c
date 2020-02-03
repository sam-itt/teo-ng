#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include <SDL.h>

#include "teo.h"
#include "defs.h" //MIN/MAX
#include "sdl2/teo-sdl-sound.h"
#include "sdl2/teo-sdl-log.h"

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

#define teoSDL_SoundSpecEquals(a,b) (((a).freq == (b).freq) && ((a).format == (b).format) && ((a).channels == (b).channels))

#ifdef PLATFORM_OGXBOX
#define printf debugPrint
#endif

/*Back buffer where we store data 
 * incoming from the virtual TO8
 * */
static int sound_freq;
static int sound_buffer_size;
static unsigned char *sound_buffer;
static unsigned char *sound_convert_buffer;
static unsigned char last_data;
static int last_index;


SDL_AudioSpec native_spec;
static SDL_AudioSpec spec;
static SDL_AudioDeviceID dev_id = 0;
static SDL_AudioStream *sdl_stream = NULL;

#define ENABLE_SOUND_RECORDER 0
#if ENABLE_SOUND_RECORDER
#if PLATFORM_OGXBOX
HANDLE output = INVALID_HANDLE_VALUE;
#else
FILE *output = NULL;
#endif //PLATFORM_OGXBOX
static bool dump_pcm = false;
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
        if(sdl_stream){ //If sdl_stream exists, it means we need to do some resempling
            int rc = SDL_AudioStreamPut(sdl_stream, &sound_buffer[last_index], (n_bytes) * sizeof (uint8_t));
            if (rc == -1) {
                printf("Uhoh, failed to put samples in stream: %s\n", SDL_GetError());
            }
        }
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
    if(sdl_stream){ //If sdl_stream exists, it means we need to do some resempling
        int rc = SDL_AudioStreamPut(sdl_stream, &sound_buffer[last_index], (sound_buffer_size-last_index) * sizeof (uint8_t));
        if (rc == -1) {
            printf("Uhoh, failed to put samples in stream: %s\n", SDL_GetError());
        }
    }
    last_index=0;
}


void teoSDL_SoundPlay(void)
{
    SDL_assert(dev_id != 0);

    void *play_buffer;
    size_t play_buffer_size;

    teoSDL_SoundFinishFrame();

    play_buffer = sound_buffer;
    play_buffer_size = sound_buffer_size;

    if(sdl_stream){
        int available;
        available = SDL_AudioStreamAvailable(sdl_stream);
        int gotten = SDL_AudioStreamGet(sdl_stream, sound_convert_buffer, sizeof(uint8_t)*available);
        if (gotten == -1) {
            printf("Uhoh, failed to get converted data: %s\n", SDL_GetError());
        }else{
            play_buffer = sound_convert_buffer;
            play_buffer_size = sizeof(uint8_t)*available;
        }
     }
#if ENABLE_SOUND_RECORDER
    if(dump_pcm){
#ifdef PLATFORM_OGXBOX
        if(output != INVALID_HANDLE_VALUE){
            DWORD written;
            WriteFile(output, play_buffer, play_buffer_size, &written, NULL);
        }
#else
        if(output){
            fwrite(play_buffer, sizeof(uint8_t), play_buffer_size, output);
        }
#endif
    }
#endif //ENABLE_SDL2_SOUND_RECORDER
    SDL_QueueAudio(dev_id, play_buffer, play_buffer_size);
}

void teoSDL_SoundClear(void)
{
    SDL_ClearQueuedAudio(dev_id);
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
    spec.freq = 48000; 
	spec.format = AUDIO_S16; 
	spec.channels = 2; 
	spec.samples = 4096; 
	spec.callback = NULL;
    spec.userdata = NULL;

    int rv;
    rv = SDL_OpenAudio(&spec, NULL);
    if(rv < 0){
        debugPrint("Couldn't open audio\n");
        Sleep(4000);
    }
    dev_id = 1; /* The device from SDL_OpenAudio() is always device #1. */
#else
    dev_id = SDL_OpenAudioDevice(NULL, 0, &native_spec, &spec, 0);
#endif

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
    sound_convert_buffer = malloc(sizeof(uint8_t)*(sound_buffer_size*10));
    printf("Sound buffer size is %d (bytes)\n",sizeof(uint8_t)*sound_buffer_size);

    if(!teoSDL_SoundSpecEquals(native_spec,spec)){
        printf("Will convert from sound stream !\n");
        sdl_stream = SDL_NewAudioStream(
            native_spec.format, native_spec.channels, native_spec.freq, 
            spec.format, spec.channels,spec.freq
        );
    }

    teo_PutSoundByte=teoSDL_SoundPutByte;
    teo_SilenceSound=teoSDL_SoundSilence;

    teoSDL_SoundSilence();
    last_index = 0;

    SDL_PauseAudioDevice(dev_id, 0);
#if ENABLE_SOUND_RECORDER
    if(dump_pcm){
#ifdef PLATFORM_OGXBOX
        output = CreateFileA("D:\\xbox-dump.raw", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_FLAG_RANDOM_ACCESS, NULL);
        if(output == INVALID_HANDLE_VALUE)
            debugPrint("Couldn't open sound dump file !\n");
        else
            debugPrint("Successfuly opened sound dump file !\n");
#else
        output = fopen("pc-dump.raw","wb");
        if(!output)
            printf("Couldn't open sound dump file !\n");
        else
            printf("Successfuly opened sound dump file !\n");
#endif
    }
#endif //ENABLE_SOUND_RECORDER
    return true;
}

void teoSDL_SoundShutdown(void)
{
    if(sdl_stream)
     SDL_FreeAudioStream(sdl_stream);
    if(dev_id){
        SDL_PauseAudioDevice(dev_id, 1);
    }
    if(SDL_WasInit(SDL_INIT_AUDIO)){
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }
#if ENABLE_SOUND_RECORDER
#ifdef PLATFORM_OGXBOX
    if(output != INVALID_HANDLE_VALUE)
        CloseHandle(output);
#endif
#endif //ENABLE_SOUND_RECORDER
}
