#include "oled.h"
#include "config.h"
#include "THINKING.h"
#include "READING.h"
#include "TYPING.h"
#include "RUNNING.h"
#include "SEARCHING.h"
#include "DONE.h"
#include "ERROR.h"
#include "WAITING.h"
#include "BOOT.h"
#include "SLEEP.h"
#include "BROWSING.h"
#include "SPAWNING.h"
#include "HERDING.h"
#include "COMPACTING.h"
#include "PLANNING.h"
#include "led_strip.h"
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

namespace
{
  Adafruit_SSD1306 sDisplay(config::kScreenWidth, config::kScreenHeight, &Wire, -1);

  // --- Animation state ---

  oled::Anim sAnim = oled::THINKING;
  uint16_t sFrame = 0;
  constexpr int16_t kSpriteX = (config::kScreenWidth - kThinkingW) / 2;
  constexpr int16_t kSpriteY = 0;

  struct AnimData
  {
    const uint8_t * const *frames;
    uint8_t count;
    uint8_t w;
    uint8_t h;
  };

  // Order must match oled::Anim enum
  constexpr AnimData kAnims[] = {
    { kThinkingFrames,   kThinkingFrameCount,   kThinkingW,   kThinkingH   },
    { kReadingFrames,    kReadingFrameCount,    kReadingW,    kReadingH    },
    { kTypingFrames,     kTypingFrameCount,     kTypingW,     kTypingH     },
    { kRunningFrames,    kRunningFrameCount,    kRunningW,    kRunningH    },
    { kSearchingFrames,  kSearchingFrameCount,  kSearchingW,  kSearchingH  },
    { kDoneFrames,       kDoneFrameCount,       kDoneW,       kDoneH       },
    { kErrorFrames,      kErrorFrameCount,      kErrorW,      kErrorH      },
    { kWaitingFrames,    kWaitingFrameCount,    kWaitingW,    kWaitingH    },
    { kBootFrames,       kBootFrameCount,       kBootW,       kBootH       },
    { kSleepFrames,      kSleepFrameCount,      kSleepW,      kSleepH      },
    { kBrowsingFrames,   kBrowsingFrameCount,   kBrowsingW,   kBrowsingH   },
    { kSpawningFrames,   kSpawningFrameCount,   kSpawningW,   kSpawningH   },
    { kHerdingFrames,    kHerdingFrameCount,    kHerdingW,    kHerdingH    },
    { kCompactingFrames, kCompactingFrameCount, kCompactingW, kCompactingH },
    { kPlanningFrames,   kPlanningFrameCount,   kPlanningW,   kPlanningH   },
  };

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
    return true;
  }

  void setAnim(Anim a)
  {
    if (a == sAnim) return;
    sAnim = a;
    sFrame = 0;
  }

  void drawFrame()
  {
    sDisplay.clearDisplay();

    const AnimData &ad = kAnims[sAnim];

    const uint8_t *frame = (const uint8_t *)pgm_read_ptr(&ad.frames[sFrame % ad.count]);
    sDisplay.drawBitmap(kSpriteX, kSpriteY, frame, ad.w, ad.h, SSD1306_WHITE);

    sDisplay.display();
    sFrame++;

    // One-shot: DONE plays once then transitions to WAITING
    if (sAnim == DONE && sFrame >= ad.count)
    {
      sAnim = WAITING;
      sFrame = 0;
      led::setMode(led::OFF);
    }
  }
}
