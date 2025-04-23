#ifndef WIFILIB_H
#define WIFILIB_H

#include "ErrorLib.h"
#include "WiFiCaptivePortal.h"
#include "WiFiItems.h"
#include <Arduino.h>
#include <ESPmDNS.h>
#include <LogLibrary.h>
#include <Preferences.h>
#include <WiFi.h>
#include <vector>

class WiFiLib
{
public:
  static constexpr std::string_view nvs_namespace = "wifi_config";

  static WiFiLib &getInstance(WiFiLog log = WiFiLog::ENABLE)
  {
    static WiFiLib instance(log);
    return instance;
  }
  ~WiFiLib();

  void begin();

  bool isCredentials();

  inline bool isConfigLoaded() { return _wifi.configLoaded; }
  inline bool isDhcp() { return _wifi.dhcp; }
  inline bool isSsid() { return _wifi.ssid.length() > 0; }

private:
  WiFiLib(WiFiLog log);                         // Construtor privado
  WiFiLib(const WiFiLib &) = delete;            // Previne cópia
  WiFiLib &operator=(const WiFiLib &) = delete; // Previne atribuição

  void connectToWiFi(WiFiItems wifi);
  void WiFiEvent(WiFiEvent_t event);
  bool _beginCredentials();
  bool _loadCredentials(WiFiItems &wifi);

  static const char *TAG;
  WiFiItems _wifi;
  WiFiLog _log;
  WiFiCaptivePortal _captivePortal;
  mutable Preferences _preferences;
};

#endif // WiFiLib_H