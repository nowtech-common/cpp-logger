#include "cmsis_os_utils.h"

bool nowtech::OsUtil::sMemoryAllocationFailed = false;

void nowtech::applicationMallocFailedHook(void) {
  nowtech::OsUtil::setMemoryAllocationFailed();
}

extern "C" void vApplicationMallocFailedHook(void) {
  nowtech::applicationMallocFailedHook();
}
