//
// Created by rainmeterLotus on 2021/7/26.
//

#include "soundtouch_wrap.h"
#include "../../sound_touch/include/SoundTouch.h"
#include "../timeline//util/ViktorLog.h"

using namespace soundtouch;

void* soundtouch_create(){
    auto *handle_ptr = new SoundTouch();
    return handle_ptr;
}

int soundtouch_translate(void *handle, short* data, float speed, float pitch, int len, int bytes_per_sample, int n_channel, int n_sampleRate){
    VIKTOR_LOGI("soundtouch_translate speed:%f,pitch:%f,len:%d,bytes_per_sample:%d,n_channel:%d,n_sampleRate:%d",
         speed,pitch,len,bytes_per_sample,n_channel,n_sampleRate);
    auto *handle_ptr = static_cast<SoundTouch *>(handle);
    if (handle_ptr == nullptr){
        return 0;
    }

    int put_n_sample = len / n_channel;
    int nb = 0;
    int pcm_data_size = 0;
    VIKTOR_LOGI("soundtouch_translate put_n_sample:%d",put_n_sample);
    handle_ptr->setPitch(pitch);
    handle_ptr->setRate(speed);

    handle_ptr->setSampleRate(n_sampleRate);
    handle_ptr->setChannels(n_channel);

    handle_ptr->putSamples((SAMPLETYPE*)data, put_n_sample);

    do {
        nb = handle_ptr->receiveSamples((SAMPLETYPE*)data,n_sampleRate/n_channel);
        VIKTOR_LOGI("soundtouch_translate do while nb:%d",nb);
        pcm_data_size += nb * n_channel * bytes_per_sample;
    }while (nb != 0);
    VIKTOR_LOGI("soundtouch_translate end pcm_data_size:%d",pcm_data_size);
    return pcm_data_size;
}

void soundtouch_destroy(void *handle){
    auto *handle_ptr = static_cast<SoundTouch *>(handle);
    if (handle_ptr == nullptr){
        return;
    }
    handle_ptr->clear();
    delete handle_ptr;
}