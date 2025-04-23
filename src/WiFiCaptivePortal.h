#pragma once
// #include "WiFiStorageInterface.h"
#include "ErrorLib.h"
#include "WiFiItems.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
#include <LogLibrary.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>

#define CAPTIVE_PORTAL_SSID "ESP32-Captive-Portal"
#define CAPTIVE_PORTAL_DNS_PORT 53
#define CAPTIVE_PORTAL_TASK_STACK_SIZE 4096

class WiFiCaptivePortal {
  private:
    WebServer _server;
    Preferences _preferences;
    DNSServer _dnsServer;
    TaskHandle_t _serverTaskHandle;
    WiFiLog _log;
    IPAddress _ipAp;

    static constexpr std::string_view nvs_namespace = "wifi_config";
    static constexpr std::string_view captivePortalFolder = "/CaptivePortal";
    static constexpr std::string_view configFolder = "/Config";

  public:
    // WiFiCaptivePortal(WiFiStorageInterface &storage);

    WiFiCaptivePortal(WiFiLog log = WiFiLog::ENABLE);
    ~WiFiCaptivePortal();

    void begin();
    void end();
    bool isRunning() const;

  private:
    void _startAP();
    void _setupDNS();
    void _setupServer();
    void _handleClient();
    String _loadFromLittleFS(const String &path);
    String _getContentType(const String &filename);
    static void _serverTask(void *pvParameters);

    bool _beginCredentials();
    bool _saveCredentials(WiFiItems &credentials);
    bool _loadCredentials(WiFiItems &credentials);

    bool _saveSettings(WiFiItems &settings);

    void _handleRoot();
    void _logError(const __FlashStringHelper *message, const String &path, ErrorCode code);
    void _embedFileContent(String &html, const String &filePath, const String &prefix, const String &suffix,
                           const String &insertBefore);

    void _handleScanWifi();
    void _handleConfig();
    void _handleSaveWiFiSettings();

    bool _isRunning;

    // WiFiStorageInterface &_storage;
};