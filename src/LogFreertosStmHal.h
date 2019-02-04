//
// Copyright 2018 Now Technologies Zrt.
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without restriction,
// including without limitation the rights to use, copy, modify, merge,
// publish, distribute, sublicense, and/or sell copies of the Software,
// and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH
// THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

#ifndef NOWTECH_LOG_FREERTOS_STMHAL_INCLUDED
#define NOWTECH_LOG_FREERTOS_STMHAL_INCLUDED

#include "Log.h"
#include "CmsisOsUtils.h"
#include "FreeRTOS.h"
#include "projdefs.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"
#include "queue.h"
#include "stm32hal.h"
#include "stm32utils.h"
#include <atomic>

extern "C" void logRefreshNeededFreeRtosStmHal(TimerHandle_t);

namespace nowtech {

  /// Class implementing log interface for FreeRTOS and STM HAL as STM32CubeMX
  /// provides them.
  class LogFreeRtosStmHal final : public LogOsInterface {
  private:
    /// The STM HAL serial port descriptor to use.
    static UART_HandleTypeDef* sSerialDescriptor;

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

    /// Used for lock and unlock calls.
    SemaphoreHandle_t         mApiGuard;

    /// True if the other transmission buffer is being transmitted. This is
    /// defined here because OS-specific functionality is here.
    static std::atomic<bool> *sProgressFlag;

    /// True if the partially filled buffer should be sent. This is
    /// defined here because OS-specific functionality is here.
    static std::atomic<bool> *sRefreshNeeded;

  public:
    /// Sets parameters and creates the mutex for locking.
    /// @param aSerialDescriptor the STM HAL serial port descriptor to use
    /// @param aConfig config.
    /// @param aTaskStackLength stack length of the transmitter task
    /// @param aPriority of the tranmitter task
    LogFreeRtosStmHal(UART_HandleTypeDef* const aSerialDescriptor
      , LogConfig const & aConfig
      , uint16_t const aTaskStackLength
      , UBaseType_t const aPriority) noexcept
      : LogOsInterface(aConfig)
      , mTaskStackLength(aTaskStackLength)
      , mPriority(aPriority) {
      mQueue = xQueueCreate(aConfig.queueLength, mChunkSize);
      mRefreshTimer = xTimerCreate("LogRefreshTimer", pdMS_TO_TICKS(mRefreshPeriod), pdFALSE, nullptr, logRefreshNeededFreeRtosStmHal);
      mApiGuard = xSemaphoreCreateMutex();
      sSerialDescriptor = aSerialDescriptor;
    }

    /// This object is not intended to be deleted, so control should never
    /// get here.
    virtual ~LogFreeRtosStmHal() {
      vQueueDelete(mQueue);
      xTimerDelete(mRefreshTimer, 0);
      vSemaphoreDelete(mApiGuard);
    }

    /// Returns true if we are in an ISR.
    static bool isInterrupt() noexcept {
      return stm32utils::isInterrupt();
    }

    /// Returns the task name.
    /// Must not be called from ISR.
    virtual const char * getThreadName(uint32_t const aHandle) noexcept override {
      return pcTaskGetName(reinterpret_cast<TaskHandle_t>(aHandle));
    }

    /// Returns the current task name.
    /// Must not be called from ISR.
    virtual const char * getCurrentThreadName() noexcept override {
      return pcTaskGetName(nullptr);
    }

    /// Returns the FreeRTOS-specific thread ID.
    /// Must not be called from ISR.
    virtual uint32_t getCurrentThreadId() noexcept override {
      return reinterpret_cast<uint32_t>(xTaskGetCurrentTaskHandle());
    }

    /// Returns the FreeRTOS tick count converted into ms.
    virtual uint32_t getLogTime() const noexcept override {
      return nowtech::OsUtil::getUptimeMillis();
    }

    /// Creates the transmitter thread using the name logtransmitter.
    /// @param log the Log object to operate on.
    /// @param threadFunc the C function which serves as the task body and
    /// which will call Log.transmitterThread.
    virtual void createTransmitterThread(Log *aLog, void(* aThreadFunc)(void *)) noexcept override {
      xTaskCreate(aThreadFunc, "logtransmitter", mTaskStackLength, aLog, mPriority, &mTaskHandle);
    }

    /// Joins the thread.
    virtual void joinTransmitterThread() noexcept override {
      vTaskDelete(mTaskHandle);
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
    virtual void push(char const * const aChunkStart, bool const aBlocks) noexcept override {
      if(stm32utils::isInterrupt()) {
        BaseType_t higherPriorityTaskWoken;
        xQueueSendFromISR(mQueue, aChunkStart, &higherPriorityTaskWoken);
        if(higherPriorityTaskWoken == pdTRUE) {
          //TODO consider if needed. In theory not necessary
          portYIELD_FROM_ISR(pdTRUE);
        }
      }
      else {
        xQueueSend(mQueue, aChunkStart, aBlocks ? portMAX_DELAY : 0);
      }
    }

    /// Removes the oldest chunk from the queue.
    virtual bool pop(char * const aChunkStart) noexcept override {
      auto ret = xQueueReceive(mQueue, aChunkStart, nowtech::OsUtil::msToRtosTick(mPauseLength));
      return ret == pdTRUE ? true : false; // TODO remove
    }

    /// Pauses execution for the period given in the constructor.
    virtual void pause() noexcept override {
      nowtech::OsUtil::taskDelayMillis(mPauseLength);
    }

    /// Transmits the data using the serial descriptor given in the constructor.
    /// @param buffer start of data
    /// @param length length of data
    /// @param aProgressFlag address of flag to be set on transmission end.
    virtual void transmit(const char * const aBuffer, LogSizeType const aLength, std::atomic<bool> *aProgressFlag) noexcept override {
      sProgressFlag = aProgressFlag;
      HAL_UART_Transmit_DMA(sSerialDescriptor, reinterpret_cast<uint8_t*>(const_cast<char*>(aBuffer)), aLength);
    }

    /// Sets the flag.
    ///
    /// Needs the following function with such a line in the application:
    ///
    /// extern "C" void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    ///   nowtech::LogFreeRtosStmHal::transmitFinished(huart);
    /// }
    static void transmitFinished(UART_HandleTypeDef const * const huart) noexcept {
      if(huart == sSerialDescriptor) {
        sProgressFlag->store(false);
      }
      else { // nothing to do
      }
    }

    /// Starts the timer after which a partially filled buffer should be sent.
    virtual void startRefreshTimer(std::atomic<bool> *aRefreshFlag) noexcept override {
      sRefreshNeeded = aRefreshFlag;
      xTimerStart(mRefreshTimer, 0);
    }

    /// Sets the flag.
    static void refreshNeeded() noexcept {
      sRefreshNeeded->store(true);
    }

    /// Calls az OS-specific lock to acquire a critical section, if implemented
    virtual void lock() noexcept override {
      xSemaphoreTakeFromISR(mApiGuard, nullptr);
    }

    /// Calls az OS-specific lock to release critical section, if implemented
    virtual void unlock() noexcept override {
      xSemaphoreGiveFromISR(mApiGuard, nullptr);
    }
  };

} //namespace nowtech

#endif // NOWTECH_LOG_FREERTOS_STMHAL_INCLUDED
















