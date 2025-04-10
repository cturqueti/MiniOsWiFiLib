#include "WiFiLib.h"

const char *WiFiLib::TAG = "WiFiLib";

WiFiLib::WiFiLib() : _log(WiFiLog::DISABLE) {}

void WiFiLib::begin(WiFiItems wifi)
{
    if (wifi.configLoaded)
    {
        if (isSsid())
        {
            // Start Wifi
            connectToWiFi(wifi);
            // modify WiFiIcon Wifiservice
        }
        else
        {
            // startAP
        }
    }
}

void WiFiLib::begin(WiFiItems wifi, WiFiLog log)
{
    _log = log;
    if (wifi.configLoaded)
    {
        if (isSsid())
        {
            // Start Wifi
            connectToWiFi(wifi);
            // modify WiFiIcon Wifiservice
        }
        else
        {
            // startAP
        }
    }
}

WiFiItems WiFiLib::loadConfig(std::string fileAddress)
{
    File file = LittleFS.open(fileAddress.c_str(), "r");
    if (!file)
    {
        if (_log == WiFiLog::ENABLE)
        {
            LOG_ERROR("[WIFI] Erro ao abrir %s", fileAddress.c_str());
        }

        return WiFiItems(); // Defina um código de erro apropriado
    }

    size_t size = file.size();
    if (size == 0)
    {
        if (_log == WiFiLog::ENABLE)
        {
            LOG_ERROR("[WIFI] Arquivo vazio: %s", fileAddress.c_str());
        }
        _wifi.reset();
        return WiFiItems(); // Retorna nullptr se o arquivo estiver vazio
    }

    // Aloca o buffer com o tamanho do arquivo
    std::unique_ptr<char[]> buf(new char[size + 1]); // +1 para garantir espaço para o terminador '\0'
    file.readBytes(buf.get(), size);
    buf[size] = '\0'; // Adiciona o terminador de string

    file.close();
    JsonDocument doc; // Ajuste o tamanho conforme necessário
    DeserializationError error = deserializeJson(doc, buf.get());

    if (error)
    {
        if (_log == WiFiLog::ENABLE)
        {
            LOG_ERROR("[WIFI] Falha ao analisar JSON: %s", error.c_str());
        }
        _wifi.reset();
        return WiFiItems(); // Defina um código de erro apropriado
    }

    if (doc["ssid"].is<const char *>())
    {
        _wifi.ssid = doc["ssid"].as<String>().c_str();
        if (_wifi.ssid == "")
        {
            _wifi.reset();
            return WiFiItems();
        }
    }
    else
    {
        _wifi.reset();
        return WiFiItems();
    }
    if (doc["password"].is<const char *>())
    {
        _wifi.password = doc["password"].as<String>().c_str();
    }
    else
    {
        _wifi.reset();
        return WiFiItems();
    }

    _wifi.dhcpFlag = doc["dhcp"] | false;

    if (!_wifi.dhcpFlag)
    {
        if (_log == WiFiLog::ENABLE)
        {
            LOG_INFO("[WIFI] Rading fixed IP configurations");
        }
        if (doc["ip"].is<JsonArray>() && doc["ip"].size() == 4)

        {
            for (int i = 0; i < 4; i++)
            {
                _wifi.ip[i] = doc["ip"][i];
            }
            if (_wifi.ip[0] < 0 || _wifi.ip[0] > 255)
            {
                if (_log == WiFiLog::ENABLE)
                {
                    LOG_ERROR("[WIFI] IP address invalid");
                }
                _wifi.reset();
                return WiFiItems();
            }
        }
        else
        {
            if (_log == WiFiLog::ENABLE)
            {
                LOG_ERROR("[WIFI] Chave 'ip' ausente ou inválida.");
            }
            _wifi.reset();
            return WiFiItems(); // Defina um código de erro apropriado
        }

        if (doc["gateway"].is<JsonArray>() && doc["gateway"].size() == 4)
        {
            for (int i = 0; i < 4; i++)
            {
                _wifi.gateway[i] = doc["gateway"][i];
            }
        }
        else
        {
            if (_log == WiFiLog::ENABLE)
            {
                LOG_ERROR("[WIFI] Chave 'gateway' ausente ou inválida.");
            }
            _wifi.reset();
            return WiFiItems(); // Defina um código de erro apropriado
        }

        if (doc["subnet"].is<JsonArray>() && doc["subnet"].size() == 4)
        {
            for (int i = 0; i < 4; i++)
            {
                _wifi.subnet[i] = doc["subnet"][i];
            }
            if (_wifi.subnet[0] < 0 || _wifi.subnet[0] > 255)
            {
                if (_log == WiFiLog::ENABLE)
                {
                    LOG_ERROR("[WIFI] Subnet Mask Invalid");
                }
                _wifi.reset();
                return WiFiItems();
            }
        }
        else
        {
            if (_log == WiFiLog::ENABLE)
            {
                LOG_ERROR("[WIFI] Chave 'subnet' ausente ou inválida.");
            }
            _wifi.reset();
            return WiFiItems(); // Defina um código de erro apropriado
        }
    }
    else
    {
        if (_log == WiFiLog::ENABLE)
        {
            LOG_INFO("[WIFI] Selected DHCP");
        }
    }
    _wifi.configLoaded = true;
    if (_log == WiFiLog::ENABLE)
    {
        LOG_INFO("[WIFI] Configurações carregadas com sucesso.");
    }

    return _wifi;
}

