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

#ifndef NOWTECH_LOG_INCLUDED
#define NOWTECH_LOG_INCLUDED

#include "BanCopyMove.h"
#include <cstdint>
#include <type_traits>
#include <atomic>
#include <limits>
#include <cmath>
#include <map>

#include <iostream>

namespace nowtech {

  /// Type for all logging-related sizes
  typedef uint32_t LogSizeType;

  /// Type for task ID in map.
  typedef uint8_t TaskIdType;

  /// Struct holding numeric system and zero fill information.
  struct LogFormat {
    /// Base of the numeric system. Can be 2, 10, 16.
    uint8_t base;

    /// Number of digits to emit with zero fill, or 0 if no fill.
    uint8_t fill;

    /// Constructor.
    constexpr LogFormat()
    : base(0u)
    , fill(0u) {
    }
    
    /// Constructor.
    constexpr LogFormat(uint8_t const aBase, uint8_t const aFill)
    : base(aBase)
    , fill(aFill) {
    }

    LogFormat(LogFormat const &) = default;
    LogFormat(LogFormat &&) = default;
    LogFormat& operator=(LogFormat const &) = default;
    LogFormat& operator=(LogFormat &&) = default;

    bool isValid() const noexcept {
      return base == 2u || base == 10u || base == 16u;
    }
  };

  /// Type of application subsystem to log. These needs to be registered
  /// if the log is invoked like Log::send(nowtech::LogApp::cSystem, "stuff to log")
  /// If registration is omitted, the above call will do nothing.
  /// Regural calls like Log::send("stuff to log") will always have effect.
  enum class LogApp : uint8_t {
    cSystem,
    cWatchdog
  };

  /// Configuration struct with default values for general usage.
  struct LogConfig final : public BanCopyMove {
  public:
    /// Type of info to log about the sender task
    enum class TaskRepresentation : uint8_t {cNone, cId, cName};

    /// This is the default logging format and the only one I will document
    /// here. For the others, the letter represents the base of the number
    /// system and the number represents the minimum digits to write, possibly
    /// with leading zeros. When formats are applied to floating point
    /// numbers, the numeric system info is discarded.
    static constexpr LogFormat cDefault = LogFormat(10, 0);
    static constexpr LogFormat cNone = LogFormat(0, 0);
    static constexpr LogFormat cB4 = LogFormat(2, 4);
    static constexpr LogFormat cB8 = LogFormat(2, 8);
    static constexpr LogFormat cB12 = LogFormat(2, 12);
    static constexpr LogFormat cB16 = LogFormat(2, 16);
    static constexpr LogFormat cB24 = LogFormat(2, 24);
    static constexpr LogFormat cB32 = LogFormat(2, 32);
    static constexpr LogFormat cD1 = LogFormat(10, 1);
    static constexpr LogFormat cD2 = LogFormat(10, 2);
    static constexpr LogFormat cD3 = LogFormat(10, 3);
    static constexpr LogFormat cD4 = LogFormat(10, 4);
    static constexpr LogFormat cD5 = LogFormat(10, 5);
    static constexpr LogFormat cD6 = LogFormat(10, 6);
    static constexpr LogFormat cD7 = LogFormat(10, 7);
    static constexpr LogFormat cD8 = LogFormat(10, 8);
    static constexpr LogFormat cX1 = LogFormat(16, 1);
    static constexpr LogFormat cX2 = LogFormat(16, 2);
    static constexpr LogFormat cX3 = LogFormat(16, 3);
    static constexpr LogFormat cX4 = LogFormat(16, 4);
    static constexpr LogFormat cX6 = LogFormat(16, 6);
    static constexpr LogFormat cX8 = LogFormat(16, 8);

    /// If true, use of Log << something << to << log << Log::end; calls will
    /// be allowed from registered threads (but NOT from ISR).
    /// This requires pre-allocating 256 * chunkSize bytes of memory, but lets
    /// reduce the stack sizes dramatically.
    bool allowShiftChainingCalls = true;

    /// If true, logging will work from ISR.
    bool logFromIsr = false;

    /// Total message chunk size to use in queue and buffers. The net capacity is
    /// one less, because the task ID takes a character. Messages are not handled
    /// in characters os as a string, but as chunks. \n signs the end of a message.
    LogSizeType chunkSize = 8u;

    /// Length of a FreeRTOS queue in chunks.
    LogSizeType queueLength = 64u;

