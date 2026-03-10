#pragma once
#include <cstdint>

namespace mic
{
  void init();

  // Read a block of samples and return the RMS volume (0-255).
  uint8_t readVolume();
}
