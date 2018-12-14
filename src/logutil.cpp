#include "logutil.h"

nowtech::Chunk const &nowtech::CircularBuffer::inspect(TaskIdType const aTaskId) noexcept {
  while(mInspectedCount < mCount && mFound.getTaskId() != aTaskId) {
    ++mInspectedCount;
    ++mFound;
  }
  if(mInspectedCount == mCount) {
    nowtech::Chunk source(mOsInterface, mBuffer, mBufferLength);
    nowtech::Chunk destination(mOsInterface, mBuffer, mBufferLength);
    source = mStuffStart.getData();
    destination = mStuffStart.getData();
    while(source.getData() != mStuffEnd.getData()) {
      if(destination.getTaskId() != nowtech::Chunk::cInvalidTaskId) {
        if(source.getData() == destination.getData()) {
          ++source;
        }
        else { // nothing to do
        }
        ++destination;
      }
      else {
        if(source.getTaskId() == nowtech::Chunk::cInvalidTaskId) {
          ++source;
        }
        else {
          destination = source;
          source.invalidate();
        }
      }
    }
    char *startRemoved = destination.getData();
    char *endRemoved = mStuffEnd.getData();
    if(startRemoved > endRemoved) {
      endRemoved += mChunkSize * mBufferLength;
    }
    else { // nothing to do
    }
    mCount -= (endRemoved - startRemoved) / mChunkSize;
    mStuffEnd = destination.getData();
    mInspected = true;
  }
  return mFound;
}

nowtech::TransmitBuffers &nowtech::TransmitBuffers::operator<<(nowtech::Chunk const &aChunk) noexcept {
  if(aChunk.getTaskId() != nowtech::Chunk::cInvalidTaskId) {
    LogSizeType i = 1;
    char const * const origin = aChunk.getData();
    mWasTerminalChunk = false;
    char * buffer = mBuffers[mBufferToWrite];
    LogSizeType &index = mIndex[mBufferToWrite];
    while(!mWasTerminalChunk && i < mChunkSize) {
      buffer[index] = origin[i];
      mWasTerminalChunk = origin[i] == '\n';
      ++i;
      ++index;
    }
    ++mChunkCount[mBufferToWrite];
    if(mWasTerminalChunk) {
      mActiveTaskId = nowtech::Chunk::cInvalidTaskId;
    }
    else {
      mActiveTaskId = aChunk.getTaskId();
    }
  }
  return *this;
}

void nowtech::TransmitBuffers::transmitIfNeeded() noexcept {
  if(mChunkCount[mBufferToWrite] == 0) {
      return;
  }
  else {
    if(mChunkCount[mBufferToWrite] == mBufferLength) {
      while(mTransmitInProgress.load() == true) {
        mOsInterface.pause();
      }
      mRefreshNeeded.store(true);
    }
    else { // nothing to do
    }
    if(mTransmitInProgress.load() == false && mRefreshNeeded.load() == true) {

      mTransmitInProgress.store(true);
      mOsInterface.transmit(mBuffers[mBufferToWrite], mIndex[mBufferToWrite], &mTransmitInProgress);
      mBufferToWrite = 1 - mBufferToWrite;
      mIndex[mBufferToWrite] = 0;
      mChunkCount[mBufferToWrite] = 0;
      mRefreshNeeded.store(false);
      mOsInterface.startRefreshTimer(&mRefreshNeeded);
    }
    else { // nothing to do
    }
  }
}