    /// Length of the circular buffer used for message sorting, measured also in
    /// chunks.
    LogSizeType circularBufferLength = 64u;

    /// Length of a buffer in the transmission double-buffer pair, in chunks.
    LogSizeType transmitBufferLength = 32u;

    /// Length of stack-reserved buffer for number to string conversion. Can be
    /// reduced if no binary output is used.
    LogSizeType appendStackBufferLength = 34u;

    /// Length of a pause in ms during waiting for transmission of the other
    /// buffer or timeout while reading from the FreeRTOS queue.
    uint32_t pauseLength = 100u;

    /// Length of the period used to wait for messages before transmitting a partially
    /// filled transmission buffer. The shorter the value the more prompt the display.
    uint32_t refreshPeriod = 1000u;

    /// Signs if writing the FreeRTOS queue can block or should return on the expense
    /// of losing chunks. Note, that even in blocking mode the throughput can not
    /// reach the theoretical UART bps limit.
    /// Important! In non-blocking mode high demands will result in loss of complete
    /// messages and often an internal lockup of the log system.
    bool blocks = true;

    /// Representation of a task in the message header, if any. It can be missing,
    /// numeric task ID or FreeRTOS task name.
    TaskRepresentation taskRepresentation = TaskRepresentation::cId;

    /// True if number formatter should append 0b or 0x
    bool appendBasePrefix = false;

    /// Format for displaying the task ID in the message header.
    LogFormat taskIdFormat = cX2;

    /// Format for displaying the FreeRTOS ticks in the header, if any. Should be
    /// LogFormat::cNone to disable tick output.
    LogFormat tickFormat   = cD5;

    /// These are default formats for some types.
    LogFormat int8Format   = cDefault;
    LogFormat int16Format  = cDefault;
    LogFormat int32Format  = cDefault;
    LogFormat int64Format  = cDefault;
    LogFormat uint8Format  = cDefault;
    LogFormat uint16Format = cDefault;
    LogFormat uint32Format = cDefault;
    LogFormat uint64Format = cDefault;
    LogFormat floatFormat  = cD5;
    LogFormat doubleFormat = cD8;

    /// If true, positive numbers will be prepended with a space to let them align negatives.
    bool alignSigned = false;

    LogConfig() noexcept = default;
  };

  class Log;

  /// Abstract base class for OS/architecture/dependent log functionality under
  /// the Log class. The instance directly referenced by the Log object will
  /// contain an OS thread to let the actual write into the sink happen
  /// independently of the logging.
  /// Since this object requires OS resources, it must be constructed during
  /// initialization of the application to be sure all resources are granted.
  class LogOsInterface : public BanCopyMove {
  protected:
    /// Chunk size, see in LogConfig.
    LogSizeType const mChunkSize;

    /// Length of a pause in ms during waiting for transmission.
    uint32_t mPauseLength;

    /// See in LogConfig.
    uint32_t mRefreshPeriod;

  public:
    /// Has default constructor to let the stub versions work.
    LogOsInterface()
      : mChunkSize(1)
      , mPauseLength(1)
      , mRefreshPeriod(1) {
    }

    /// Has default constructor to let the stub versions work.
    LogOsInterface(LogConfig const & aConfig)
      : mChunkSize(aConfig.chunkSize)
      , mPauseLength(aConfig.pauseLength)
      , mRefreshPeriod(aConfig.refreshPeriod) {
    }

    /// Has default destructor to let the stub versions work.
    virtual ~LogOsInterface() = default;

    /// Returns true if the implementation can examine whether we are in an ISR and we are in fact in an ISR.
    /// By default it returns false.
    static bool isInterrupt() noexcept {
      return false;
    }

    LogSizeType getChunkSize() const noexcept {
      return mChunkSize;
    }
   
    /// Only needed in implementations without an OS-supported task name and task ID exceeding uint32_t.
    /// This function does nothing.
    /// Registers the given name and an artificial ID in a local map.
    /// This function MUST NOT be called from user code.
    /// void Log::registerCurrentTask(char const * const aTaskName) may call it only.
    /// @param aTaskName Task name to register.
    virtual void registerThreadName(char const * const) noexcept {
    }

    /// Returns a textual representation of the given thread ID.
    /// This will be OS dependent.
    /// @return the thread ID text if called from a thread.
    virtual char const * const getThreadName(uint32_t const aHandle) noexcept = 0;

