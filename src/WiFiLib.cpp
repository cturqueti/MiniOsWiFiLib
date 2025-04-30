#include "WiFiLib.h"

WiFiLib::WiFiLib(WiFiLog log)
{
    _log = log;
    _wifi.configLoaded = _loadCredentials(_wifi);
    // _wifi.configLoaded = false; // Descomente essa linha para habilitar a captivade
}
WiFiLib::~WiFiLib() {}

void WiFiLib::begin()
{

    if (_wifi.configLoaded)
    {
        if (isSsid())
        {
            // Start Wifi
            if (connectToWiFi(_wifi))
            {
                if (_log == WiFiLog::ENABLE)
                {
                    LOG_INFO("[WIFI] Wi-Fi started");
                }
                _loadSettings();
                if (!MDNS.begin(_wifi.mDns))
                {
                    if (_log == WiFiLog::ENABLE)
                    {
                        LOG_ERROR("[WIFI] MDNS nao iniciado com hostname: %s", _wifi.mDns.c_str());
                    }
                    ERRORS_LIST.addError(ErrorCode::MDNS_NOT_STARTED);
                }
                else
                {
                    if (_log == WiFiLog::ENABLE)
                    {
                        LOG_INFO("[WIFI] MDNS started with hostname: %s", _wifi.mDns.c_str());
                    }
                }
            }
        }
        else
        {
            if (_log == WiFiLog::ENABLE)
            {
                LOG_ERROR("[WIFI] SSID nao configurado");
            }
            ERRORS_LIST.addError(ErrorCode::SSID_NOT_FOUND);
        }
    }
    else
    {
        _captivePortal.begin();
    }
}

bool WiFiLib::isCredentials()
{
    if (!_wifi.configLoaded)
        return false;
    if (_wifi.ssid.length() == 0)
        return false;
    if (_wifi.password.length() == 0)
        return false;
    // Pode adicionar outras verificações como tamanho mínimo da senha, etc.
    return true;
}

bool WiFiLib::connectToWiFi(WiFiItems wifi)
{
    if (_log == WiFiLog::ENABLE)
    {
        LOG_DEBUG("[WIFI] Connecting to Wi-Fi...");
        LOG_DEBUG("[WIFI] SSID: %s", wifi.ssid.c_str());
    }
    WiFi.mode(WIFI_STA);
    if (!wifi.dhcp)
    {
        WiFi.config(wifi.ip, wifi.gateway, wifi.subnet);
    }

    WiFi.onEvent([this](WiFiEvent_t event, WiFiEventInfo_t info)
                 { this->WiFiEvent(event); });

    WiFi.setHostname(wifi.mDns.c_str());
    WiFi.begin(wifi.ssid.c_str(), wifi.password.c_str());

    int attempts = 0;
    const int maxAttempts = 50;
    if (_log == WiFiLog::ENABLE)
    {
        Log::enableNewline(false);
        LOG_INFO("[WIFI] Conectando.");
    }
    while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts)
    {
        vTaskDelay(pdMS_TO_TICKS(200));
        if (_log == WiFiLog::ENABLE)
        {
            Serial0.print(".");
        }
        attempts++;
    }
    if (_log == WiFiLog::ENABLE)
    {
        Serial0.println("");
        Log::enableNewline(true);
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        if (_log == WiFiLog::ENABLE)
        {
            LOG_INFO("[WIFI] Connected to Wi-Fi!");
            LOG_INFO("[WIFI] IP Address: %s", WiFi.localIP().toString().c_str());
        }
        return true;
    }
    else
    {
        if (_log == WiFiLog::ENABLE)
        {
            LOG_WARN("[WIFI] Failed to connect to Wi-Fi.");
        }
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF); // Reinicia completamente a interface Wi-Fi
        return false;
    }
}

