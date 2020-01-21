#include <SDL2/SDL.h>

#include "teo.h"
#include "defs.h" //MIN/MAX
#include "sdl2/teo-sdl-sound.h"
#include "sdl2/teo-sdl-log.h"


/*Back buffer where we store data 
 * incoming from the virtual TO8
 * */
static int sound_freq;
static int sound_buffer_size;
static unsigned char *sound_buffer;
static unsigned char *mix_buffer;
static int last_index;
static unsigned char last_data;

static SDL_AudioSpec spec;
static SDL_AudioDeviceID dev_id = 0;

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
    last_data = spec.silence;
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

//    printf("%ld-%d:0x%x\n",clock,index,data);  
    if (index < last_index)
        index=sound_buffer_size;

    n_bytes = index-last_index;
    if(n_bytes){
        memset (&sound_buffer[last_index], last_data, n_bytes);
//         printf("Put Wrote %d bytes of 0x%x\n", n_bytes, last_data);
    }

//    for(int i=last_index; i<index; i++)
//        sound_buffer[i]=last_data;



    last_index=index;
    last_data=data;
}

void teoSDL_SoundPlay(void)
{
    char *buffer_ptr;
    int rv;
    Uint8 *dst;

    if(spec.callback) return;
//    if(!dev_id || SDL_GetAudioDeviceStatus(dev_id) != SDL_AUDIO_PLAYING) return;
    if(!dev_id) return;

    if(!last_index) return;

    int n_bytes;
    n_bytes = sound_buffer_size-last_index;
    /* Fill the buffer with the last pending byte set by the previous call to put_sound_byte*/
//   for (int i=last_index; i<sound_buffer_size; i++)
//        sound_buffer[i]=last_data;
 //   if(last_data = 0xfc)
//        last_data = 0x80;
    memset (&sound_buffer[last_index], last_data, sound_buffer_size-last_index);
//    printf("Filler wrote %d bytes of 0x%x\n", n_bytes,last_data);
//    printf("last_index was %d\n",last_index);

    last_index=0;
//    last_data=0x80;
//    memset(mix_buffer, spec.silence, sound_buffer_size);
//    SDL_MixAudioFormat(mix_buffer, sound_buffer, AUDIO_U8, sound_buffer_size, SDL_MIX_MAXVOLUME*0.75);
//    SDL_QueueAudio(dev_id, mix_buffer, sound_buffer_size);

    SDL_QueueAudio(dev_id, sound_buffer, sound_buffer_size);

//    last_data = 0x80;
}

void teoSDL_SoundAudioCallback(void *udata, Uint8 *stream, int len)
{
#if 0
    int n_bytes;

    if(len < sound_buffer_size){
        printf("len too small !\n");
        exit(-1);
    }

    n_bytes = index-last_index;
    if(n_bytes > 0)
        printf("Got %d bytes !\n", n_bytes);

    if(n_bytes < sound_buffer_size)
        return;
    printf("Got enough bytes !\n");

    SDL_memset(stream, spec.silence, len);

    SDL_MixAudio(stream, sound_buffer, sound_buffer_size, SDL_MIX_MAXVOLUME*0.8);
#endif
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
    SDL_AudioSpec wanted_spec;

    if(!SDL_WasInit(SDL_INIT_AUDIO)){
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0){
            Log_Printf(LOG_WARN, "Could not init audio: %s\n", SDL_GetError());
            return false;
        }
    }

    wanted_spec.freq = freq; 
	wanted_spec.format = AUDIO_U8; 
	wanted_spec.channels = 1; 
	wanted_spec.samples = next_pow2(freq/TEO_FRAME_FREQ); 
	wanted_spec.callback = NULL;
//	wanted_spec.callback = teoSDL_SoundAudioCallback;
    wanted_spec.userdata = NULL;

    int i;

    for (i = 0; i < SDL_GetNumAudioDrivers(); ++i) {
        printf("Audio driver %d: %s\n", i, SDL_GetAudioDriver(i));
    }



    dev_id = SDL_OpenAudioDevice(NULL, 0, &wanted_spec, &spec, 0);
//    dev_id = SDL_OpenAudioDevice(NULL, 0, &wanted_spec, &spec, SDL_AUDIO_ALLOW_ANY_CHANGE);

    if(!dev_id){
		printf("ERROR: can't open audio. Error is: %s\n",SDL_GetError());
		return false;
	}
    printf("dev_id is: %d\n", dev_id);

    printf("Asked:\n");
    teoSDL_SoundDumpSpec(&wanted_spec);
    printf("Got:\n");
    teoSDL_SoundDumpSpec(&spec);

    sound_freq = freq;
    sound_buffer_size = sound_freq/TEO_FRAME_FREQ;
    sound_buffer = malloc(sizeof(unsigned char)*sound_buffer_size);
    mix_buffer = malloc(sizeof(unsigned char)*sound_buffer_size);

    teo_PutSoundByte=teoSDL_SoundPutByte;
    teo_SilenceSound=teoSDL_SoundSilence;

    teoSDL_SoundSilence();
    last_index = 0;

    SDL_PauseAudioDevice(dev_id, 0);
    return true;
}
