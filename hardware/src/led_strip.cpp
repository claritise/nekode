#include "led_strip.h"
#include "config.h"
#include <Arduino.h>
#include "esp32-hal-rmt.h"
#include <cstring>

namespace
{
  rmt_obj_t *sRmt = nullptr;
  uint8_t sPixels[config::kLedCount * 3];
  led::Mode sMode = led::OFF;

  void setPixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b)
  {
    if (index >= config::kLedCount) return;
    uint16_t offset = index * 3;
    sPixels[offset + 0] = r;
    sPixels[offset + 1] = g;
    sPixels[offset + 2] = b;
  }

  void show()
  {
    if (sRmt == nullptr) return;

    constexpr uint32_t kNumBits = config::kLedCount * 24;
    rmt_data_t rmtData[kNumBits];

    uint32_t idx = 0;
    for (uint16_t p = 0; p < config::kLedCount; p++)
    {
      for (int byteIdx = 0; byteIdx < 3; byteIdx++)
      {
        uint8_t val = sPixels[p * 3 + byteIdx];
        for (int bit = 7; bit >= 0; bit--)
        {
          if (val & (1 << bit))
          {
            rmtData[idx].level0 = 1;
            rmtData[idx].duration0 = 8;
            rmtData[idx].level1 = 0;
            rmtData[idx].duration1 = 4;
          }
          else
          {
            rmtData[idx].level0 = 1;
            rmtData[idx].duration0 = 4;
            rmtData[idx].level1 = 0;
            rmtData[idx].duration1 = 8;
          }
          idx++;
        }
      }
    }
    rmtWriteBlocking(sRmt, rmtData, kNumBits);
  }

  // Convert HSV (h: 0-1535, s: 0-255, v: 0-255) to RGB.
  void hsvToRgb(uint16_t h, uint8_t s, uint8_t v,
                uint8_t &r, uint8_t &g, uint8_t &b)
  {
    h = h % 1536;
    uint8_t region = h / 256;
    uint8_t remainder = h % 256;
    uint8_t p = (uint16_t(v) * (255 - s)) >> 8;
    uint8_t q = (uint16_t(v) * (255 - ((uint16_t(s) * remainder) >> 8))) >> 8;
    uint8_t t = (uint16_t(v) * (255 - ((uint16_t(s) * (255 - remainder)) >> 8))) >> 8;

    switch (region)
    {
    case 0: r = v; g = t; b = p; break;
    case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break;
    case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break;
    default: r = v; g = p; b = q; break;
    }
  }
}

namespace led
{
  void init()
  {
    sRmt = rmtInit(config::kLedPin, RMT_TX_MODE, RMT_MEM_64);
    if (sRmt == nullptr)
    {
      Serial.println("RMT init failed");
      return;
    }
    rmtSetTick(sRmt, 100);
    Serial.println("LED strip ready: 6x APA106 on GPIO 11");
    memset(sPixels, 0, sizeof(sPixels));
    show();
  }

  void setMode(Mode m)
  {
    sMode = m;
  }

  void tick()
  {
    if (sMode == OFF)
    {
      memset(sPixels, 0, sizeof(sPixels));
      show();
      return;
    }

    if (sMode == SOLID)
    {
      for (uint16_t i = 0; i < config::kLedCount; i++)
        setPixel(i, config::kLedBrightness, config::kLedBrightness, config::kLedBrightness);
      show();
      return;
    }

    // RAINBOW
    uint16_t hue = -(uint16_t)(millis() / 2);
    for (uint16_t i = 0; i < config::kLedCount; i++)
    {
      uint16_t pixelHue = hue + (i * 1536 / config::kLedCount);
      uint8_t r, g, b;
      hsvToRgb(pixelHue, 255, config::kLedBrightness, r, g, b);
      setPixel(i, r, g, b);
    }
    show();
  }
}
