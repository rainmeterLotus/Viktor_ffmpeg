//
// Created by rainmeterLotus on 2021/6/22.
//

#ifndef VIKTOR_FFMPEG_FF_SDL_H
#define VIKTOR_FFMPEG_FF_SDL_H
#include <mutex>
#include <thread>

/* Minimum SDL audio buffer size, in samples. */
#define SDL_AUDIO_MIN_BUFFER_SIZE 512
/* Calculate actual buffer size keeping in mind not cause too frequent audio callbacks */
#define SDL_AUDIO_MAX_CALLBACKS_PER_SEC 30

#define SDL_MIX_MAXVOLUME (128)

#define AUDIO_INVALID   0x0000
#define AUDIO_U8        0x0008  /**< Unsigned 8-bit samples */
#define AUDIO_S8        0x8008  /**< Signed 8-bit samples */
#define AUDIO_U16LSB    0x0010  /**< Unsigned 16-bit samples */
#define AUDIO_S16LSB    0x8010  /**< Signed 16-bit samples */
#define AUDIO_U16MSB    0x1010  /**< As above, but big-endian byte order */
#define AUDIO_S16MSB    0x9010  /**< As above, but big-endian byte order */
#define AUDIO_U16       AUDIO_U16LSB
#define AUDIO_S16       AUDIO_S16LSB

#define AUDIO_S32LSB    0x8020  /**< 32-bit integer samples */
#define AUDIO_S32MSB    0x9020  /**< As above, but big-endian byte order */
#define AUDIO_S32       AUDIO_S32LSB

#define AUDIO_F32LSB    0x8120  /**< 32-bit floating point samples */
#define AUDIO_F32MSB    0x9120  /**< As above, but big-endian byte order */
#define AUDIO_F32       AUDIO_F32LSB

#define AUDIO_U16SYS    AUDIO_U16LSB
#define AUDIO_S16SYS    AUDIO_S16LSB
#define AUDIO_S32SYS    AUDIO_S32LSB
#define AUDIO_F32SYS    AUDIO_F32LSB


std::mutex *SDL_CreateMutex();
std::condition_variable *SDL_CreateCond();
std::thread *SDL_CreateReadThread(int (*fn)(void *,void *),void *data,void *context);
std::thread *SDL_CreateThread(void (*fn)(void *, void *), void *data, void *context);
void SDL_CondWait(std::condition_variable *cond,std::mutex *mutex);
void SDL_WaitThread(std::thread *thread);

char *SDL_getenv(const char *name);
void SDL_MixAudio(uint8_t*       dst,
                  const uint8_t* src,
                  uint32_t       len,
                  int          volume);


typedef uint16_t SDL_AudioFormat;

typedef void (*SDL_AudioCallback) (void *userdata, uint8_t * stream, int len);
typedef struct SDL_AudioSpec
{
    int freq;                   /**< DSP frequency -- samples per second */
    SDL_AudioFormat format;     /**< Audio data format */
    uint8_t channels;             /**< Number of channels: 1 mono, 2 stereo */
    uint8_t silence;              /**< Audio buffer silence value (calculated) */
    int16_t samples;             /**< Audio buffer size in samples (power of 2) */
    int16_t padding;             /**< NOT USED. Necessary for some compile environments */
    uint32_t size;                /**< Audio buffer size in bytes (calculated) */
    SDL_AudioCallback callback;
    void *userdata;
} SDL_AudioSpec;

#endif //VIKTOR_FFMPEG_FF_SDL_H