    /// Returns a textual representation of the current thread ID.
    /// This will be OS dependent.
    /// @return the thread ID text if called from a thread.
    virtual char const * const getCurrentThreadName() noexcept = 0;

    /// Returns a value unique among threads.
    virtual uint32_t getCurrentThreadId() noexcept = 0;

    /// Returns some kind of system time in an OS dependent way.
    /// Can be anything from OS ticks, ms or s.
    virtual uint32_t getLogTime() const noexcept = 0;

    /// Creates a separate thread for sending log contents to the sink.
    /// @param log the Log instance to be passed as parameter to the function in the other parameter
    /// @param threadFunc the function to serve as the body of the new thread.
    virtual void createTransmitterThread(Log *aLog, void(* aThreadFunc)(void *)) noexcept = 0;
      
    /// Joins the thread, if applicable to the OsInterface subclass. This des nothing.
    virtual void joinTransmitterThread() noexcept {
    };

    /// Enqueues the chunks, possibly blocking if the queue is full.
    virtual void push(char const * const aChunkStart, bool const aBlocks) noexcept = 0;

    /// Removes the oldest chunk from the queue.
    virtual bool pop(char * const aChunkStart) noexcept = 0;

    /// Pauses the current thread for a period determined during construction
    /// of the derived object.
    virtual void pause() noexcept = 0;

    /// Transmits the buffer contents to the sink and calls the chained
    /// object's transmit if any.
    /// @param buffer holds the contents to send.
    /// @param length number of characters to send.
    virtual void transmit(char const * const buffer, LogSizeType const length, std::atomic<bool> *mProgressFlag) noexcept {
    }

    virtual void startRefreshTimer(std::atomic<bool> *aRefreshFlag) noexcept {
    }

    /// Calls az OS-specific lock to acquire a critical section, if implemented
    virtual void lock() noexcept {
    }

    /// Calls az OS-specific lock to release critical section, if implemented
    virtual void unlock() noexcept {
    }
  };

  /// Auxiliary class, not part of the Log API.
  class Chunk final {
  public:
    static constexpr TaskIdType cInvalidTaskId = 0u;
    /// Artificial task ID for interrupts.
    static constexpr TaskIdType cIsrTaskId = std::numeric_limits<TaskIdType>::max();


  private:
    LogOsInterface *mOsInterface;
    char * const mOrigin;
    char * mChunk;
    LogSizeType const mChunkSize;
    LogSizeType const mBufferBytes;
    LogSizeType mIndex = 1;
    bool const mBlocks;

  public:
    Chunk() noexcept
      : mOsInterface(nullptr)
      , mOrigin(nullptr)
      , mChunk(nullptr)
      , mChunkSize(0u)
      , mBufferBytes(0u)
      , mBlocks(true) {
    }

    Chunk(LogOsInterface *aOsInterface, char * const aChunk, LogSizeType const aBufferLength) noexcept
      : mOsInterface(aOsInterface)
      , mOrigin(aChunk)
      , mChunk(aChunk)
      , mChunkSize(aOsInterface->getChunkSize())
      , mBufferBytes(aBufferLength * mChunkSize)
      , mBlocks(true) {
    }

    Chunk(LogOsInterface *aOsInterface
      , char * const aChunk
      , LogSizeType const aBufferLength
      , TaskIdType const aTaskId) noexcept
      : mOsInterface(aOsInterface)
      , mOrigin(aChunk)
      , mChunk(aChunk)
      , mChunkSize(aOsInterface->getChunkSize())
      , mBufferBytes(aBufferLength * mChunkSize)
      , mBlocks(true) {
      mChunk[0] = *reinterpret_cast<char const *>(&aTaskId);
    }

    Chunk(LogOsInterface *aOsInterface
      , char * const aChunk
      , LogSizeType const aBufferLength
      , TaskIdType const aTaskId
      , bool const aBlocks) noexcept
      : mOsInterface(aOsInterface)
      , mOrigin(aChunk)
      , mChunk(aChunk)
      , mChunkSize(aOsInterface->getChunkSize())
      , mBufferBytes(aBufferLength * mChunkSize)
      , mBlocks(aBlocks) {
      mChunk[0] = *reinterpret_cast<char const*>(&aTaskId);
    }
    
/*Chunk(Chunk const &) = default;
    Chunk(Chunk &&) = default;
    Chunk& operator=(Chunk const &) = default;
    Chunk& operator=(Chunk &&) = default;*/

    char * getData() const noexcept {
      return mChunk;
    }

