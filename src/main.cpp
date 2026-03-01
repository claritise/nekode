#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "esp32-hal-rmt.h"

namespace
{
  // OLED config
  constexpr uint8_t kOledSda = 16;
  constexpr uint8_t kOledScl = 17;
  constexpr uint16_t kScreenWidth = 128;
  constexpr uint16_t kScreenHeight = 64;
  constexpr uint8_t kOledAddr = 0x3C;

  Adafruit_SSD1306 gDisplay(kScreenWidth, kScreenHeight, &Wire, -1);

  // LED strip config
  constexpr uint8_t kLedPin = 11;
  constexpr uint16_t kLedCount = 20;
  constexpr uint16_t kFrameDelayMs = 30;
  constexpr uint8_t kBrightness = 60;

  // RMT state
  rmt_obj_t *gRmt = nullptr;
  uint8_t gPixels[kLedCount * 3];
  uint16_t gHue = 0;
  uint32_t gLastFrameMs = 0;

  // Convert HSV (h: 0-1535, s: 0-255, v: 0-255) to RGB.
  void hsvToRgb(uint16_t h, uint8_t s, uint8_t v, uint8_t &r, uint8_t &g, uint8_t &b)
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

  void showPixels()
  {
    if (gRmt == nullptr)
    {
      return;
    }
    constexpr uint32_t kNumBits = kLedCount * 24;
    rmt_data_t rmtData[kNumBits];

    uint32_t idx = 0;
    for (uint16_t p = 0; p < kLedCount; p++)
    {
      for (int byteIdx = 0; byteIdx < 3; byteIdx++)
      {
        uint8_t val = gPixels[p * 3 + byteIdx];
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
    rmtWriteBlocking(gRmt, rmtData, kNumBits);
  }

  // APA106 uses RGB byte order.
  void setPixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b)
  {
    uint16_t offset = index * 3;
    gPixels[offset + 0] = r;
    gPixels[offset + 1] = g;
    gPixels[offset + 2] = b;
  }

  void clear()
  {
    memset(gPixels, 0, sizeof(gPixels));
  }

  void renderFrame()
  {
    for (uint16_t i = 0; i < kLedCount; i++)
    {
      // Spread the rainbow across the strip, offset by gHue for animation.
      uint16_t pixelHue = gHue + (i * 1536 / kLedCount);
      uint8_t r, g, b;
      hsvToRgb(pixelHue, 255, kBrightness, r, g, b);
      setPixel(i, r, g, b);
    }
    showPixels();
    gHue -= 16;
  }
}

void setup()
{
  Serial.begin(115200);
  delay(500);
  Serial.println("Booting...");

  // Init OLED on custom I2C pins
  Wire.begin(kOledSda, kOledScl);
  if (!gDisplay.begin(SSD1306_SWITCHCAPVCC, kOledAddr))
  {
    Serial.println("OLED init failed");
  }
  else
  {
    Serial.println("OLED ready: 128x64 on GPIO 16/17");
    gDisplay.clearDisplay();
    gDisplay.setTextSize(1);
    gDisplay.setTextColor(SSD1306_WHITE);
    gDisplay.setCursor(0, 0);
    gDisplay.println("claude-status");
    gDisplay.println();
    gDisplay.println("OLED: 128x64 I2C");
    gDisplay.println("LEDs: 20x APA106");
    gDisplay.println("GPIO 11 (RMT)");
    gDisplay.display();
  }

  // Init LED strip
  gRmt = rmtInit(kLedPin, RMT_TX_MODE, RMT_MEM_64);
  if (gRmt == nullptr)
  {
    Serial.println("RMT init failed");
  }
  else
  {
    Serial.println("LED strip ready: 20x APA106 on GPIO 11");
    rmtSetTick(gRmt, 100);
    clear();
    showPixels();
  }
}

void loop()
{
  uint32_t now = millis();
  if ((uint32_t)(now - gLastFrameMs) >= kFrameDelayMs)
  {
    gLastFrameMs = now;
    renderFrame();
  }
}
