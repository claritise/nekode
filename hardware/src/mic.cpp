#include "mic.h"
#include "config.h"
#include <Arduino.h>
#include <driver/i2s.h>

namespace
{
  constexpr i2s_port_t kI2sPort = I2S_NUM_0;
  int32_t sSampleBuf[config::kMicBlockSize];
}

namespace mic
{
  void init()
  {
    i2s_config_t cfg = {};
    cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX);
    cfg.sample_rate = config::kMicSampleRate;
    cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT;
    cfg.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
    cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    cfg.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
    cfg.dma_buf_count = 4;
    cfg.dma_buf_len = config::kMicBlockSize;
    cfg.use_apll = false;

    if (i2s_driver_install(kI2sPort, &cfg, 0, nullptr) != ESP_OK)
    {
      Serial.println("I2S driver install failed");
      return;
    }

    i2s_pin_config_t pins = {};
    pins.bck_io_num = config::kMicSck;
    pins.ws_io_num = config::kMicWs;
    pins.data_in_num = config::kMicSd;
    pins.data_out_num = I2S_PIN_NO_CHANGE;

    if (i2s_set_pin(kI2sPort, &pins) != ESP_OK)
    {
      Serial.println("I2S set pin failed");
      return;
    }

    Serial.println("Mic ready: INMP441 on GPIO 7/15/5");
  }

  uint8_t readVolume()
  {
    size_t bytesRead = 0;
    esp_err_t err = i2s_read(kI2sPort, sSampleBuf, sizeof(sSampleBuf),
                             &bytesRead, portMAX_DELAY);
    if (err != ESP_OK || bytesRead == 0)
    {
      return 0;
    }

    uint32_t samplesRead = bytesRead / sizeof(int32_t);

    // Compute RMS of the high 16 bits (INMP441 data is in upper bits).
    uint64_t sumSq = 0;
    for (uint32_t i = 0; i < samplesRead; i++)
    {
      int16_t sample = (int16_t)(sSampleBuf[i] >> 16);
      sumSq += (int32_t)sample * sample;
    }
    uint32_t rms = sqrt((double)sumSq / samplesRead);

    // Scale to 0-255. ~2000 RMS is moderately loud for INMP441.
    uint16_t vol = rms / 8;
    if (vol > 255) vol = 255;
    return (uint8_t)vol;
  }
}