    TaskIdType getTaskId() const noexcept {
      return *reinterpret_cast<TaskIdType*>(mChunk);
    }

    bool isValid() const noexcept {
      return mOsInterface != nullptr;
    }

    char * const operator++() noexcept {
      mIndex = 1;
      mChunk += mChunkSize;
      if(mChunk == mOrigin + mBufferBytes) {
        mChunk = mOrigin;
      }
      else { // nothing to do
      }
      return mChunk;
    }

    Chunk& operator=(char * const aStart) noexcept {
      mIndex = 1;
      mChunk = const_cast<char*>(aStart);
      return *this;
    }

    Chunk& operator=(Chunk const &aChunk) noexcept {
      mIndex = 1;
      for(int i = 0; i < mChunkSize; i++) {
        mChunk[i] = aChunk.mChunk[i];
      }
      return *this;
    }

    void invalidate() noexcept {
      mIndex = 1;
      mChunk[0] = cInvalidTaskId;
    }

    /// defined in .cpp to allow stub.
    void push(char const mChar) noexcept;

    void flush() noexcept {
      mChunk[mIndex++] = '\n';
      mOsInterface->push(mChunk, mBlocks);
      mIndex = 1;
    }

    void pop() noexcept {
      mIndex = 1;
      if(!mOsInterface->pop(mChunk)) {
        mChunk[0] = cInvalidTaskId;
      }
      else { // nothing to do
      }
    }
  };

  /// Dummy type to use in << chain as end marker.
  enum class LogShiftChainMarker : uint8_t {
    cEnd = 0u
  };

  class LogShiftChainHelper final {
    Log      *mLog;
    Chunk     mAppender;
    LogFormat mNextFormat;

  public:
    LogShiftChainHelper() : mLog(nullptr) {
    }

    LogShiftChainHelper(Log *aLog, Chunk aAppender)
    : mLog(aLog)
    , mAppender(aAppender) {
    }

    LogShiftChainHelper(Log *aLog, Chunk const aAppender, LogFormat const aFormat)
    : mLog(aLog)
    , mAppender(aAppender)
    , mNextFormat(aFormat) {
    }

    template<typename ArgumentType>
    LogShiftChainHelper& operator<<(ArgumentType const aValue) noexcept;

    LogShiftChainHelper& operator<<(LogFormat const &aFormat) noexcept {
      mNextFormat = aFormat;
      return *this;
    }

    LogShiftChainHelper& operator<<(LogShiftChainMarker const) noexcept;
  };

  /// High-level template based logging class for logging characters, C-style
  /// strings, integers up to 32 bit width and floating point types. This class
  /// is designed for 32 bit
  /// MCUs with small memory, performs serialization and number conversion
  /// itself. It uses a heap-allocated buffers and requires logging threads to
  /// register themselves. To ensure there is enough memory for all application
  /// functions, object construction and thread registration should be done
  /// as early as possible.
  /// The class operates roughly as follows:
  /// The message is constructed on the caller stack and is broken into chunks
  /// identified by the artificially created task ID. Therefore, all tasks should
  /// register themselves on the very beginning of their life. Calling from ISRs is
  /// also possible, but all such calls will get the same ID.
  /// These resulting chunks (log message parts) are then fed into a common
  /// FreeRTOS queue, either blocking to wait for place or letting chunks dropped.
  /// Concurrent loggings can get their chunks interleaved in the queue.
  /// If the queue becomes full, chunks will be dropped, which results in corrupted
  /// messages.
  /// An algorithm ensures the messages are de-interleaved in the order of their
  /// first chunk in the queue and fed into one transmission buffer, while the other
  /// one may be transmitted via DMA. A timeout value is used to ensure transmission
  /// in a defined amount of time even if the transmission buffer is not full.
  /// This class can has a stub implementation in an other .cpp file to prevent
  /// logging in release without the need of #ifdefs and macros. This class shall
  /// have only one instance, and can be accessed via a static method.
  /// If code size matters, the template argument pattern should come from
  /// a limited number parameter type combinations, and possibly only a small
  /// number of parameters.
  class Log final : public BanCopyMove {
    friend class LogShiftChainHelper;
  public:
    /// Will be used as Log << something << to << log << Log::end;
    static constexpr LogShiftChainMarker end = LogShiftChainMarker::cEnd;

    /// Output for unknown LogApp parameter
    static constexpr char cUnknownApplicationName[8] = "UNKNOWN";

