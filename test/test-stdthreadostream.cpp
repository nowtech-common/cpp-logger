#include "logstdthreadostream.h"
#include <iostream>
#include <cstdint>
#include <thread>

// clang++ -std=c++17 -Isrc src/log.cpp src/logstdthreadostream.cpp src/logutil.cpp src/nointerrupt.cpp test/test-stdthreadostream.cpp -lpthread -g3 -Og -o test-stdthreadostream

constexpr int32_t threadCount = 10;

char names[10][10] = {
  "thread_0",
  "thread_1",
  "thread_2",
  "thread_3",
  "thread_4",
  "thread_5",
  "thread_6",
  "thread_7",
  "thread_8",
  "thread_9"
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
  logConfig.taskRepresentation = nowtech::LogConfig::TaskRepresentation::cName;
  logConfig.refreshPeriod      = 200u;
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

