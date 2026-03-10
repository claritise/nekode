#pragma once
#include <cstdint>

namespace config
{
  // LED strip (APA106 on RMT)
  constexpr uint8_t kLedPin = 11;
  constexpr uint16_t kLedCount = 6;
  constexpr uint8_t kLedBrightness = 13;
  constexpr uint16_t kLedFrameMs = 30;

  // OLED display (SSD1306 over I2C)
  constexpr uint8_t kOledSda = 16;
  constexpr uint8_t kOledScl = 17;
  constexpr uint8_t kOledAddr = 0x3C;
  constexpr uint16_t kScreenWidth = 128;
  constexpr uint16_t kScreenHeight = 64;
  constexpr uint16_t kNekoFrameMs = 150;

  // WiFi TCP server
  constexpr uint16_t kTcpPort = 23;
  constexpr const char *kMdnsHost = "nekode";

  // Microphone (INMP441 over I2S)
  constexpr uint8_t kMicSck = 7;
  constexpr uint8_t kMicWs = 15;
  constexpr uint8_t kMicSd = 5;
  constexpr uint16_t kMicSampleRate = 16000;
  constexpr uint16_t kMicBlockSize = 256;
}
