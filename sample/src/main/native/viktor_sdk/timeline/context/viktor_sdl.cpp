//
// Created by rainmeterLotus on 2021/12/27.
//

#include "viktor_sdl.h"



std::mutex *sdl_create_mutex(){
    return new(std::nothrow) std::mutex();
}
std::condition_variable *sdl_create_cond(){
    return new(std::nothrow) std::condition_variable();
}

void sdl_cond_wait(std::condition_variable *cond,std::mutex *mutex){
    std::unique_lock<std::mutex> lock(*mutex);
    cond->wait(lock);
}

void sdl_wait_thread(std::thread *thread) {
    if (!thread) return;
    thread->join();
}