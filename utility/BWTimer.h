#pragma once
#pragma message("including BWTimer")

class BWTimer {

private:

    int fps;
    int frames_left;
    int store_frames_left;
    bool started;
    bool auto_restart;
    void (*callback)();

public:

    BWTimer(int _fps=24);

    void start(int seconds, int frames, bool _auto_restart=false);
    void start(void (*func)(), int seconds, int frames, bool _auto_restart=false);
    // call once per frame
    void on_frame_update();
    bool is_stopped();
    void restart();
    int get_frames_left();
    void sleep_for(int seconds);
};
