#include "TestMessage.h"
#include "BWTimer.h"
#include "stdio.h"
#include <string>
#include <vector>

namespace Test::Message {


    namespace {
        std::vector<std::string> unique_interval_messages;
        BWTimer unique_interval_message_timer;
        int _interval = 5;
    }
    
    void init(int interval) {
        unique_interval_message_timer.start(interval, 0);
    }

    void on_frame_update() {
        unique_interval_message_timer.on_frame_update();
        if (unique_interval_message_timer.is_stopped()) {
            unique_interval_message_timer.restart();
            unique_interval_messages.clear();
            printf("\n----------- [New DBGMSG Interval] ---------- \n\n");
        }
    }

    void print_interval_message(const char* msg) {
        bool already_buffered = false;
        for (auto& str : unique_interval_messages) {
            if (strcmp(str.c_str(), msg) == 0) {
                already_buffered = true;
                break;
            }
        }
        if (!already_buffered) {
            unique_interval_messages.push_back(std::string(msg));
            printf("%s\n", msg);
        }
    }

}