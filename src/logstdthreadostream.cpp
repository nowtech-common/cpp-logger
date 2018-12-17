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
      
void nowtech::LogStdThreadOstream::FreeRtosQueue::send(char const * const aChunkStart, bool const aBlocks) noexcept {
  bool success;
  do {
    success = mQueue.try_enqueue_bulk(aChunkStart, mBlockSize);
    if(!success) {
      std::this_thread::sleep_for(std::chrono::milliseconds(cEnqueuePollDelay));
    }
    else {
      mConditionVariable.notify_one();
    }
  } while(aBlocks && !success);
}

bool nowtech::LogStdThreadOstream::FreeRtosQueue::receive(char * const aChunkStart, uint32_t const mPauseLength) noexcept {
  bool result;
  if(mConditionVariable.wait_for(mLock, std::chrono::milliseconds(mPauseLength)) == std::cv_status::timeout) {
    result = false;
  }
  else { // We try to utilize our constant block size and hope the best
    size_t count = mQueue.try_dequeue_bulk(aChunkStart, mBlockSize);
    result = count == mBlockSize;
  }
  return result;
}


void nowtech::LogStdThreadOstream::FreeRtosTimer::run() noexcept {
  while(mKeepRunning.load()) {
    if(mAlarmed) {
      if(mConditionVariable.wait_for(mLock, std::chrono::milliseconds(mTimeout)) == std::cv_status::timeout) {
        mLambda();
        mAlarmed.store(false);
      }  
      else { // nothing to do
      }
    }
    else {
      mConditionVariable.wait(mLock);
    }
  }
}

const char * const nowtech::LogStdThreadOstream::getThreadName(uint32_t const aHandle) noexcept {
  char const * result = "";
  for(auto const &iterator : mTaskNamesIds) {
    if(iterator.second.id == aHandle) {
      result = iterator.second.name.c_str();
    }
    else { // nothing to do
    }
  }
  return result;
}
