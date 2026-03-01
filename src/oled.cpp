#include "oled.h"
#include "config.h"
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

namespace
{
  Adafruit_SSD1306 sDisplay(config::kScreenWidth, config::kScreenHeight, &Wire, -1);

  // --- Sprite data (16x16, PROGMEM) ---
  //
  // Visual reference (eyes open):
  //   ..#.......#.....  ear tips
  //   .##......##.....  ears
  //   .##......##.....  ears (tall)
  //   .###....###.....  ears widen
  //   .###########....  head
  //   .###########....  head
  //   .##..###..##....  eyes
  //   .###########..#.  chin + tail tip
  //   ..#########..#..  neck + tail
  //   ..##########.#..  body + tail
  //   ..##########....  body
  //   ..##########....  body
  //   ..####.####.....  legs
  //   ..##....##......  feet

  // Eyes open (normal)
  const uint8_t PROGMEM kNekoOpen[] = {
    0x20, 0x40, // ..#.......#.....
    0x60, 0x60, // .##......##.....
    0x60, 0x60, // .##......##.....
    0x70, 0x70, // .###....###.....
    0x7F, 0xF0, // .###########....
    0x7F, 0xF0, // .###########....
    0x67, 0x30, // .##..###..##....
    0x7F, 0xF2, // .###########..#.
    0x3F, 0xE4, // ..#########..#..
    0x3F, 0xF4, // ..##########.#..
    0x3F, 0xF0, // ..##########....
    0x3F, 0xF0, // ..##########....
    0x3D, 0xE0, // ..####.####.....
    0x30, 0xC0, // ..##....##......
    0x00, 0x00,
    0x00, 0x00,
  };

  // Eyes closed (blink / sleep)
  const uint8_t PROGMEM kNekoClosed[] = {
    0x20, 0x40,
    0x60, 0x60,
    0x60, 0x60,
    0x70, 0x70,
    0x7F, 0xF0,
    0x7F, 0xF0,
    0x6F, 0x70, // .##.####.###....  (squint)
    0x7F, 0xF2,
    0x3F, 0xE4,
    0x3F, 0xF4,
    0x3F, 0xF0,
    0x3F, 0xF0,
    0x3D, 0xE0,
    0x30, 0xC0,
    0x00, 0x00,
    0x00, 0x00,
  };

  // 5x5 heart
  const uint8_t PROGMEM kHeart[] = {
    0x50, // .#.#.
    0xF8, // #####
    0xF8, // #####
    0x70, // .###.
    0x20, // ..#..
  };

  // --- Animation state ---

  enum NekoState { IDLE, HAPPY, SLEEP, NUM_STATES };
  NekoState sState = IDLE;
  uint16_t sFrame = 0;
  uint32_t sStateStartMs = 0;

  constexpr uint32_t kStateDurations[] = { 10000, 4000, 6000 };
  constexpr uint8_t kSpriteW = 16;
  constexpr uint8_t kSpriteH = 16;
  constexpr uint8_t kScale = 3;
  constexpr int16_t kCatX = (config::kScreenWidth - kSpriteW * kScale) / 2;
  constexpr int16_t kCatY = 4;

  // Draw a 1-bit bitmap scaled up by an integer factor.
  void drawScaledBitmap(int16_t x, int16_t y, const uint8_t *bmp,
                        uint8_t w, uint8_t h, uint8_t scale)
  {
    uint8_t bytesPerRow = (w + 7) / 8;
    for (uint8_t j = 0; j < h; j++)
    {
      for (uint8_t i = 0; i < w; i++)
      {
        if (pgm_read_byte(&bmp[j * bytesPerRow + i / 8]) & (0x80 >> (i & 7)))
        {
          sDisplay.fillRect(x + i * scale, y + j * scale,
                            scale, scale, SSD1306_WHITE);
        }
      }
    }
  }

  void advanceState()
  {
    uint32_t now = millis();
    if (now - sStateStartMs >= kStateDurations[sState])
    {
      sState = static_cast<NekoState>((sState + 1) % NUM_STATES);
      sStateStartMs = now;
      sFrame = 0;
    }
  }

  void drawIdle()
  {
    int16_t bob = ((sFrame / 3) % 2) ? -kScale : 0;
    bool blink = (sFrame % 24 < 2);
    drawScaledBitmap(kCatX, kCatY + bob,
                     blink ? kNekoClosed : kNekoOpen,
                     kSpriteW, kSpriteH, kScale);

    sDisplay.setTextSize(1);
    sDisplay.setTextColor(SSD1306_WHITE);
    sDisplay.setCursor(40, 56);
    sDisplay.print("~ nyan ~");
  }

  void drawHappy()
  {
    int16_t bounce = ((sFrame / 2) % 2) ? -(kScale * 2) : 0;
    bool atTop = (bounce != 0);
    drawScaledBitmap(kCatX, kCatY + bounce,
                     atTop ? kNekoClosed : kNekoOpen,
                     kSpriteW, kSpriteH, kScale);

    // Floating heart
    int16_t heartY = 24 - (int16_t)(sFrame % 16);
    int16_t heartX = kCatX + kSpriteW * kScale + 4;
    if (heartY >= -10)
    {
      drawScaledBitmap(heartX, heartY, kHeart, 5, 5, 2);
    }

    sDisplay.setTextSize(1);
    sDisplay.setTextColor(SSD1306_WHITE);
    sDisplay.setCursor(37, 56);
    sDisplay.print("happy :3");
  }

  void drawSleep()
  {
    drawScaledBitmap(kCatX, kCatY + kScale, kNekoClosed,
                     kSpriteW, kSpriteH, kScale);

    // Animated zzz (appear one by one, then reset)
    sDisplay.setTextSize(1);
    sDisplay.setTextColor(SSD1306_WHITE);
    int16_t zx = kCatX + kSpriteW * kScale + 4;
    uint8_t zStep = (sFrame / 6) % 4;
    if (zStep >= 1) { sDisplay.setCursor(zx, 28);      sDisplay.print("z"); }
    if (zStep >= 2) { sDisplay.setCursor(zx + 7, 18);  sDisplay.print("Z"); }
    if (zStep >= 3) { sDisplay.setCursor(zx + 14, 8);  sDisplay.print("z"); }

    sDisplay.setCursor(31, 56);
    sDisplay.print("sleeping...");
  }
}

namespace oled
{
  bool init()
  {
    Wire.begin(config::kOledSda, config::kOledScl);
    if (!sDisplay.begin(SSD1306_SWITCHCAPVCC, config::kOledAddr))
    {
      Serial.println("OLED init failed");
      return false;
    }
    Serial.println("OLED ready: 128x64 on GPIO 16/17");
    sStateStartMs = millis();
    return true;
  }

  void drawNekoFrame()
  {
    advanceState();
    sDisplay.clearDisplay();

    switch (sState)
    {
    case IDLE:  drawIdle();  break;
    case HAPPY: drawHappy(); break;
    case SLEEP: drawSleep(); break;
    default: break;
    }

    sDisplay.display();
    sFrame++;
  }
}