  private:
    /// The character to use instead of the thread ID if it is unknown.
    static constexpr char cIsrTaskName = '?';

    /// The character to use to sign a failure during the number to text
    /// conversion. There is no distinction between illegal base (neither of 2,
    /// 10, 16) or overflow of the conversion buffer.
    static constexpr char cNumericError = '#';

    /// Zero-fill character.
    static constexpr char cNumericFill = '0';

    /// Separator between header fields of the log message.
    static constexpr char cSeparatorNormal = ' ';

    /// Separator signing message truncation due to buffer overflow. An unknown
    /// amount of subsequent messages may be missing.
    static constexpr char cSeparatorFailure = '@';

    /// Used to convert digits to characters.
    static constexpr char cDigit2char[16] = {
      '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'
    };

    /// The subclass object used as interface to the OS, which also handles
    /// locking, time and thread management.
    LogOsInterface &mOsInterface;

    /// Can be used to shut off the transmitter thread, if any
    std::atomic<bool> mKeepRunning = true;

    /// The user-defined configuration values for message header and number
    /// rendering and else.
    LogConfig const &mConfig;

    /// See in LogConfig.
    LogSizeType const mChunkSize;

    /// The next value of the artificial task ID. If overflows to 0, will
    /// remain there, so at most 255 tasks are allowed.
    TaskIdType mNextTaskId = 1;

    /// Map used to turn OS-specific task IDs into the artificial counterparts.
    std::map<uint32_t, TaskIdType> mTaskIds;

    /// Registry to check calls like Log::send(nowtech::LogApp::cSystem, "stuff to log")
    std::map<LogApp, char const *> mRegisteredApps;

    /// Used for Chunk buffers during shift chain-type calls.
    char *mShiftChainingCallBuffers = nullptr;

    /// Instance for static access.
    static Log *sInstance;

  public:
    /// Defined in .cpp to allow stub.
    /// Creates a new Log instance using the provided OS interface
    /// implementation.
    /// @param aOsInterface the interface instance. This will be used to create
    /// a new thread, provide time and locking.
    /// @param aConfig configuration.
    Log(LogOsInterface &aOsInterface, LogConfig const &aConfig) noexcept;

    /// Does nothing, because this object is not intended to be destroyed.
    ~Log() noexcept {
      mKeepRunning.store(false);
      mOsInterface.joinTransmitterThread();
      delete[] mShiftChainingCallBuffers;
    }

    /// Registers the current task if not already present. It can register
    /// at most 255 tasks. All others will be handled as one.
    /// NOTE: this method locks to inhibit concurrent access of methods with the same name.
    static void registerCurrentTask() noexcept {
      sInstance->doRegisterCurrentTask();
    }

    /// Registers the current task if not already present. It can register
    /// at most 255 tasks. All others will be handled as one.
    /// NOTE: this method locks to inhibit concurrent access of methods with the same name.
    /// @param aTaskName Task name to use, when the osInterface supports it.
    static void registerCurrentTask(char const * const aTaskName) noexcept {
      sInstance->doRegisterCurrentTask(aTaskName);
    }

    /// Registers the current log application
    /// at most 255 tasks. All others will be handled as one.
    static void registerApp(LogApp aApp, char const * const aPrefix) noexcept {
      sInstance->mRegisteredApps[aApp] = aPrefix;
    }

    /// Returns true if the given app was registered.
    static bool isRegistered(LogApp aApp) noexcept {
      return sInstance->mRegisteredApps.find(aApp) != sInstance->mRegisteredApps.end();
    }

    static Log& i() noexcept {
      return *sInstance;
    }

    /// Transmitter thread implementation.
    void transmitterThreadFunction() noexcept;

    /// Starts a << operator chain with the specified argument
    template<typename ArgumentType>
    LogShiftChainHelper operator<<(ArgumentType const aValue) noexcept {
      if(mShiftChainingCallBuffers) {
        TaskIdType taskId = getCurrentTaskId();
        Chunk appender = startSend(mShiftChainingCallBuffers + taskId * mChunkSize, taskId);
        if(appender.isValid()) {
          append(appender, aValue);
          return LogShiftChainHelper(this, appender);
        }
        else {
          return LogShiftChainHelper();
        }
      }
      else {
        return LogShiftChainHelper();
      }
    }

    /// Starts a << operator chain with the specified LogApp
    LogShiftChainHelper operator<<(LogApp const aApp) noexcept;

