#ifndef CMSIS_OS_UTILS_H_INCLUDED
#define CMSIS_OS_UTILS_H_INCLUDED

#include "FreeRTOS.h"
#include "task.h"
#include "BanCopyMove.h"

namespace nowtech {

  void applicationMallocFailedHook(void);

  class OsUtil : public BanCopyMove {
    friend void applicationMallocFailedHook(void);

  private:
    static bool sMemoryAllocationFailed;

    static void setMemoryAllocationFailed() noexcept {
      sMemoryAllocationFailed = true;
    }

  public:
    static constexpr uint32_t cOneSecond = 1000u;

    static uint32_t msToRtosTick(uint32_t const aTimeMs) noexcept {
      return aTimeMs / portTICK_PERIOD_MS;
    }

    static uint32_t rtosTickToMs(uint32_t const aTick) noexcept {
      return aTick * portTICK_PERIOD_MS;
    }

    static uint32_t getUptimeMillis() noexcept {
      return rtosTickToMs(xTaskGetTickCountFromISR());
    }

    static void taskDelayMillis(uint32_t const aTimeMs) noexcept {
      vTaskDelay(msToRtosTick(aTimeMs));
    }

    static void infiniteWait() noexcept {
      while(true) {
        taskDelayMillis(cOneSecond);
      }
    }

    static bool isAnyMemoryAllocationFailed() noexcept {
      return sMemoryAllocationFailed;
    }
  };

}

#endif // CMSIS_UTILS_H_INCLUDED
