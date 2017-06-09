#include "Timer.hpp"

using namespace std::chrono ;

Timer::Timer(): running(false){
  start_time=steady_clock::now();
  end_time=start_time;
}

void Timer::start(){
  running  = true;
  start_time = steady_clock::now() ;
}

void Timer::stop(){
  end_time = steady_clock::now() ;
  running=false;
}

void Timer::reset(){
  start_time = steady_clock::now() ;
  end_time = start_time;
  running = false;
}

void Timer::_continue(){
  if( running ) return; //nothing to do
  auto dist = end_time - start_time;
  auto now  = steady_clock::now();

  start_time = now - dist;
  running = true;
}

long int Timer::getMicroTime()const{
  //duration<std::chrono::microseconds> time_span ;
  long int time_span;
  if( running ){
    time_span = duration_cast<std::chrono::microseconds>(steady_clock::now() -start_time).count();
  } else {
    time_span = duration_cast<std::chrono::microseconds>(end_time-start_time).count();
  }
  return time_span;
}

double Timer::getTime()const{
  duration<double> time_span ;
  if( running ){
    time_span = duration_cast<duration<double>>( steady_clock::now() - start_time );
  } else {
    time_span = duration_cast<duration<double>>(end_time - start_time);
  }

  return time_span.count() ;
}
