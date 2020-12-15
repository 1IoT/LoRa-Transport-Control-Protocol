#pragma once
#include <Arduino.h>

class MyTimer
{

public:
    MyTimer();
    MyTimer(uint32_t intervalMS, uint32_t preloadMS, bool running, bool repeat);
    bool checkTimer();

    void startTimer();
    void startTimer(uint32_t intervalMS, uint32_t preloadMS, bool repeat);
    void stopTimer();
    void continueTimer();

private:
    uint32_t intervalMS;
    uint32_t preloadMS;
    bool running;
    bool repeat;

    uint32_t timer;
    uint32_t lastTime;
};