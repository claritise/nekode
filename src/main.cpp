#include <Arduino.h>
#include "config.h"
#include "led_strip.h"
#include "oled.h"

namespace
{
  bool gOledReady = false;
  uint32_t gLastLedMs = 0;
  uint32_t gLastNekoMs = 0;
  char gCmdBuf[32];
  uint8_t gCmdLen = 0;

  struct CmdEntry
  {
    const char *name;
    oled::Anim anim;
    led::Mode ledMode;
  };

  constexpr CmdEntry kCommands[] = {
    { "thinking",   oled::THINKING,   led::SOLID   },
    { "reading",    oled::READING,    led::RAINBOW },
    { "typing",     oled::TYPING,     led::RAINBOW },
    { "running",    oled::RUNNING,    led::RAINBOW },
    { "searching",  oled::SEARCHING,  led::RAINBOW },
    { "done",       oled::DONE,       led::RAINBOW },
    { "error",      oled::ERROR,      led::RAINBOW },
    { "waiting",    oled::WAITING,    led::OFF     },
    { "boot",       oled::BOOT,       led::RAINBOW },
    { "sleep",      oled::SLEEP,      led::OFF     },
    { "browsing",   oled::BROWSING,   led::RAINBOW },
    { "spawning",   oled::SPAWNING,   led::RAINBOW },
    { "herding",    oled::HERDING,    led::RAINBOW },
    { "compacting", oled::COMPACTING, led::RAINBOW },
    { "planning",   oled::PLANNING,   led::RAINBOW },
  };

  void handleCommand(const char *cmd)
  {
    for (const auto &entry : kCommands)
    {
      if (strcmp(cmd, entry.name) == 0)
      {
        oled::setAnim(entry.anim);
        led::setMode(entry.ledMode);
        Serial.print("anim="); Serial.println(cmd);
        return;
      }
    }
    Serial.print("unknown: "); Serial.println(cmd);
  }

  void pollSerial()
  {
    while (Serial.available())
    {
      char c = Serial.read();
      if (c == '\n' || c == '\r')
      {
        if (gCmdLen > 0)
        {
          gCmdBuf[gCmdLen] = '\0';
          handleCommand(gCmdBuf);
          gCmdLen = 0;
        }
      }
      else if (gCmdLen < sizeof(gCmdBuf) - 1)
      {
        gCmdBuf[gCmdLen++] = c;
      }
    }
  }
}

void setup()
{
  Serial.begin(115200);
  delay(500);
  Serial.println("Booting...");

  gOledReady = oled::init();
  if (gOledReady)
  {
    oled::drawFrame();
  }

  led::init();
}

void loop()
{
  pollSerial();

  uint32_t now = millis();
  if ((uint32_t)(now - gLastLedMs) >= config::kLedFrameMs)
  {
    gLastLedMs = now;
    led::tick();
  }
  if (gOledReady && (uint32_t)(now - gLastNekoMs) >= config::kNekoFrameMs)
  {
    gLastNekoMs = now;
    oled::drawFrame();
  }
}