void WiFiLib::WiFiEvent(WiFiEvent_t event)
{
    switch (event)
    {
    case ARDUINO_EVENT_WIFI_SCAN_DONE:
        if (_log == WiFiLog::ENABLE)
        {
            LOG_INFO("[WIFI] Wi-Fi scan done");
        }
        break;

    case ARDUINO_EVENT_WIFI_STA_START:
        if (_log == WiFiLog::ENABLE)
        {
            LOG_INFO("[WIFI] Wi-Fi STA started");
        }
        break;

    case ARDUINO_EVENT_WIFI_STA_STOP:
        if (_log == WiFiLog::ENABLE)
        {
            LOG_INFO("[WIFI] Wi-Fi STA stopped");
        }
        break;

    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
        if (_log == WiFiLog::ENABLE)
        {
            LOG_INFO("[WIFI] Wi-Fi STA connected");
        }
        _wifi.connectionStatus = ARDUINO_EVENT_WIFI_STA_CONNECTED;
        break;

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        if (_log == WiFiLog::ENABLE)
        {
            LOG_INFO("[WIFI] Wi-Fi STA disconnected");
        }
        _wifi.connectionStatus = ARDUINO_EVENT_WIFI_STA_DISCONNECTED;
        break;

    default:
        if (_log == WiFiLog::ENABLE)
        {
            LOG_INFO("[WIFI] Unhandled Wi-Fi event: %d", event);
        }
        break;
    }
}

bool WiFiLib::_beginCredentials()
{
    if (!_preferences.begin(nvs_namespace.data(), false))
    { // Use c_str() para String
        if (_log == WiFiLog::ENABLE)
        {
            LOG_ERROR("[WiFi] Error on load NVS");
        }
        ERRORS_LIST.addError(ErrorCode::NVS_BEGIN_ERROR);
        return false;
    }
    return true;
}

bool WiFiLib::_loadCredentials(WiFiItems &wifi)
{
    if (_beginCredentials())
    {
        wifi.ssid = _preferences.getString("ssid", "").c_str();
        wifi.password = _preferences.getString("password", "").c_str();
        _preferences.end();
        if (wifi.ssid != "")
        {
            if (_log == WiFiLog::ENABLE)
            {
                LOG_INFO("[WiFi] Loaded SSID: %s", wifi.ssid);
                LOG_DEBUG("[WiFi] Loaded Password: %s", wifi.password);
            }
            return true;
        }
        if (_log == WiFiLog::ENABLE)
        {
            LOG_ERROR("[WiFi] Don't have SSID");
        }
        ERRORS_LIST.addError(ErrorCode::SSID_NOT_FOUND);
    }

    return false;
}

bool WiFiLib::_loadSettings()
{
    if (_log == WiFiLog::ENABLE)
    {
        LOG_INFO("[WiFi] Loading settings");
    }

    JsonDocument doc;
    String configPath = String(configFolder.data()) + "/configWiFi.json";
    File configWiFi = LittleFS.open(configPath, "r");
    if (!configWiFi)
    {
        if (_log == WiFiLog::ENABLE)
        {
            LOG_ERROR("[WiFi] Failed to open file %s for reading", configPath.c_str());
        }
        ERRORS_LIST.addError(ErrorCode::FILE_NOT_FOUND);
        return false;
    }

    DeserializationError error = deserializeJson(doc, configWiFi);
    if (error)
    {
        if (_log == WiFiLog::ENABLE)
        {
            LOG_ERROR("[WiFi] JSON inválido: %s", error.c_str());
        }
        ERRORS_LIST.addError(ErrorCode::JSON_ERROR);
        return false;
    }
    _wifi.mDns = doc["mDns"].as<String>().c_str();
    _wifi.dhcp = doc["dhcp"].as<bool>();
    if (!_wifi.dhcp)
    {
        _wifi.ip = IPAddress(doc["ip"].as<String>().c_str());
        _wifi.gateway = IPAddress(doc["gateway"].as<String>().c_str());
        _wifi.subnet = IPAddress(doc["subnet"].as<String>().c_str());
    }
    else
    {
        _wifi.ip = WiFi.localIP();
        _wifi.gateway = WiFi.gatewayIP();
        _wifi.subnet = WiFi.subnetMask();
    }

    configWiFi.close();

    if (_log == WiFiLog::ENABLE)
    {
        LOG_DEBUG("[WiFi] mDns: %s", _wifi.mDns);
        LOG_DEBUG("[WiFi] dhcp: %s", _wifi.dhcp ? "true" : "false");
        LOG_DEBUG("[WiFi] ip: %s", _wifi.ip.toString().c_str());
        LOG_DEBUG("[WiFi] gateway: %s", _wifi.gateway.toString().c_str());
        LOG_DEBUG("[WiFi] subnet: %s", _wifi.subnet.toString().c_str());
    }
    return true;
}
