#pragma once

#include <Arduino.h>

class Timer 
{
public:
    Timer();
    Timer(uint32_t delay, timercallback cb);

    void setup(uint32_t delay, timercallback cb);
    void loop();

protected:
    uint32_t prev;
    uint32_t delay;
    timercallback callback;
};