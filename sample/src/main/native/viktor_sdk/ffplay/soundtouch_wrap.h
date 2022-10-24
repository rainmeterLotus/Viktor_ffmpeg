//
// Created by rainmeterLotus on 2021/7/26.
//

#ifndef VIKTOR_FFMPEG_SOUNDTOUCH_WRAP_H
#define VIKTOR_FFMPEG_SOUNDTOUCH_WRAP_H
/**
 * 变声
 * @return
 */
void *soundtouch_create();
int soundtouch_translate(void *handle, short* data, float speed, float pitch, int len, int bytes_per_sample, int n_channel, int n_sampleRate);
void soundtouch_destroy(void *handle);


#endif //VIKTOR_FFMPEG_SOUNDTOUCH_WRAP_H
