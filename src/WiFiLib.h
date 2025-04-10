#ifndef WIFILIB_H
#define WIFILIB_H
#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <LogLibrary.h>
#include <vector>

// #include "Utils.h"
#include "WiFi.h"
// #include "WiFiAP.h"

struct WiFiItems
{
    std::string ssid;
    std::string password;
    bool dhcpFlag;
    std::vector<uint8_t> ip, gateway, subnet;
    bool configLoaded;
    int connectionStatus;
    int power;
    WiFiItems() : ip(4),
                  gateway(4),
                  subnet(4),
                  configLoaded(false),
                  connectionStatus(0),
                  power(0)
    {
    }
    void reset()
    {
        ssid.clear();
        password.clear();
        configLoaded = false;
        connectionStatus = 0;
    }
};

class WiFiLib
{
public:
    enum class WiFiLog
    {
        DISABLE = 0,
        ENABLE
    };
    static WiFiLib &getInstance()
    {
        static WiFiLib instance;
        return instance;
    }

    void begin(WiFiItems wifi);
    void begin(WiFiItems wifi, WiFiLog log);

    WiFiItems loadConfig(std::string fileAddress);

    inline bool isConfigLoaded() { return _wifi.configLoaded; }
    inline bool isDhcp() { return _wifi.dhcpFlag; }
    inline bool isSsid() { return _wifi.ssid.size() > 0; }

private:
    WiFiLib();                                    // Construtor privado
    WiFiLib(const WiFiLib &) = delete;            // Previne cópia
    WiFiLib &operator=(const WiFiLib &) = delete; // Previne atribuição

    void connectToWiFi(WiFiItems wifi);
    void WiFiEvent(WiFiEvent_t event);

    static const char *TAG;
    WiFiItems _wifi;
    WiFiLog _log;
};

#endif // WiFiLib_H