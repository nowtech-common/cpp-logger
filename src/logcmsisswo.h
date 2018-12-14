#ifndef NOWTECH_LOG_CMSISSWO_INCLUDED
#define NOWTECH_LOG_CMSISSWO_INCLUDED

#include "log.h"
#include "cmsis_os.h"  // we use only an assembly macro, nothing more CMSIS-related

namespace nowtech {

  /// Class implementing log interface for CMSIS SWO alone without any buffering or concurrency support.
  /// The ITM_SendChar contains a busy wait loop without timeout. If this is an issue,
  /// the function should be implemented with a timeout.
  class LogCmsisSwo final : public LogOsInterface {
  public:
    /// Sets parameters and creates the mutex for locking.
    /// @param aConfig config.
    LogCmsisSwo(LogConfig const & aConfig) noexcept
      : LogOsInterface(aConfig) {
    }

    /// This object is not intended to be deleted, so control should never
    /// get here.
    virtual ~LogCmsisSwo() {
    }

    /// Returns nullptr.
    virtual const char * const getCurrentThreadName() noexcept {
      return nullptr;
    }

    /// Returns 0.
    virtual uint32_t getCurrentThreadId() noexcept {
      return 0u;
    }

    /// Returns 0.
    virtual uint32_t getLogTime() const noexcept {
      return 0u;
    }

    /// Does nothing.
    virtual void createTransmitterThread(Log *, void(*)(void *)) noexcept {
    }

    /// Sends the chunk contents immediately.
    virtual void push(char const * const aChunkStart, bool const) noexcept {
      LogSizeType length;
      for(length = 0; length < mChunkSize - 1; ++length) {
        ITM_SendChar(static_cast<uint32_t>(aChunkStart[length + 1]));
        if(aChunkStart[length + 1] == '\n') {
          ++length;
          break;
        }
      }
    }

    /// Does nothing.
    virtual bool pop(char * const aChunkStart) noexcept {
      return false;
    }

    /// Does nothing.
    virtual void pause() noexcept {
    }

    /// Does nothing.
    virtual void transmit(const char * const, LogSizeType const, std::atomic<bool> *) noexcept {
    }

    /// Does nothing.
    static void transmitFinished() noexcept {
    }

    /// Does nothing.
    virtual void startRefreshTimer(std::atomic<bool> *) noexcept {
    }

    /// Does nothing.
    static void refreshNeeded() noexcept {
    }

  };

} //namespace nowtech

#endif // NOWTECH_LOG_CMSISSWO_INCLUDED