void WiFiLib::connectToWiFi(WiFiItems wifi)
{
    if (_log == WiFiLog::ENABLE)
    {
        LOG_DEBUG("[WIFI] Connecting to Wi-Fi...");
        LOG_DEBUG("[WIFI] SSID: %s", wifi.ssid.c_str());
    }
    WiFi.mode(WIFI_STA);
    if (!wifi.dhcpFlag)
    {
        IPAddress staticIP(wifi.ip[0], wifi.ip[1], wifi.ip[2], wifi.ip[3]);
        IPAddress gateway(wifi.gateway[0], wifi.gateway[1], wifi.gateway[2], wifi.gateway[3]);
        IPAddress subnet(wifi.subnet[0], wifi.subnet[1], wifi.subnet[2], wifi.subnet[3]);
        WiFi.config(staticIP, gateway, subnet);
    }

    WiFi.onEvent([this](WiFiEvent_t event, WiFiEventInfo_t info)
                 { this->WiFiEvent(event); });

    WiFi.begin(wifi.ssid.c_str(), wifi.password.c_str());

    int attempts = 0;
    const int maxAttempts = 50;
    printf("Conectando.");
    while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts)
    {
        vTaskDelay(pdMS_TO_TICKS(200));
        printf(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        if (_log == WiFiLog::ENABLE)
        {
            LOG_INFO("[WIFI] Connected to Wi-Fi!");
            LOG_INFO("[WIFI] IP Address: %s", WiFi.localIP().toString().c_str());
        }
    }
    else
    {
        ESP_LOGW(TAG, "Failed to connect to Wi-Fi.");
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF); // Reinicia completamente a interface Wi-Fi
    }
    return;
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
        // display.modifyIconOnTray("WiFi", wifiConnectedIcon, sizeof(wifiConnectedIcon));
        break;

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        if (_log == WiFiLog::ENABLE)
        {
            LOG_INFO("[WIFI] Wi-Fi STA disconnected");
        }
        _wifi.connectionStatus = ARDUINO_EVENT_WIFI_STA_DISCONNECTED;
        // display.modifyIconOnTray("WiFi", wifiDisconnectedIcon, sizeof(wifiDisconnectedIcon));
        break;

    default:
        if (_log == WiFiLog::ENABLE)
        {
            LOG_INFO("[WIFI] Unhandled Wi-Fi event: %d", event);
        }
        break;
    }
}