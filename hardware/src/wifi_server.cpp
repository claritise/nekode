#include "wifi_server.h"
#include "config.h"
#include "wifi_credentials.h"
#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>

namespace
{
  WiFiServer sServer(config::kTcpPort);
  WiFiClient sClient;
  char sBuf[32];
  uint8_t sBufLen = 0;

  void connectWiFi()
  {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.printf("WiFi connecting to %s", WIFI_SSID);

    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000)
    {
      delay(250);
      Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.printf("\nWiFi connected: %s\n", WiFi.localIP().toString().c_str());
      if (MDNS.begin(config::kMdnsHost))
      {
        Serial.printf("mDNS: %s.local\n", config::kMdnsHost);
      }
    }
    else
    {
      Serial.println("\nWiFi failed — serial only");
    }
  }
}

namespace wifiserver
{
  void init()
  {
    connectWiFi();
    sServer.begin();
  }

  bool poll(char *buf, uint8_t bufSize)
  {
    // Accept new client if none connected
    if (!sClient || !sClient.connected())
    {
      sClient = sServer.available();
      if (sClient)
      {
        sBufLen = 0;
      }
    }

    if (!sClient || !sClient.available())
      return false;

    while (sClient.available())
    {
      char c = sClient.read();
      if (c == '\n' || c == '\r')
      {
        if (sBufLen > 0)
        {
          sBufLen = (sBufLen < bufSize - 1) ? sBufLen : bufSize - 1;
          memcpy(buf, sBuf, sBufLen);
          buf[sBufLen] = '\0';
          sBufLen = 0;
          return true;
        }
      }
      else if (sBufLen < sizeof(sBuf) - 1)
      {
        sBuf[sBufLen++] = c;
      }
    }
    return false;
  }
}