    /// Starts a << operator chain with the specified format to be used by the next argument
    LogShiftChainHelper operator<<(LogFormat const &aFormat) noexcept;

    /// Finishes the << operator chain just started
    LogShiftChainHelper operator<<(LogShiftChainMarker const aMarker) noexcept;

    /// If aApp is registered, calls the normal send to process the arguments
    template<typename... Args>
    static void send(LogApp aApp, Args... args) noexcept {
      char chunk[sInstance->mChunkSize];
      Chunk appender = sInstance->startSend(chunk, Chunk::cInvalidTaskId, aApp);
      if(appender.isValid()) {
        sInstance->doSend(appender, args...);
      }
      else { // nothing to do
      }
    }

    /// Sends any number of parameters which may be:
    /// - char
    /// - char string
    /// - int8_t
    /// - int16_t
    /// - int32_t
    /// - int64_t
    /// - uint8_t
    /// - uint16_t
    /// - uint32_t
    /// - uint64_t
    /// - bool
    /// - float
    /// - double
    /// - the integer and floating point types preceded by a possible LogFormat
    /// object for formatting.
    /// Other types are not sent, but signed by -=unknown=-
    /// Please avoid sending #, @ or newline characters. Sends newline
    /// automatically in the end.
    template<typename... Args>
    static void send(Args... args) noexcept {
      char chunk[sInstance->mChunkSize];
      Chunk appender = sInstance->startSend(chunk, Chunk::cInvalidTaskId);
      if(appender.isValid()) {
        sInstance->doSend(appender, args...);
      }
      else { // nothing to do
      }
    }

    /// Similar to send but does not emit any preconfigured header.
    template<typename... Args>
    static void sendNoHeader(LogApp aApp, Args... args) noexcept {
      char chunk[sInstance->mChunkSize];
      Chunk appender = sInstance->startSendNoHeader(chunk, Chunk::cInvalidTaskId, aApp);
      if(appender.isValid()) {
        sInstance->doSend(appender, args...);
      }
      else { // nothing to do
      }
    }

    /// Similar to send but does not emit any preconfigured header.
    template<typename... Args>
    static void sendNoHeader(Args... args) noexcept {
      char chunk[sInstance->mChunkSize];
      Chunk appender = sInstance->startSendNoHeader(chunk, Chunk::cInvalidTaskId);
      if(appender.isValid()) {
        sInstance->doSend(appender, args...);
      }
      else { // nothing to do
      }
    }

    /// Sends the trailing newline character.
    void finishSend(Chunk &aChunk) noexcept {
      aChunk.flush();
    }

private:
    void doRegisterCurrentTask() noexcept;
    void doRegisterCurrentTask(char const * const) noexcept;

    /// Defined in .cpp to allow stub
    TaskIdType getCurrentTaskId() const noexcept;

    /// Building block for variadic template based message construction.
    template<typename T>
    void doSend(Chunk &aChunk, T const aValue) noexcept {
      append(aChunk, aValue);
      finishSend(aChunk);
    }

    /// Building block for variadic template based message construction.
    template<typename T>
    void doSend(Chunk &aChunk, LogFormat& aFormat, T const aValue) noexcept {
      append(aChunk, aFormat, aValue);
      finishSend(aChunk);
    }

    /// Building block for variadic template based message construction.
    template<typename T, typename... Args>
    void doSend(Chunk &aChunk, T const aValue, Args... aArgs) noexcept {
      append(aChunk, aValue);
      doSend(aChunk, aArgs...);
    }

    /// Building block for variadic template based message construction.
    template<typename = LogFormat, typename T, typename... Args>
    void doSend(Chunk &aChunk, LogFormat& aFormat, T const aValue, Args... aArgs) noexcept {
      append(aChunk, aFormat, aValue);
      doSend(aChunk, aArgs...);
    }

    Chunk startSend(char * const aChunkBuffer, TaskIdType const aTaskId) noexcept;
    Chunk startSend(char * const aChunkBuffer, TaskIdType const aTaskId, LogApp aApp) noexcept;
    Chunk startSendNoHeader(char * const aChunkBuffer, TaskIdType const aTaskId) noexcept;
    Chunk startSendNoHeader(char * const aChunkBuffer, TaskIdType const aTaskId, LogApp aApp) noexcept;

