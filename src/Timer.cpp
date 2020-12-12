#include "Timer.hpp"

  Timer::Timer(unsigned long intervalMS, unsigned long preloadMS, bool running, bool repeat, fp_call callback) {
    this->intervalMS = intervalMS;
    this->preloadMS = preloadMS;
    this->running = running;
    this->repeat = repeat;
    this->callback = callback;

    timer = preloadMS;
    lastTime = millis();
  }

  void Timer::updateTimer() {
    if(running) {
      unsigned long now = millis();
      timer += now - lastTime;
      if(timer >= intervalMS) {
        callback();
        timer -= intervalMS;
        if(!repeat){
          running = false;
        }
      }
      lastTime = now;
    }
  }

  void Timer::startTimer() {
    running = true;
    timer = preloadMS;
    lastTime = millis();
  }

  void Timer::stopTimer() {
    running = false;
  }
  
  void Timer::pauseTimer() {
    running = false;
  }
  
  void Timer::continueTimer() {
    running = true;
    lastTime = millis();
  }
  
