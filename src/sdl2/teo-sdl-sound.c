#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include <SDL.h>

#include "teo.h"
#include "defs.h" //MIN/MAX
#include "sdl2/teo-sdl-sound.h"
#include "logsys.h"

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

#define BUFFER_NFRAMES 5
static unsigned char *frame_holder[BUFFER_NFRAMES];
int frames_ahead = 0;
bool need_buffer = true;

#define TEO_AUDIO_FRAME_SIZE 960 //In bytes
SDL_AudioSpec native_spec;
static SDL_AudioSpec spec;
static SDL_AudioDeviceID dev_id = 0;

extern bool sfront_frame_drop;
#define FRAMEDROP_SPEED_START -8.0
#define FRAMEDROP_SPEED_STOP -3.0


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
            log_msgf(LOG_ERROR,"Couldn't open sound dump file !\n");
        else
            log_msgf(LOG_INFO,"Successfuly opened sound dump file !\n");
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
    log_msgf(LOG_TRACE,"Spec %p:\n", spec);
    log_msgf(LOG_TRACE,"\tfreq: %d\n",spec->freq);
    log_msgf(LOG_TRACE,"\tformat: %d\n",spec->format);
    log_msgf(LOG_TRACE,"\tchannels: %d\n",spec->channels);
    log_msgf(LOG_TRACE,"\tsilence: %d\n",spec->silence);
    log_msgf(LOG_TRACE,"\tsamples: %d\n",spec->samples);
    log_msgf(LOG_TRACE,"\tsize: %d\n",spec->size);
    log_msgf(LOG_TRACE,"\tcallback: %p\n",spec->callback);
    log_msgf(LOG_TRACE,"\tuserdata: %p\n",spec->userdata);
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

static inline bool isThereEnoughRoomForNextFrame(void)
{
    Uint32 qlen;

    qlen = SDL_GetQueuedAudioSize(dev_id);
    if(qlen <= (BUFFER_NFRAMES-1)*TEO_AUDIO_FRAME_SIZE){
        return true;
    }
    return false;
}


static void teoSDL_SoundSync(void)
{
    /* Wait until the buffer has "room" (i.e that one frame worth of audio has been played)
     * for one frame*/
#if ENABLE_FRAMEDROP
    Uint32 qlen, ticks;
    static Uint32 last_qlen = 0;
    static Uint32 last_ticks = 0;
    ticks = SDL_GetTicks();
    qlen = SDL_GetQueuedAudioSize(dev_id);
    if(last_qlen && last_ticks){ /*One would be enough*/
        double dq = ((double)qlen)-last_qlen;
        double speed = dq/(ticks - last_ticks);
        if(speed < FRAMEDROP_SPEED_START)
            sfront_frame_drop = true;
        if(speed > FRAMEDROP_SPEED_STOP)
            sfront_frame_drop = false;
        log_event(SOUND_QLEN_SPEED, speed);
    }
    last_qlen = qlen;
    last_ticks = ticks;
#endif //ENABLE_FRAMEDROP
    log_event(SOUND_DOING_SYNC, SDL_GetQueuedAudioSize(dev_id)); 
    while(isThereEnoughRoomForNextFrame() == false)
        ;
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
        SDL_PauseAudioDevice(dev_id, 1);
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
        SDL_PauseAudioDevice(dev_id, 0);
    }
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
            log_msgf(LOG_WARNING, "Could not init audio: %s\n", SDL_GetError());
            return false;
        }
    }

    native_spec.freq = freq;
	native_spec.format = AUDIO_U8; 
	native_spec.channels = 1; 
	native_spec.samples = next_pow2(freq/TEO_FRAME_FREQ); 
	native_spec.callback = NULL;
    native_spec.userdata = NULL;

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
		log_msgf(LOG_ERROR,"ERROR: can't open audio. Error is: %s\n",SDL_GetError());
		return false;
	}
    log_msgf(LOG_TRACE,"dev_id is: %d\n", dev_id);

    log_msgf(LOG_TRACE,"Asked:\n");
    teoSDL_SoundDumpSpec(&native_spec);
    log_msgf(LOG_TRACE,"Got:\n");
    teoSDL_SoundDumpSpec(&spec);


    sound_freq = freq;
    /*One video frame / emulator loop worth of audio data, in samples*/
    sound_buffer_size = sound_freq/TEO_FRAME_FREQ;
    sound_buffer = malloc(sizeof(uint8_t)*sound_buffer_size);
    log_msgf(LOG_DEBUG,"Sound buffer size is %d (bytes)\n",sizeof(uint8_t)*sound_buffer_size);

    for(int i = 0; i < BUFFER_NFRAMES; i++)
        frame_holder[i] = malloc(sizeof(uint8_t)*sound_buffer_size);
    frames_ahead = 0;
    need_buffer = BUFFER_NFRAMES ? true : false;

    teo_PutSoundByte=teoSDL_SoundPutByte;
    teo_SilenceSound=teoSDL_SoundSilence;

    teoSDL_SoundSilence();
    last_index = 0;

    SDL_PauseAudioDevice(dev_id, 1);
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
