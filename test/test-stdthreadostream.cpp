#include "logstdthreadostream.h"
#include <iostream>
#include <cstdint>
#include <thread>

// clang++ -std=c++17 -Isrc src/log.cpp src/logstdthreadostream.cpp src/logutil.cpp src/nointerrupt.cpp test/test-stdthreadostream.cpp -lpthread -g3 -Og -o test-stdthreadostream

constexpr int32_t threadCount = 1;

char names[10][10] = {
  "thread 0",
  "thread 1",
  "thread 2",
  "thread 3",
  "thread 4",
  "thread 5",
  "thread 6",
  "thread 7",
  "thread 8",
  "thread 9"
};

void delayedLog(int32_t n) {
  Log::registerCurrentTask(names[n]);
  for(int32_t i = 0; i < 10; ++i) {
    Log::send(nowtech::LogApp::cSystem, names[n], ": ", i);
    std::this_thread::sleep_for(std::chrono::milliseconds(8 << i));
  }
}
 
int main() {
  std::thread threads[threadCount];
  
  nowtech::LogConfig logConfig;
  nowtech::LogStdThreadOstream osInterface(std::cout, logConfig);
  nowtech::Log log(osInterface, logConfig);
  Log::registerApp(nowtech::LogApp::cSystem, "system");

  for(int32_t i = 0; i < threadCount; ++i) {
    threads[i] = std::thread(delayedLog, i);
  }
  for(int32_t i = 0; i < threadCount; ++i) {
    threads[i].join();
  }
  return 0;
}

