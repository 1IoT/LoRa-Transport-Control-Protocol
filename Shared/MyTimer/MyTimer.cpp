#include "MyTimer.hpp"

MyTimer::MyTimer()
    : intervalMS(1000), preloadMS(0), running(false), repeat(false)
{
}

MyTimer::MyTimer(uint32_t intervalMS, uint32_t preloadMS, bool running, bool repeat)
    : intervalMS(intervalMS), preloadMS(preloadMS), running(running), repeat(repeat)
{
    timer = preloadMS;
    lastTime = millis();
}

bool MyTimer::checkTimer()
{
    bool ret = false;

    if (running)
    {
        uint32_t now = millis();
        timer += now - lastTime;
        if (timer >= intervalMS)
        {
            ret = true;
            timer -= intervalMS;
            if (!repeat)
            {
                running = false;
            }
        }
        lastTime = now;
    }

    return ret;
}

void MyTimer::startTimer()
{
    running = true;
    timer = preloadMS;
    lastTime = millis();
}

void MyTimer::startTimer(uint32_t intervalMS, uint32_t preloadMS, bool repeat)
{
    this->intervalMS = intervalMS;
    this->preloadMS = preloadMS;
    this->repeat = repeat;

    startTimer();
}

void MyTimer::stopTimer()
{
    running = false;
}

void MyTimer::continueTimer()
{
    running = true;
    lastTime = millis();
}
