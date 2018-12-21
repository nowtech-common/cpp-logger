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

#ifndef NOWTECH_LOG_STD_THREAD_OSTREAM_INCLUDED
#define NOWTECH_LOG_STD_THREAD_OSTREAM_INCLUDED

#include "log.h"
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>
#include <string>
#include <string>
#include <ostream>
#include <functional>
#include <condition_variable>
#include <boost/lockfree/queue.hpp>

namespace nowtech {

  /// Class implementing log interface for FreeRTOS and STM HAL as STM32CubeMX
  /// provides them.
  class LogStdThreadOstream final : public LogOsInterface {
    static constexpr uint32_t cInvalidGivenTaskId = 0u;
    static constexpr uint32_t cEnqueuePollDelay = 1u;

    struct NameId {
      std::string name;
      uint32_t    id;

      NameId() : id(0u) {
      }

      NameId(std::string &&aName, uint32_t const aId) {
        name = aName;
        id = aId;
      }
    };

    /// Uses moodycamel::ConcurrentQueue to simulate a FreeRTOS queue.
    class FreeRtosQueue final : public BanCopyMove {
      boost::lockfree::queue<char *> mQueue;
      boost::lockfree::queue<char *> mFreeList;
      std::mutex                     mMutex;
      std::unique_lock<std::mutex>   mLock;
      std::condition_variable        mConditionVariable;

      size_t const mBlockSize;
      char        *mBuffer;
    
    public:
      /// First implementation, we assume we have plenty of memory.
      FreeRtosQueue(size_t const aBlockCount, size_t const aBlockSize) noexcept
        : mQueue(aBlockCount)
        , mFreeList(aBlockCount)
        , mLock(mMutex)
        , mBlockSize(aBlockSize) 
        , mBuffer(new char[aBlockCount * aBlockSize]) {
        char *ptr = mBuffer;
        for(size_t i = 0; i < aBlockCount; ++i) {
          mFreeList.bounded_push(ptr);
          ptr += aBlockSize;
        }
      }

      ~FreeRtosQueue() noexcept {
        delete[] mBuffer;
      }

      void send(char const * const aChunkStart, bool const aBlocks) noexcept;
      bool receive(char * const aChunkStart, uint32_t const aPauseLength) noexcept;
    } mQueue;

    /// Used to force transmission of partially filled buffer in a defined
    /// period of time.
    class FreeRtosTimer final : public BanCopyMove {
      uint32_t                     mTimeout;
      std::function<void()>        mLambda;
      std::mutex                   mMutex;
      std::unique_lock<std::mutex> mLock;
      std::condition_variable      mConditionVariable;
      std::thread                  mThread;
      std::atomic<bool>            mKeepRunning = true;
      std::atomic<bool>            mAlarmed     = false;

    public:
      FreeRtosTimer(uint32_t const aTimeout, std::function<void()> aLambda)
      : mTimeout(aTimeout)
      , mLambda(aLambda) 
      , mLock(mMutex)
      , mThread(&nowtech::LogStdThreadOstream::FreeRtosTimer::run, this) {
      }

      ~FreeRtosTimer() noexcept {
        mKeepRunning.store(false);
        mConditionVariable.notify_one();
        mThread.join();
      }
    
      void run() noexcept;

      void start() noexcept {
        mAlarmed.store(true);
        mConditionVariable.notify_one();
      }
    } mRefreshTimer;

    /// The output stream to use.
    std::ostream &mOutput;

    /// The transmitter task.
    std::thread *mTransmitterThread;

    std::map<std::thread::id, NameId> mTaskNamesIds;

    uint32_t mNextGivenTaskId = cInvalidGivenTaskId + 1u;

    /// True if the other transmission buffer is being transmitted. This is
    /// defined here because OS-specific functionality is here.
    std::atomic<bool> *mProgressFlag;

    /// True if the partially filled buffer should be sent. This is
    /// defined here because OS-specific functionality is here.
    std::atomic<bool> *mRefreshNeeded;

    std::recursive_mutex         mApiMutex;

