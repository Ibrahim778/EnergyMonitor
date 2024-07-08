#pragma once

#define BLYNK_PRINT Serial
#include "secrets.h"
#include <BlynkSimpleEsp8266.h>

inline BlynkWifi::BlynkState GetBlynkState()
{
    return *(BlynkWifi::BlynkState *)((char *)&Blynk + 0x30);
}

inline void SetBlynkState(BlynkWifi::BlynkState state)
{
    *(BlynkWifi::BlynkState *)((char *)&Blynk + 0x30) = state;
}