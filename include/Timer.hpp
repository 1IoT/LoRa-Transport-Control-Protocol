#pragma once
#include <Arduino.h>

class Timer
{

  typedef void (*fp_call)();

public:
  Timer(unsigned long intervalMS, unsigned long preloadMS, bool running, bool repeat, fp_call callback);
  void updateTimer();

  void startTimer();
  void stopTimer();
  void pauseTimer();
  void continueTimer();

private:
  unsigned long intervalMS;
  unsigned long preloadMS;
  bool running;
  bool repeat;
  fp_call callback;

  unsigned long timer;
  unsigned long lastTime;
};