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

#include "log.h"
#include "logutil.h"

void nowtech::Chunk::push(char const mChar) noexcept {
  mChunk[mIndex++] = mChar;
  if(mIndex == mChunkSize) {
    mOsInterface.push(mChunk, mBlocks);
    mIndex = 1u;
  }
}

extern "C" void logTransmitterThreadFunction(void *argument) {
  static_cast<nowtech::Log*>(argument)->transmitterThreadFunction();
}

constexpr nowtech::LogFormat nowtech::LogConfig::cDefault;
constexpr nowtech::LogFormat nowtech::LogConfig::cNone;
constexpr nowtech::LogFormat nowtech::LogConfig::cB8;
constexpr nowtech::LogFormat nowtech::LogConfig::cB16;
constexpr nowtech::LogFormat nowtech::LogConfig::cB24;
constexpr nowtech::LogFormat nowtech::LogConfig::cB32;
constexpr nowtech::LogFormat nowtech::LogConfig::cD2;
constexpr nowtech::LogFormat nowtech::LogConfig::cD3;
constexpr nowtech::LogFormat nowtech::LogConfig::cD4;
constexpr nowtech::LogFormat nowtech::LogConfig::cD5;
constexpr nowtech::LogFormat nowtech::LogConfig::cD6;
constexpr nowtech::LogFormat nowtech::LogConfig::cD7;
constexpr nowtech::LogFormat nowtech::LogConfig::cD8;
constexpr nowtech::LogFormat nowtech::LogConfig::cX2;
constexpr nowtech::LogFormat nowtech::LogConfig::cX4;
constexpr nowtech::LogFormat nowtech::LogConfig::cX6;
constexpr nowtech::LogFormat nowtech::LogConfig::cX8;

constexpr char nowtech::Log::cDigit2char[16];
constexpr char nowtech::Log::cUnknownApplicationName[8];

nowtech::Log *nowtech::Log::sInstance;

void nowtech::Log::transmitterThreadFunction() noexcept {
  // we assume all the buffers are valid
  CircularBuffer circularBuffer(mOsInterface, mConfig.circularBufferLength, mChunkSize);
  TransmitBuffers transmitBuffers(mOsInterface, mConfig.transmitBufferLength, mChunkSize);
  while(mKeepRunning.load()) {
    // At this point the transmitBuffers must have free space for a chunk
    if(!transmitBuffers.hasActiveTask()) {
      if(circularBuffer.isEmpty()) {
        transmitBuffers << circularBuffer.fetch();
      }
      else { // the circularbuffer may be full or not
        transmitBuffers << circularBuffer.peek();
        circularBuffer.pop();
      }
    }
    else { // There is a task in the transmitBuffers to be continued
      if(circularBuffer.isEmpty()) {
        Chunk const &chunk = circularBuffer.fetch();
        if(chunk.getTaskId() != nowtech::Chunk::cInvalidTaskId) {
          if(transmitBuffers.getActiveTaskId() == chunk.getTaskId()) {
            transmitBuffers << chunk;
          }
          else {
            circularBuffer.keepFetched();
          }
        }
        else { // nothing to do
        }
      }
      else if(!circularBuffer.isFull()) {
        if(circularBuffer.isInspected()) {
          Chunk const &chunk = circularBuffer.fetch();
          if(chunk.getTaskId() != nowtech::Chunk::cInvalidTaskId) {
            if(transmitBuffers.getActiveTaskId() == chunk.getTaskId()) {
              transmitBuffers << chunk;
            }
            else {
              circularBuffer.keepFetched();
            }
          }
          else { // nothing to do
          }
        }
        else {
          Chunk const &chunk = circularBuffer.inspect(transmitBuffers.getActiveTaskId());
          if(!circularBuffer.isInspected()) {
            transmitBuffers << chunk;
            circularBuffer.removeFound();
          }
          else { // nothing to do
          }
        }
      }
      else { // the circular buffer is full
        transmitBuffers << circularBuffer.peek();
        circularBuffer.pop();
        circularBuffer.clearInspected();
      }
    }
    if(transmitBuffers.gotTerminalChunk()) {
      circularBuffer.clearInspected();
    }
    else {
    }
    transmitBuffers.transmitIfNeeded();
  }
}

nowtech::Log::Log(LogOsInterface &aOsInterface, LogConfig const &aConfig) noexcept
  : mOsInterface(aOsInterface)
  , mConfig(aConfig)
  , mChunkSize(aConfig.chunkSize) {
  sInstance = this;
  mOsInterface.createTransmitterThread(this, logTransmitterThreadFunction);
}

