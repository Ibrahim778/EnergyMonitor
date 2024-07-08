// This is a wrapper class to control low-level logic 5v relays with the 3.3v logic levels of the ESP8266

#include <Arduino.h>

#include "relay.h"

Relay::Relay(int _pin):pin(_pin) 
{
    disable();
}

void Relay::disable() 
{
    pinMode(pin, INPUT); // This disconnects our pin and resets our relay, turning it off
    status = false;
}

void Relay::enable()
{
    pinMode(pin, OUTPUT); // Setup our pin for output
    pinMode(pin, HIGH); // You can set it to high or low here, the 3.3v is not enough to turn it off on it's own anyways
    status = true;
}

void Relay::toggle()
{
    if(status)
        disable();
    else 
        enable();
}
