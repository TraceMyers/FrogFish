#pragma once
#pragma message("including TestMessage")

#include <string.h>
#include <stdio.h>

#define DBGMSG Test::Message::print_interval_message

namespace Test::Message {

    const int MAX_MESSAGE_LEN = 500;

    void init(int interval);

    void on_frame_update(); 

    void print_interval_message(const char* msg);

    template <typename T>
    void print_interval_message(const char* msg, T item) {
        char buff[MAX_MESSAGE_LEN];
        sprintf(buff, msg, item);
        print_interval_message(buff);
    }

    template <typename T>
    void print_interval_message(const char* msg, T item1, T item2) {
        char buff[MAX_MESSAGE_LEN];
        sprintf(buff, msg, item1, item2);
        print_interval_message(buff);
    }
}