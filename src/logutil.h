#ifndef NOWTECH_LOGUTIL_INCLUDED
#define NOWTECH_LOGUTIL_INCLUDED

#include "log.h"
#include <atomic>

namespace nowtech {

  /// Auxiliary class, not part of the Log API.
  class CircularBuffer final : public BanCopyMove {
  private:
    LogOsInterface &mOsInterface;

    /// Counted in chunks
    LogSizeType const mBufferLength;
    LogSizeType const mChunkSize;
    char * const mBuffer;
    Chunk mStuffStart;
    Chunk mStuffEnd;
    LogSizeType mCount = 0;
    LogSizeType mInspectedCount = 0;
    bool mInspected = true;
    Chunk mFound;

  public:
    CircularBuffer(LogOsInterface &aOsInterface, LogSizeType const aBufferLength, LogSizeType const aChunkSize) noexcept
      : mOsInterface(aOsInterface)
      , mBufferLength(aBufferLength)
      , mChunkSize(aChunkSize)
      , mBuffer(new char[aBufferLength * aChunkSize])
      , mStuffStart(aOsInterface, mBuffer, aBufferLength, Chunk::cInvalidTaskId)
      , mStuffEnd(aOsInterface, mBuffer, aBufferLength, Chunk::cInvalidTaskId)
      , mFound(aOsInterface, mBuffer, aBufferLength, Chunk::cInvalidTaskId) {
    }

    /// Not intended to be destroyed
    ~CircularBuffer() = default;

    bool isEmpty() const noexcept {
      return mCount == 0;
    }

    bool isFull() const noexcept {
      return mCount == mBufferLength;
    }

    bool isInspected() const noexcept {
      return mInspected;
    }

    void clearInspected() noexcept {
      mInspected = false;
      mInspectedCount = 0;
      mFound = mStuffStart.getData();
    }

    Chunk const &fetch() noexcept {
      mStuffEnd.pop();
      return mStuffEnd;
    }

    Chunk const &peek() const noexcept {
      return mStuffStart;
    }

    void pop() noexcept {
      --mCount;
      ++mStuffStart;
      mFound = mStuffStart.getData();
    }

    void keepFetched() noexcept {
      ++mCount;
      ++mStuffEnd;
    }

    ///  searches for aTaskId in mFound and sets mInspected if none found. @return mFound
    Chunk const &inspect(TaskIdType const aTaskId) noexcept;

    /// Invalidates found element, which will be later eliminated
    void removeFound() noexcept {
      mFound.invalidate();
    }
  };

  /// Auxiliary class, not part of the Log API.
  class TransmitBuffers final : public BanCopyMove {
  private:
    LogOsInterface &mOsInterface;

    /// counted in chunks
    LogSizeType const mBufferLength;
    LogSizeType const mChunkSize;
    LogSizeType mBufferToWrite = 0;
    char * mBuffers[2];
    LogSizeType mChunkCount[2] = {
      0,0
    };
    LogSizeType mIndex[2] = {
      0,0
    };
    uint8_t mActiveTaskId = Chunk::cInvalidTaskId;
    bool mWasTerminalChunk = false;
    std::atomic<bool> mTransmitInProgress;
    std::atomic<bool> mRefreshNeeded;

  public:
    TransmitBuffers(LogOsInterface &aOsInterface, LogSizeType const aBufferLength, LogSizeType const aChunkSize) noexcept
      : mOsInterface(aOsInterface)
      , mBufferLength(aBufferLength)
      , mChunkSize(aChunkSize) {
      mBuffers[0] = new char[aBufferLength * (aChunkSize - 1)];
      mBuffers[1] = new char[aBufferLength * (aChunkSize - 1)];
      mTransmitInProgress.store(false);
      mRefreshNeeded.store(false);
      mOsInterface.startRefreshTimer(&mRefreshNeeded);
    }

    bool hasActiveTask() const noexcept {
      return mActiveTaskId != Chunk::cInvalidTaskId;
    }

    TaskIdType getActiveTaskId() const noexcept {
      return mActiveTaskId;
    }

    bool gotTerminalChunk() const noexcept {
      return mWasTerminalChunk;
    }

    /// Assumes that the buffer to write has space for it
    TransmitBuffers &operator<<(Chunk const &aChunk) noexcept;

    void transmitIfNeeded() noexcept;
  };

} // namespace nowtech

#endif // NOWTECH_LOGUTIL_INCLUDED
