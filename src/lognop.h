#ifndef NOWTECH_LOG_NOP_INCLUDED
#define NOWTECH_LOG_NOP_INCLUDED

#include "log.h"

namespace nowtech {

/// Class implementing a no-operation log interface.
class LogNop final : public LogOsInterface {
public:
  /// Does nothing.
  LogNop(LogConfig const & aConfig) noexcept
    : LogOsInterface(aConfig) {
  }

  /// This object is not intended to be deleted, so control should never
  /// get here.
  virtual ~LogNop() {
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

  /// Does nothing.
  virtual void push(char const * const aChunkStart, bool const) noexcept {
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

#endif // NOWTECH_LOG_NOP_INCLUDED
