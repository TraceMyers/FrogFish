#include "BWTimer.h"
#include <thread>
#include <chrono>
#include <iostream>

BWTimer::BWTimer(int _fps) :
    fps(_fps),
    frames_left(0),
    store_frames_left(0),
    started(false),
    auto_restart(false),
    callback(nullptr)
{}

void BWTimer::start(int seconds, int frames, bool _auto_restart) {
    started = true;
    frames_left = seconds * fps + frames;
    store_frames_left = frames_left;
    callback = nullptr;
    auto_restart = _auto_restart;
}

void BWTimer::start(void (*func)(), int seconds, int frames, bool _auto_restart) {
    started = true;
    frames_left = seconds * fps + frames;
    store_frames_left = frames_left;
    callback = func;
    auto_restart = _auto_restart;
}

void BWTimer::on_frame_update() {
    if (started) {
        --frames_left;
        if (frames_left <= 0) {
            if (callback != nullptr) {
                callback();
            }
            if (auto_restart) {
                frames_left = store_frames_left;
            }
            else {
                started = false;
            }
        }
    }
}

bool BWTimer::is_stopped() {
    return !started;
}

void BWTimer::restart() {
    frames_left = store_frames_left;
    started = true;
}

int BWTimer::get_frames_left() {
    return frames_left;
}

void BWTimer::sleep_for(int seconds) {
    std::cout << "sleeping for " << seconds << " seconds." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
    std::cout << "Returning to game..." << std::endl;
}