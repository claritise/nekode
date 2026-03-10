#pragma once
#include <cstdint>

namespace wifiserver
{
  void init();
  // Returns true and writes into buf (null-terminated) if a full command was received.
  bool poll(char *buf, uint8_t bufSize);
}