    template<typename T>
    void append(Chunk &aChunk, LogFormat& aFormat, T const aValue) noexcept {
      /// avoid too many template instantiations
      if(std::is_same<T, int8_t>::value || std::is_same<T, int16_t>::value || std::is_same<T, int32_t>::value) {
        append(aChunk, static_cast<int32_t>(aValue), static_cast<int32_t>(aFormat.base), aFormat.fill);
      }
      else if(std::is_same<T, uint8_t>::value || std::is_same<T, uint16_t>::value || std::is_same<T, uint32_t>::value) {
        append(aChunk, static_cast<uint32_t>(aValue), static_cast<uint32_t>(aFormat.base), aFormat.fill);
      }
      else if(std::is_same<T, int64_t>::value) {
        append(aChunk, static_cast<int64_t>(aValue), static_cast<int64_t>(aFormat.base), aFormat.fill);
      }
      else if(std::is_same<T, uint64_t>::value) {
        append(aChunk, static_cast<uint64_t>(aValue), static_cast<uint64_t>(aFormat.base), aFormat.fill);
      }
      else if(std::is_same<T, float>::value || std::is_same<T, double>::value) {
        append(aChunk, static_cast<double>(aValue), aFormat.fill);
      }
      else {
        append(aChunk, "-=unknown=-");
      }
    }

    void append(Chunk &aChunk, bool const aBool) noexcept {
      if(aBool) {
        append(aChunk, "true");
      }
      else {
        append(aChunk, "false");
      }
    }

    /// Appends the character to the message. If this is the last to fit the
    /// buffer, appends a newline instead of the given character, and changes
    /// the preceding one to @ to show that the message is truncated and
    /// possibly other messages will be dropped.
    /// @param ch character to append.
    /// @return true if succeeded, false if truncation occurs or buffer was full.
    void append(Chunk &aChunk, char const aCh) noexcept {
      aChunk.push(aCh);
    }

    /// Uses append(char const ch) to send the string character by character.
    /// @param string to send
    /// @return the return value of the last append(char const ch) call.
    void append(Chunk &aChunk, char const * const aString) noexcept {
      int i = 0;
      if(aString != nullptr) {
        while(aString[i] != 0) {
          aChunk.push(aString[i++]);
        }
      }
    }

    /// Uses append(T const value, T const base, uint8_t const fill) with mConfig.uint8Format
    /// @param value number to convert and send
    /// @return the return value of the last append(char const ch) call.
    void append(Chunk &aChunk, uint8_t const aValue) noexcept {
      append(aChunk, static_cast<uint32_t>(aValue), static_cast<uint32_t>(mConfig.uint8Format.base), mConfig.uint8Format.fill);
    }

    /// Uses append(T const value, T const base, uint8_t const fill) with mConfig.uint16Format
    /// @param value number to convert and send
    /// @return the return value of the last append(char const ch) call.
    void append(Chunk &aChunk, uint16_t const aValue) noexcept {
      append(aChunk, static_cast<uint32_t>(aValue), static_cast<uint32_t>(mConfig.uint16Format.base), mConfig.uint16Format.fill);
    }

    /// Uses append(T const value, T const base, uint8_t const fill) with mConfig.uint32Format
    /// @param value number to convert and send
    /// @return the return value of the last append(char const ch) call.
    void append(Chunk &aChunk, uint32_t const aValue) noexcept {
      append(aChunk, aValue, static_cast<uint32_t>(mConfig.uint32Format.base), mConfig.uint32Format.fill);
    }

    /// Uses append(T const value, T const base, uint8_t const fill) with mConfig.uint64Format
    /// NOTE perhaps to be avoided in 32-bit embedded environment.
    /// @param value number to convert and send
    /// @return the return value of the last append(char const ch) call.
    void append(Chunk &aChunk, uint64_t const aValue) noexcept {
      append(aChunk, aValue, static_cast<uint64_t>(mConfig.uint32Format.base), mConfig.uint32Format.fill);
    }

    /// Uses append(T const value, T const base, uint8_t const fill) with mConfig.int8Format
    /// @param value number to convert and send
    /// @return the return value of the last append(char const ch) call.
    void append(Chunk &aChunk, int8_t const aValue) noexcept {
      append(aChunk, static_cast<int32_t>(aValue), static_cast<int32_t>(mConfig.int8Format.base), mConfig.int8Format.fill);
    }

