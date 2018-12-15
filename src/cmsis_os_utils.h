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
