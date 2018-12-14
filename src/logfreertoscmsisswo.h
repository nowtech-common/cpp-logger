#ifndef NOWTECH_LOGFREERTOSCMSISSWO_H_INCLUDED
#define NOWTECH_LOGFREERTOSCMSISSWO_H_INCLUDED

#include "log.h"
#include "stm32f2xx_hal.h"
#include "cmsis_os.h"  // we use only an assembly macro, nothing more CMSIS-related
#include "FreeRTOS.h"
#include "projdefs.h"
#include "task.h"
#include "timers.h"
#include "queue.h"
#include <atomic>

extern "C" void logRefreshNeeded(TimerHandle_t);

namespace nowtech {

  /// Class implementing log interface for FreeRTOS and STM HAL as STM32CubeMX
  /// provides them.
  class LogFreeRtosCmsisSwo final : public LogOsInterface {
  private:
    /// Stack length of the transmitter task. It should be large enough to
    /// accomodate Log.transmitStackBufferLength
    uint16_t const mTaskStackLength;

    /// Priority for the transmitter task.
    UBaseType_t const mPriority;

    /// The transmitter task's handle.
    TaskHandle_t mTaskHandle = nullptr;

    /// Used for mutual exclusion for the Log.buffer
    QueueHandle_t mQueue;

    /// Used to force transmission of partially filled buffer in a defined
    /// period of time.
    TimerHandle_t mRefreshTimer;

    /// True if the partially filled buffer should be sent. This is
    /// defined here because OS-specific functionality is here.
    static std::atomic<bool> *sRefreshNeeded;

  public:
    /// Sets parameters and creates the mutex for locking.
    /// @param aConfig config.
    /// @param aTaskStackLength stack length of the transmitter task
    /// @param aPriority of the tranmitter task
    LogFreeRtosCmsisSwo(LogConfig const & aConfig
      , uint16_t const aTaskStackLength
      , UBaseType_t const aPriority) noexcept
      : LogOsInterface(aConfig)
      , mTaskStackLength(aTaskStackLength)
      , mPriority(aPriority) {
      mQueue = nowtech::OsUtil::taskCreate(aConfig.mQueueLength, mChunkSize);
      mRefreshTimer = xTimerCreate("LogRefreshTimer", pdMS_TO_TICKS(mRefreshPeriod), pdFALSE, nullptr, logRefreshNeeded);
    }

    /// This object is not intended to be deleted, so control should never
    /// get here.
    virtual ~LogFreeRtosCmsisSwo() {
    }

    /// Returns the task name.
    /// Must not be called from ISR.
    virtual const char * const getCurrentThreadName() noexcept {
      return pcTaskGetName(nullptr);
    }

    /// Returns the FreeRTOS-specific thread ID.
    /// Must not be called from ISR.
    virtual uint32_t getCurrentThreadId() noexcept {
      return reinterpret_cast<uint32_t>(xTaskGetCurrentTaskHandle());
    }

    /// Returns the FreeRTOS tick count converted into ms.
    virtual uint32_t getLogTime() const noexcept {
      return nowtech::OsUtil::getUptimeMillis();
    }

    /// Creates the transmitter thread using the name logtransmitter.
    /// @param log the Log object to operate on.
    /// @param threadFunc the C function which serves as the task body and
    /// which will call Log.transmitterThread.
    virtual void createTransmitterThread(Log *aLog, void(* aThreadFunc)(void *)) noexcept {
      xTaskCreate(aThreadFunc, "logtransmitter", mTaskStackLength, aLog, mPriority, &mTaskHandle);
    }

    /// Enqueues the chunks, possibly blocking if the queue is full.
    /// In ISR, we do not use pxHigherPriorityTaskWoken because
    /// if it ever causes a problem, the first thing to do will
    /// be to remove logging from ISR.
    /// "If you set the parameter to NULL, the context switch
    /// will happen at the next tick (ticks happen each millisecond
    /// if you are using the default settings), not immediatlely after
    /// the end of the ISR. Depending on your use-case this may
    /// be acceptable or not."
    /// https://stackoverflow.com/questions/28985010/about-pxhigherprioritytaskwoken
    virtual void push(char const * const aChunkStart, bool const aBlocks) noexcept {
      if(stm32utils::isInterrupt()) {
        xQueueSendFromISR(mQueue, aChunkStart, nullptr);
      }
      else {
        xQueueSend(mQueue, aChunkStart, aBlocks ? portMAX_DELAY : 0);
      }
    }

    /// Removes the oldest chunk from the queue.
    virtual bool pop(char * const aChunkStart) noexcept {
      auto ret = xQueueReceive(mQueue, aChunkStart, pdMS_TO_TICKS(mPauseLength));
      return ret == pdTRUE ? true : false; // TODO remove
    }

    /// Pauses execution for the period given in the constructor.
    virtual void pause() noexcept {
      vTaskDelay(pdMS_TO_TICKS(mPauseLength));
    }

    /// Transmits the data using the serial descriptor given in the constructor.
    /// @param buffer start of data
    /// @param length length of data
    /// @param aProgressFlag address of flag to be set on transmission end.
    virtual void transmit(const char * const aBuffer, LogSizeType const aLength, std::atomic<bool> *aProgressFlag) noexcept {
      for(LogSizeType i = 0; i < aLength; i++) {
        ITM_SendChar(static_cast<uint32_t>(aBuffer[i]));
      }
      aProgressFlag->store(false);
    }

    /// Starts the timer after which a partially filled buffer should be sent.
    virtual void startRefreshTimer(std::atomic<bool> *aRefreshFlag) noexcept {
      sRefreshNeeded = aRefreshFlag;
      xTimerStart(mRefreshTimer, 0);
    }

    /// Sets the flag.
    static void refreshNeeded() noexcept {
      sRefreshNeeded->store(true);
    }

  };

} //namespace nowtech

#endif /* NOWTECH_LOGFREERTOSCMSISSWO_H_INCLUDED */