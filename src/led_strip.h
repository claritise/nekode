#pragma once
#include <cstdint>

namespace led
{
  enum Mode : uint8_t { OFF, RAINBOW, SOLID };

  void init();
  void setMode(Mode m);
  void tick();
}
