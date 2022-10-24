//
// Created by rainmeterLotus on 2021/12/27.
//

#ifndef VIKTOR_FFMPEG_VIKTOR_SDL_H
#define VIKTOR_FFMPEG_VIKTOR_SDL_H
#include <mutex>
#include <thread>

std::mutex *sdl_create_mutex();
std::condition_variable *sdl_create_cond();
void sdl_cond_wait(std::condition_variable *cond,std::mutex *mutex);

void sdl_wait_thread(std::thread *thread);
#endif //VIKTOR_FFMPEG_VIKTOR_SDL_H
