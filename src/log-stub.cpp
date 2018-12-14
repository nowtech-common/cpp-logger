#include "log.h"
#include "logutil.h"

void nowtech::Chunk::push(char const mChar) noexcept {
}

constexpr nowtech::LogFormat nowtech::LogConfig::cDefault;
constexpr nowtech::LogFormat nowtech::LogConfig::cNone;
constexpr nowtech::LogFormat nowtech::LogConfig::cB8;
constexpr nowtech::LogFormat nowtech::LogConfig::cB16;
constexpr nowtech::LogFormat nowtech::LogConfig::cB24;
constexpr nowtech::LogFormat nowtech::LogConfig::cB32;
constexpr nowtech::LogFormat nowtech::LogConfig::cD2;
constexpr nowtech::LogFormat nowtech::LogConfig::cD3;
constexpr nowtech::LogFormat nowtech::LogConfig::cD4;
constexpr nowtech::LogFormat nowtech::LogConfig::cD5;
constexpr nowtech::LogFormat nowtech::LogConfig::cD6;
constexpr nowtech::LogFormat nowtech::LogConfig::cD7;
constexpr nowtech::LogFormat nowtech::LogConfig::cD8;
constexpr nowtech::LogFormat nowtech::LogConfig::cX2;
constexpr nowtech::LogFormat nowtech::LogConfig::cX4;
constexpr nowtech::LogFormat nowtech::LogConfig::cX6;
constexpr nowtech::LogFormat nowtech::LogConfig::cX8;

constexpr char nowtech::Log::cDigit2char[16];

nowtech::Log *nowtech::Log::sInstance;

void nowtech::Log::transmitterThreadFunction() noexcept
{
}

nowtech::Log::Log(LogOsInterface &aOsInterface, LogConfig const &aConfig) noexcept
  : mOsInterface(aOsInterface)
  , mConfig(aConfig)
  , mChunkSize(aConfig.mChunkSize) {
  sInstance = this;
}

void nowtech::Log::doRegisterCurrentTask() noexcept {
}

nowtech::TaskIdType nowtech::Log::getCurrentTaskId() const noexcept {
  return Chunk::cInvalidTaskId;
}

// In the stub this will be implemented to return constant false
void nowtech::Log::startSend(Chunk &aChunk) noexcept {
}

void nowtech::Log::append(Chunk &aChunk, double const aValue, uint8_t const aDigitsNeeded) noexcept {
}