    /// Uses append(T const value, T const base, uint8_t const fill) with mConfig.int16Format
    /// @param value number to convert and send
    /// @return the return value of the last append(char const ch) call.
    void append(Chunk &aChunk, int16_t const aValue) noexcept {
      append(aChunk, static_cast<int32_t>(aValue), static_cast<int32_t>(mConfig.int16Format.base), mConfig.int16Format.fill);
    }

    /// Uses append(T const value, T const base, uint8_t const fill) with mConfig.int32Format
    /// @param value number to convert and send
    /// @return the return value of the last append(char const ch) call.
    void append(Chunk &aChunk, int32_t const aValue) noexcept {
      append(aChunk, aValue, static_cast<int32_t>(mConfig.int32Format.base), mConfig.int32Format.fill);
    }

    /// Uses append(T const value, T const base, uint8_t const fill) with mConfig.int64Format
    /// NOTE perhaps to be avoided in 32-bit embedded environment.
    /// @param value number to convert and send
    /// @return the return value of the last append(char const ch) call.
    void append(Chunk &aChunk, int64_t const aValue) noexcept {
      append(aChunk, aValue, static_cast<int64_t>(mConfig.int64Format.base), mConfig.int64Format.fill);
    }

    void append(Chunk &aChunk, float const aValue) noexcept {
      append(aChunk, static_cast<double>(aValue), mConfig.floatFormat.fill);
    }

    void append(Chunk &aChunk, double const aValue) noexcept {
      append(aChunk, aValue, mConfig.doubleFormat.fill);
    }

    /// Converts the number to string in a stack buffer and uses append(char
    /// const ch) to send it character by character. If the conversion fails
    /// (due to invalid base or too small buffer on stack) a # will be appended
    /// instead. For non-decimal numbers 0b or 0x is prepended.
    /// T should be int32_t, uint32_t, int64_t or uint64_t to avoid too many template instantiations.
    /// @param value the number to convert
    /// @param base of the number system to use
    /// @param fill number of digits to use at least. Shorter numbers will be
    /// filled using cNumericFill.
    /// @return the return value of the last append(char const ch) call.
    template<typename T>
    void append(Chunk &aChunk, T const value, T const base, uint8_t const fill) noexcept {
      T tmpValue = value;
      uint8_t tmpFill = fill;
      if(base != 2 && base != 10 && base != 16) {
        aChunk.push(cNumericError);
        return;
      }
      else { // nothing to do
      }
      if(mConfig.appendBasePrefix && base == 2) {
        aChunk.push('0');
        aChunk.push('b');
      }
      else { // nothing to do
      }
      if(mConfig.appendBasePrefix && base == 16) {
        aChunk.push('0');
        aChunk.push('x');
      }
      else { // nothing to do
      }
      char tmpBuffer[mConfig.appendStackBufferLength];
      uint8_t where = 0;
      bool negative = value < 0;
      do {
        T mod = tmpValue % base;
        if(mod < 0) {
          mod = -mod;
        }
        else { // nothing to do
        }
        tmpBuffer[where] = cDigit2char[mod];
        ++where;
        tmpValue /= base;
      }
      while(tmpValue != 0 && where <= mConfig.appendStackBufferLength);
      if(where > mConfig.appendStackBufferLength) {
        aChunk.push(cNumericError);
        return;
      }
      else { // nothing to do
      }
      if(negative) {
        aChunk.push('-');
      }
      else if(mConfig.alignSigned && fill > 0) {
        aChunk.push(' ');
      }
      else { // nothing to do
      }
      if(tmpFill > where) {
        tmpFill -= where;
        while(tmpFill > 0) {
          aChunk.push(cNumericFill);
          --tmpFill;
        }
      }
      for(--where; where > 0; --where) {
        aChunk.push(tmpBuffer[where]);
      }
      aChunk.push(tmpBuffer[0]);
    }

    void append(Chunk &aChunk, double const aValue, uint8_t const aDigitsNeeded) noexcept;
  };// class Log

  template<typename ArgumentType>
  LogShiftChainHelper& LogShiftChainHelper::operator<<(ArgumentType const aValue) noexcept {
    if(mLog != nullptr) {
      if(mNextFormat.isValid()) {
        mLog->append(mAppender, mNextFormat, aValue);
        mNextFormat.base = 0u;
      }
      else {
        mLog->append(mAppender, aValue);
      }
    }
    else { // nothing to do
    }
    return *this;
  }

} // namespace nowtech

typedef nowtech::Log Log;
typedef nowtech::LogConfig LC;

#endif // NOWTECH_LOG_INCLUDED
