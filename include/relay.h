#pragma once

class Relay {
public:
    Relay(int pin);
    
    void enable();
    void disable();

    void toggle();

    bool getStatus() {
        return status;
    }

protected:
    bool status;
    int pin;
};