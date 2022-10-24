//
// Created by rainmeterLotus on 2021/6/22.
//

#include "ff_sdl.h"

std::mutex *SDL_CreateMutex() {
    return new(std::nothrow) std::mutex();
}

std::condition_variable *SDL_CreateCond() {
    return new(std::nothrow) std::condition_variable();
}

std::thread *SDL_CreateReadThread(int (*fn)(void *, void *), void *data, void *context) {
    return new(std::nothrow) std::thread(fn, data, context);
}

std::thread *SDL_CreateThread(void (*fn)(void *, void *), void *data, void *context) {
    return new(std::nothrow) std::thread(fn, data, context);
}

void SDL_CondWait(std::condition_variable *cond, std::mutex *mutex) {
    std::unique_lock<std::mutex> lock(*mutex);
    cond->wait(lock);
}

void SDL_WaitThread(std::thread *thread) {
    if (!thread) return;
    thread->join();
}

char *SDL_getenv(const char *name){
    return nullptr;
}

void SDL_MixAudio(uint8_t*       dst,
                  const uint8_t* src,
                  uint32_t       len,
                  int          volume){
    // do nothing;
}