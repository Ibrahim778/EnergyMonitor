#include <Arduino.h>

#include "timer.h"

Timer::Timer(uint32_t _delay, timercallback _cb):delay(_delay),callback(_cb)
{
    prev = millis();
}

Timer::Timer()
{
    prev = millis();
}

void Timer::setup(uint32_t delay, timercallback cb)
{
    this->delay = delay;
    this->callback = cb;
}

void Timer::loop()
{
    uint32_t currentMillis = millis();

    if (prev > currentMillis) // Overflow has ocurred (will happen once ~ every 55 days)
    {
        prev = millis(); // Reset our prevMillis
        return;
    }

    if(currentMillis - prev > delay)
    {
        callback();
        prev = millis();
    }
}