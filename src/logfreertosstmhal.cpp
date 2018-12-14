#include "logfreertosstmhal.h"

UART_HandleTypeDef* nowtech::LogFreeRtosStmHal::sSerialDescriptor;
std::atomic<bool> *nowtech::LogFreeRtosStmHal::sProgressFlag;
std::atomic<bool> *nowtech::LogFreeRtosStmHal::sRefreshNeeded;

extern "C" void logRefreshNeeded(TimerHandle_t) {
  nowtech::LogFreeRtosStmHal::refreshNeeded();
}


// USART3_TX   DMA1 Stream 3
// DMA1 stream3 global interrupt
// USART3 global interrupt
