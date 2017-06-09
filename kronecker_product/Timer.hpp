#ifndef  Timer_hpp
#define  Timer_hpp

#include <chrono>

struct Timer{
  std::chrono::steady_clock::time_point start_time;
  std::chrono::steady_clock::time_point end_time;
  bool running;

  //create a timer. The created timer is not running
  Timer();

  //start the timer.
  void start();

  //stop the timer
  void stop();

  //reset the timer
  void reset();

  //continue a stoped timer without wiping the current time
  void _continue();

  //return time in microseconds
  long int getMicroTime() const;

  //return time in seconds
  double getTime() const;
};

#endif
