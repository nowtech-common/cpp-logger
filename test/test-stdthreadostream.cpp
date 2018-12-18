/*
 * Copyright 2018 Now Technologies Zrt.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH
 * THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

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

