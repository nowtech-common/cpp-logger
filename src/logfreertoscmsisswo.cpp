#include "logfreertoscmsisswo.h"

std::atomic<bool> *nowtech::LogFreeRtosCmsisSwo::sRefreshNeeded;

/*extern "C" void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
  if(huart == &huart3) {
    nowtech::LogFreeRtosStmHal::transmitFinished();
  }
  else { // nothing to do
  }
}*/

extern "C" void logRefreshNeeded(TimerHandle_t) {
  nowtech::LogFreeRtosCmsisSwo::refreshNeeded();
}
