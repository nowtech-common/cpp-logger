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

#include <deque>
std::mutex mtx;
std::deque<char> q;

void nowtech::LogStdThreadOstream::FreeRtosQueue::send(char const * const aChunkStart, bool const aBlocks) noexcept {
  bool success;
  do {
/*    char *payload;
    bool success = mFreeList.pop(payload);
    if(success) {
      std::copy(aChunkStart, aChunkStart + mBlockSize, payload);
      mQueue.bounded_push(payload); // this should always succeed here
      mConditionVariable.notify_one();
mtx.lock();
std::cerr << "send: " << payload[0] << payload[1] << payload[2] << payload[3] << payload[4] << payload[5] <<
payload[6] << payload[7] << std::endl;
mtx.unlock();*/
mtx.lock();
//std::cerr << "send: [" << static_cast<uint16_t>(aChunkStart[0]) << ']' << aChunkStart[1] << aChunkStart[2] << aChunkStart[3] << aChunkStart[4] << aChunkStart[5] <<aChunkStart[6] << aChunkStart[7] << std::endl;
for(int i = 0; i < mBlockSize; ++i) {
  q.push_back(aChunkStart[i]);
}
mtx.unlock();
      mConditionVariable.notify_one();
success = true;
/*    }
    else {
      std::this_thread::sleep_for(std::chrono::milliseconds(cEnqueuePollDelay));
    }*/
  } while(aBlocks && !success);
}

bool nowtech::LogStdThreadOstream::FreeRtosQueue::receive(char * const aChunkStart, uint32_t const mPauseLength) noexcept {
  bool result;
  if(q.size() < mBlockSize && mConditionVariable.wait_for(mLock, std::chrono::milliseconds(mPauseLength)) == std::cv_status::timeout) {
    result = false;
  }
  else {
/*    char *payload;
    result = mQueue.pop(payload);
    if(result) {
      std::copy(payload, payload + mBlockSize, aChunkStart);
      mFreeList.bounded_push(payload); // this should always succeed here
mtx.lock();
std::cerr << "recv: " << payload[0] << payload[1] << payload[2] << payload[3] << payload[4] << payload[5] <<
payload[6] << payload[7] << std::endl;
mtx.unlock();*/
mtx.lock();
if(q.size() >= mBlockSize) {
result = true;
for(int i = 0; i < mBlockSize; ++i) {
  aChunkStart[i] = q.front();
  q.pop_front();
}
//std::cerr << "recv: " << q.size() << " [" << static_cast<uint16_t>(aChunkStart[0]) << ']' << aChunkStart[1] << aChunkStart[2] << aChunkStart[3] << aChunkStart[4] << aChunkStart[5] <<aChunkStart[6] << aChunkStart[7] << std::endl;
}
mtx.unlock();
/*    }
    else { // nothing to do
    }*/
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

char const * const nowtech::LogStdThreadOstream::getCurrentThreadName() noexcept {
  char const *result;
  auto found = mTaskNamesIds.find(std::this_thread::get_id());
  if(found != mTaskNamesIds.end()) {
    result = found->second.name.c_str();
  }
  else {
    result = Log::cUnknownApplicationName;
  }
  return result;
}

uint32_t nowtech::LogStdThreadOstream::getCurrentThreadId() noexcept {
  uint32_t result;
  auto found = mTaskNamesIds.find(std::this_thread::get_id());
  if(found != mTaskNamesIds.end()) {
    result = found->second.id;
  }
  else {
    result = cInvalidGivenTaskId;
  }
  return result;
}
