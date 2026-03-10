#pragma once
#include <cstdint>

namespace oled
{
  enum Anim : uint8_t {
    THINKING,    // idle between tasks
    READING,     // reading files
    TYPING,      // writing files
    RUNNING,     // bash command
    SEARCHING,   // grep/glob
    DONE,        // task complete
    ERROR,       // tool failed
    WAITING,     // needs input
    BOOT,        // session start
    SLEEP,       // session end
    BROWSING,    // web search
    SPAWNING,    // subagent start
    HERDING,     // subagent stop
    COMPACTING,  // context compaction
    PLANNING,    // organizing tasks
  };

  bool init();
  void setAnim(Anim a);
  void drawFrame();
}