void nowtech::Log::doRegisterCurrentTask() noexcept {
  if(mNextTaskId != cIsrTaskId) {
    uint32_t taskHandle = mOsInterface.getCurrentThreadId();
    auto found = mTaskIds.find(taskHandle);
    if(found == mTaskIds.end()) {
      mTaskIds[taskHandle] = mNextTaskId++;
      doSend("-=- Registered task: ", mOsInterface.getThreadName(taskHandle), " -=-");
    }
    else { // nothing to do
    }
  }
}

void nowtech::Log::doRegisterCurrentTask(char const * const aTaskName) noexcept {
  if(mNextTaskId != cIsrTaskId) {
    mOsInterface.registerThreadName(aTaskName);
    uint32_t taskHandle = mOsInterface.getCurrentThreadId();
    auto found = mTaskIds.find(taskHandle);
    if(found == mTaskIds.end()) {
      mTaskIds[taskHandle] = mNextTaskId++;
      doSend("-=- Registered task: ", mOsInterface.getThreadName(taskHandle), " -=-");
    }
    else { // nothing to do
    }
  }
}

nowtech::TaskIdType nowtech::Log::getCurrentTaskId() const noexcept {
  if(InterruptInformation::isInterrupt()) {
    return cIsrTaskId;
  }
  else {
    auto found = mTaskIds.find(mOsInterface.getCurrentThreadId());
    return found == mTaskIds.end() ? Chunk::cInvalidTaskId : found->second;
  }
}

bool nowtech::Log::startSend(Chunk &aChunk) noexcept {
  bool ret = startSendNoHeader();
  if(ret) {
    if(mConfig.taskRepresentation == LogConfig::TaskRepresentation::cId) {
      append(aChunk, *reinterpret_cast<uint8_t*>(aChunk.getData()), mConfig.taskIdFormat.mBase, mConfig.taskIdFormat.mFill);
      append(aChunk, cSeparatorNormal);
    }
    else if(mConfig.taskRepresentation == LogConfig::TaskRepresentation::cName) {
      if(InterruptInformation::isInterrupt()) {
        append(aChunk, cIsrTaskName);
      }
      else {
        append(aChunk, mOsInterface.getCurrentThreadName());
      }
      append(aChunk, cSeparatorNormal);
    }
    else { // nothing to do
    }
    if(mConfig.tickFormat.mBase != 0) {
      append(aChunk, mOsInterface.getLogTime(), static_cast<uint32_t>(mConfig.tickFormat.mBase), mConfig.tickFormat.mFill);
      append(aChunk, cSeparatorNormal);
    }
    else { // nothing to do
    }
  }
  else { // nothing to do
  }
  return ret;
}

bool nowtech::Log::startSend(Chunk &aChunk, LogApp aApp) noexcept {
  auto found = mRegisteredApps.find(aApp);
  if(found != mRegisteredApps.end() && startSend(aChunk)) {
    append(aChunk, found->second);
    append(aChunk, cSeparatorNormal);
    return true;
  }
  else { // nothing to do
  }
  return false;
}

void nowtech::Log::append(Chunk &aChunk, double const aValue, uint8_t const aDigitsNeeded) noexcept {
  if(std::isnan(aValue)) {
    append(aChunk, "nan");
    return;
  } else if(std::isinf(aValue)) {
    append(aChunk, "inf");
    return;
  } else if(aValue == 0.0) {
    aChunk.push('0');
    return;
  }
  else {
    double value = aValue;
    if(value < 0) {
        value = -value;
        aChunk.push('-');
    }
    else if(mConfig.alignSigned) {
      aChunk.push(' ');
    }
    else { // nothing to do
    }
    double mantissa = floor(log10(value));
    double normalized = value / pow(10.0, mantissa);
    int firstDigit;
    for(uint8_t i = 1; i < aDigitsNeeded; i++) {
      firstDigit = static_cast<int>(normalized);
      if(firstDigit > 9) {
        firstDigit = 9;
      }
      else { // nothing to do
      }
      aChunk.push(cDigit2char[firstDigit]);
      normalized = 10.0 * (normalized - firstDigit);
      if(i == 1) {
        aChunk.push('.');
      }
      else { // nothing to do
      }
    }
    firstDigit = static_cast<int>(round(normalized));
    if(firstDigit > 9) {
      firstDigit = 9;
    }
    else { // nothing to do
    }
    aChunk.push(cDigit2char[firstDigit]);
    aChunk.push('e');
    if(mantissa >= 0) {
      aChunk.push('+');
    }
    else { // nothing to do
    }
    append(aChunk, static_cast<int32_t>(mantissa), static_cast<int32_t>(10), 0u);
  }
}