  public:
    /// Sets parameters and creates the mutex for locking.
    /// The class does not own the stream and only writes to it.
    /// Opening and closing it is user responsibility.
    /// @param aOutput the std::ostream to use
    /// @param aConfig config.
    LogStdThreadOstream(std::ostream &aOutput
      , LogConfig const & aConfig)
      : LogOsInterface(aConfig)
      , mQueue(aConfig.queueLength, mChunkSize)
      , mRefreshTimer(mRefreshPeriod, [this]{this->refreshNeeded();})
      , mOutput(aOutput) {
    }

    virtual ~LogStdThreadOstream() {
      delete mTransmitterThread;
      mOutput.flush();
    }

    /// Registers the given name and an artificial ID in a local map.
    /// This function MUST NOT be called from user code.
    /// void Log::registerCurrentTask(char const * const aTaskName) may call it only.
    /// @param aTaskName Task name to register.
    virtual void registerThreadName(char const * const aTaskName) noexcept {
      NameId nameId { std::string(aTaskName), mNextGivenTaskId };
      ++mNextGivenTaskId;
      mTaskNamesIds.insert(std::pair(std::this_thread::get_id(), nameId));
    }

    /// Returns the task name. This is a dummy and inefficient implementation,
    /// but normally runs only once during registering the current thread.
    /// Note, the returned pointer is valid only as long as this object lives.
    virtual char const * const getThreadName(uint32_t const aHandle) noexcept;

    /// Returns the current task name.
    virtual char const * const getCurrentThreadName() noexcept;

    /// Returns an artificial thread ID for registered threads, cInvalidGivenTaskId otherwise;
    virtual uint32_t getCurrentThreadId() noexcept;

    /// Returns the std::chrono::steady_clock tick count converted into ms and truncated to 32 bits.
    virtual uint32_t getLogTime() const noexcept {
      return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
    }

    /// Creates the transmitter thread using the name logtransmitter.
    /// @param log the Log object to operate on.
    /// @param threadFunc the C function which serves as the task body and
    /// which will call Log.transmitterThread.
    virtual void createTransmitterThread(Log *aLog, void(* aThreadFunc)(void *)) noexcept {
      mTransmitterThread = new std::thread([aLog, aThreadFunc]{aThreadFunc(aLog);});
    }
    
    /// Joins the thread.
    virtual void joinTransmitterThread() noexcept {
      mTransmitterThread->join();
    };

    /// Enqueues the chunks, possibly blocking if the queue is full.
    virtual void push(char const * const aChunkStart, bool const aBlocks) noexcept {
      mQueue.send(aChunkStart, aBlocks);
    }

    /// Removes the oldest chunk from the queue.
    virtual bool pop(char * const aChunkStart) noexcept {
      return mQueue.receive(aChunkStart, mPauseLength);
    }

    /// Pauses execution for the period given in the constructor.
    virtual void pause() noexcept {
      std::this_thread::sleep_for(std::chrono::milliseconds(mPauseLength));
    }

    /// Transmits the data using the serial descriptor given in the constructor.
    /// @param buffer start of data
    /// @param length length of data
    /// @param aProgressFlag address of flag to be set on transmission end.
    virtual void transmit(const char * const aBuffer, LogSizeType const aLength, std::atomic<bool> *aProgressFlag) noexcept {
      mOutput.write(aBuffer, aLength);
      aProgressFlag->store(false);
    }

    /// Starts the timer after which a partially filled buffer should be sent.
    virtual void startRefreshTimer(std::atomic<bool> *aRefreshFlag) noexcept {
      mRefreshNeeded = aRefreshFlag;
      mRefreshTimer.start();
    }

    /// Sets the flag.
    void refreshNeeded() noexcept {
      mRefreshNeeded->store(true);
      mOutput.flush();
    }

    /// Calls az OS-specific lock to acquire a critical section, if implemented
    virtual void lock() noexcept {
      mApiMutex.lock();
    }

    /// Calls az OS-specific lock to release critical section, if implemented
    virtual void unlock() noexcept {
      mApiMutex.unlock();
    }
  };

} //namespace nowtech

#endif // NOWTECH_LOG_FREERTOS_STMHAL_INCLUDED
