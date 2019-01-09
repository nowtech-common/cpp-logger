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

// clang++ -std=c++14 -Isrc src/log.cpp src/logstdthreadostream.cpp src/logutil.cpp test/test-stdthreadostream.cpp -lpthread -g3 -Og -o test-stdthreadostream

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
  Log::send(nowtech::LogApp::cSystem, n, ": ", 0);
  for(int64_t i = 1; i < 13; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1 << i));
    Log::send(nowtech::LogApp::cSystem, n, ". thread delay logarithm: ", LC::cX1, i);
  }
}
 
int main() {
  std::thread threads[threadCount];
  
  nowtech::LogConfig logConfig;
  logConfig.taskRepresentation = nowtech::LogConfig::TaskRepresentation::cName;
  logConfig.refreshPeriod      = 200u;
 // logConfig.allowShiftChainingCalls = false;
  logConfig.allowVariadicTemplatesWork = false;
  nowtech::LogStdThreadOstream osInterface(std::cout, logConfig);
  nowtech::Log log(osInterface, logConfig);
  Log::registerApp(nowtech::LogApp::cSystem, "system");

  uint64_t const uint64 = 123456789012345;
  int64_t const int64 = -123456789012345;

  Log::registerCurrentTask("main");

  Log::send(nowtech::LogApp::cSystem, "uint64: ", uint64, " int64: ", int64);
  Log::sendNoHeader(nowtech::LogApp::cSystem, "uint64: ", uint64, " int64: ", int64);
  Log::send("uint64: ", uint64, " int64: ", int64);
  Log::sendNoHeader("uint64: ", uint64, " int64: ", int64);
  
  Log::i(nowtech::LogApp::cSystem) << "uint64: " << uint64 << " int64: " << int64 << Log::end;
  Log::n(nowtech::LogApp::cSystem) << "uint64: " << uint64 << " int64: " << int64 << Log::end;
  Log::i() << "uint64: " << uint64 << " int64: " << int64 << Log::end;
  Log::n() << "uint64: " << uint64 << " int64: " << int64 << Log::end;

  uint8_t const uint8 = 42;
  int8_t const int8 = -42;

  try {
    Log::i(nowtech::LogApp::cSystem) << uint8 << ' ' << int8 << Log::end;
    Log::i(nowtech::LogApp::cSystem) << LC::cX2 << uint8 << ' ' << LC::cD3 << int8 << Log::end;
    Log::i() << uint8 << ' ' << int8 << Log::end;
    Log::i() << LC::cX2 << uint8 << int8 << Log::end;
    Log::i() << Log::end;
  }
  catch(std::exception &e) {
    Log::i() << "Exception: " << e.what() << Log::end;
  }

  Log::i() << "int8: " << static_cast<int8_t>(123) << Log::end;
  Log::i() << "int16: " << static_cast<int16_t>(123) << Log::end;
  Log::i() << "int32: " << static_cast<int32_t>(123) << Log::end;
  Log::i() << "int64: " << static_cast<int64_t>(123) << Log::end;
  Log::i() << "uint8: " << static_cast<uint8_t>(123) << Log::end;
  Log::i() << "uint16: " << static_cast<uint16_t>(123) << Log::end;
  Log::i() << "uint32: " << static_cast<uint32_t>(123) << Log::end;
  Log::i() << "uint64: " << static_cast<uint64_t>(123) << Log::end;
  Log::i() << "float: " << 1.234567890f << Log::end;
  Log::i() << "double: " << -1.234567890 << Log::end;
  Log::i() << "float: " << -123.4567890f << Log::end;
  Log::i() << "double: " << 123.4567890 << Log::end;
  Log::i() << "float: " << -0.01234567890f << Log::end;
  Log::i() << "double: " << 0.01234567890 << Log::end;
  Log::i() << "bool:" << true << Log::end;
  Log::i() << "bool:" << false << Log::end;

  for(int32_t i = 0; i < threadCount; ++i) {
    threads[i] = std::thread(delayedLog, i);
  }
  for(int32_t i = 0; i < threadCount; ++i) {
    threads[i].join();
  }
  return 0;
}

