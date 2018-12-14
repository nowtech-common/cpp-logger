#ifndef NOWTECH_LOG_STMHAL_INCLUDED
#define NOWTECH_LOG_STMHAL_INCLUDED

#include "log.h"
#include "stm32hal.h"

namespace nowtech {

  /// Class implementing log interface for STM HAL alone without any buffering or concurrency support.
  class LogStmHal final : public LogOsInterface {
  private:
    /// The STM HAL serial port descriptor to use.
    UART_HandleTypeDef* mSerialDescriptor;

    /// Timeout for sending in ms
    uint32_t const mUartTimeout;

  public:
    /// Sets parameters and creates the mutex for locking.
    /// @param aSerialDescriptor the STM HAL serial port descriptor to use
    /// @param aConfig config.
    /// @param aUartTimeout timeout for sending perhaps in ms
    LogStmHal(UART_HandleTypeDef* const aSerialDescriptor
      , LogConfig const & aConfig
      , uint32_t const aUartTimeout) noexcept
      : LogOsInterface(aConfig)
      , mSerialDescriptor(aSerialDescriptor)
      , mUartTimeout(aUartTimeout) {
    }

    /// This object is not intended to be deleted, so control should never
    /// get here.
    virtual ~LogStmHal() {
    }

    /// Returns a textual representation of the given thread ID.
    /// @return nullptr.
    virtual char const * const getThreadName(uint32_t const aHandle) noexcept {
      return nullptr;
    };

    /// @return nullptr.
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
      char buffer[mChunkSize];
      for(length = 0u; length < mChunkSize - 1u; ++length) {
        buffer[length] = aChunkStart[length + 1u];
        if(aChunkStart[length + 1] == '\n') {
          ++length;
          break;
        }
      }
      HAL_UART_Transmit(mSerialDescriptor, reinterpret_cast<uint8_t*>(const_cast<char*>(buffer)), length, mUartTimeout);
    }

    /// Does nothing.
    virtual bool pop(char * const aChunkStart) noexcept {
      return true;
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

#endif // NOWTECH_STMHAL_INCLUDED
