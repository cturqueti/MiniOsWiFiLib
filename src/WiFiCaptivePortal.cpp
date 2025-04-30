#include "WiFiCaptivePortal.h"
#include <LittleFS.h>

WiFiCaptivePortal::WiFiCaptivePortal(WiFiLog log) : _server(80), _isRunning(false), _serverTaskHandle(NULL)
{
    _log = log;
    if (!LittleFS.begin())
    {
        if (_log == WiFiLog::ENABLE)
        {
            LOG_ERROR("Failed to mount LittleFS");
        }
        ERRORS_LIST.addError(ErrorCode::LITTLEFS_MOUNT_ERROR);
    }

    if (!LittleFS.exists(captivePortalFolder.data()))
    {
        if (_log == WiFiLog::ENABLE)
        {
            LOG_ERROR("Pasta %s não encontrada", captivePortalFolder.data());
        }
        ERRORS_LIST.addError(ErrorCode::FILE_NOT_FOUND);
    }

    _ipAp = IPAddress(192, 168, 4, 1);
}

WiFiCaptivePortal::~WiFiCaptivePortal() { end(); }

void WiFiCaptivePortal::begin()
{
    if (_isRunning)
    {
        return;
    }
    if (_log == WiFiLog::ENABLE)
    {
        LOG_INFO("Iniciando AP");
    }
    _startAP();
    _setupDNS();
    _setupServer();

    xTaskCreate(_serverTask, "CaptivePortalTask", CAPTIVE_PORTAL_TASK_STACK_SIZE, this, 1, &_serverTaskHandle);

    _isRunning = true;
}

void WiFiCaptivePortal::end()
{
    if (!_isRunning)
        return;

    _server.stop();
    _dnsServer.stop();
    WiFi.softAPdisconnect(true);

    if (_serverTaskHandle != NULL)
    {
        vTaskDelete(_serverTaskHandle);
        _serverTaskHandle = NULL;
    }

    _isRunning = false;
}

bool WiFiCaptivePortal::isRunning() const { return _isRunning; }

void WiFiCaptivePortal::_startAP()
{
    WiFi.softAP(CAPTIVE_PORTAL_SSID);
    WiFi.softAPConfig(_ipAp, _ipAp, IPAddress(255, 255, 255, 0));
    if (_log == WiFiLog::ENABLE)
    {
        LOG_INFO("AP IP address: %s", WiFi.softAPIP().toString().c_str());
    }
}

void WiFiCaptivePortal::_setupDNS()
{
    _dnsServer.start(CAPTIVE_PORTAL_DNS_PORT, "*", WiFi.softAPIP());

    if (_log == WiFiLog::ENABLE)
    {
        LOG_INFO("DNS server started on %s", WiFi.softAPIP().toString().c_str());
    }
}

void WiFiCaptivePortal::_setupServer()
{
    _server.on("/", HTTP_GET, [this]()
               {
        if (_log == WiFiLog::ENABLE) {
            LOG_INFO("Serving index page /");
        }
        _handleRoot(); });

    _server.on("/index.html", HTTP_GET, [this]()
               {
                   if (_log == WiFiLog::ENABLE)
                   {
                       LOG_INFO("Serving index page /index.html");
                   }
                   _handleRoot(); // Redireciona para a raiz
               });

    _server.on("/config.html", HTTP_GET, [this]()
               {
        if (_log == WiFiLog::ENABLE) {
            LOG_INFO("Serving config page /config.html");
        }
        _handleConfig(); });

    /* ------------- Redireciona para a página do portal ------------ */
    _server.on("/connecttest.txt", HTTP_GET, [this]()
               {
        if (_log == WiFiLog::ENABLE) {
            LOG_INFO("Received connecttest.txt request");
        }
        // _server.send(200, "text/plain", "Microsoft NCSI");
        _server.sendHeader("Location", "/"); // Redireciona para a página do portal
        _server.send(302, "text/plain", "Redirecting to captive portal"); });

    _server.on("/hotspot-detect.html", HTTP_GET, [this]()
               {
        if (_log == WiFiLog::ENABLE) {
            LOG_INFO("Received hotspot-detect.html request");
        }
        _server.sendHeader("Location", "/");
        _server.send(302, "text/plain", ""); });

    _server.on("/ncsi.txt", HTTP_GET, [this]()
               {
                   if (_log == WiFiLog::ENABLE)
                   {
                       LOG_INFO("Received ncsi.txt request");
                   }
                   // _server.send(200, "text/plain", "Microsoft NCSI");
                   _server.sendHeader("Location", "/"); // Redireciona para a página do portal
                   _server.send(302, "text/plain", "Redirecting to captive portal");
                   // _server.send(200, "text/plain", "OK");
               });

    _server.on("/generate_204", HTTP_GET, [this]()
               {
                   if (_log == WiFiLog::ENABLE)
                   {
                       LOG_INFO("Received generate_204 request");
                   }
                   // _server.send(200, "text/plain", "Microsoft NCSI");
                   _server.sendHeader("Location", "/"); // Redireciona para a página do portal
                   _server.send(302, "text/plain", "Redirecting to captive portal");
                   // _server.send(204, "text/plain", "");
               });
    /* ------------- FIM da redirecionamento da página do portal ------------- */
    _server.on("/favicon.ico", HTTP_GET, [this]()
               {
        if (_log == WiFiLog::ENABLE) {
            LOG_INFO("Received favicon.ico request");
        }
        String iconPath = String(captivePortalFolder.data()) + "/icon.png";
        if (LittleFS.exists(iconPath)) {
            File file = LittleFS.open(iconPath, "r");
            _server.sendHeader("Cache-Control", "public, max-age=86400"); // Cache de 1 dia
            _server.send(200, "image/png", file.readString());
            file.close();
        } else {
            _server.send(404, "text/plain", "Favicon not found");
        } });

    _server.on("/scan-wifi", HTTP_GET, [this]()
               { this->_handleScanWifi(); });

    _server.on("/save-wifi-settings", HTTP_POST, [this]()
               { this->_handleSaveWiFiSettings(); });

    _server.on("/success", HTTP_GET, [this]()
               {
        if (_log == WiFiLog::ENABLE) {
            LOG_INFO("Received success request");
        }
        _server.sendHeader("Location", "/config.html");
        _server.send(302, "text/plain", "Redirecting to config"); });

    _server.on("/wifi-settings", HTTP_GET, [this]()
               {
        if (_log == WiFiLog::ENABLE) {
            LOG_INFO("Received wifi-settings request");
        }

        String path = String(configFolder.data()) + "/configWiFi.json";

        // Verifica se o arquivo HTML principal existe
        if (!LittleFS.exists(path)) {
            _logError(F("Index file not found"), path, ErrorCode::FILE_NOT_FOUND);
            _server.send(200, "text/html",
                         "<!DOCTYPE html><html><head><title>Erro</title></head>"
                         "<body><h1>Configuracao</h1><p>Página não encontrada</p></body></html>");
            return;
        }
        String json = _loadFromLittleFS(path);
        if (json.isEmpty()) {
            if (_log == WiFiLog::ENABLE) {
                LOG_ERROR("Failed to open file: %s", path.c_str());
            }
            _server.send(404, "text/plain", "File not found");
            return;
        }

        // Enviar o arquivo diretamente
        _server.send(200, "application/json", json); });

    _server.onNotFound([this]()
                       {
        _handleRoot();
        if (_log == WiFiLog::ENABLE) {
            LOG_INFO("Page not found: %s", _server.uri().c_str());
        }
        _server.sendHeader("Location", "/");
        _server.send(302, "text/plain", "Redirecting to /"); });

    _server.begin();
}

void WiFiCaptivePortal::_handleClient()
{
    _server.handleClient();
    _dnsServer.processNextRequest();
}

void WiFiCaptivePortal::_serverTask(void *pvParameters)
{
    WiFiCaptivePortal *portal = (WiFiCaptivePortal *)pvParameters;
    while (true)
    {
        portal->_handleClient();
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

String WiFiCaptivePortal::_loadFromLittleFS(const String &path)
{
    if (_log == WiFiLog::ENABLE)
    {
        LOG_DEBUG("Loading file: %s", path.c_str());
    }
    File file = LittleFS.open(path, "r");
    if (!file)
    {
        if (_log == WiFiLog::ENABLE)
        {
            LOG_ERROR("Failed to open file: %s", path.c_str());
        }
        ERRORS_LIST.addError(ErrorCode::FILE_NOT_FOUND);
        return "";
    }

    String content = file.readString();
    file.close();
    return content;
}

String WiFiCaptivePortal::_getContentType(const String &filename)
{
    if (filename.endsWith(".html"))
        return "text/html";
    else if (filename.endsWith(".css"))
        return "text/css";
    else if (filename.endsWith(".js"))
        return "application/javascript";
    else if (filename.endsWith(".ico"))
        return "image/x-icon";
    else if (filename.endsWith(".png"))
        return "image/png";
    else if (filename.endsWith(".jpg") || filename.endsWith(".jpeg"))
        return "image/jpeg";
    else if (filename.endsWith(".gif"))
        return "image/gif";
    else if (filename.endsWith(".svg"))
        return "image/svg+xml";
    else if (filename.endsWith(".json"))
        return "application/json";
    else if (filename.endsWith(".txt"))
        return "text/plain";
    else if (filename.endsWith(".xml"))
        return "text/xml";
    return "application/octet-stream";
}

bool WiFiCaptivePortal::_beginCredentials()
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

bool WiFiCaptivePortal::_saveCredentials(WiFiItems &credentials)
{
    if (!_beginCredentials())
    {
        return false;
    }

    bool success = true;

    // Verifique cada operação individualmente
    if (!_preferences.putString("ssid", credentials.ssid.c_str()))
    {
        if (_log == WiFiLog::ENABLE)
        {
            LOG_ERROR("[WiFi] Failed to save SSID");
        }
        success = false;
    }

    if (!_preferences.putString("password", credentials.password.c_str()))
    {
        if (_log == WiFiLog::ENABLE)
        {
            LOG_ERROR("[WiFi] Failed to save password");
        }
        success = false;
    }

    if (!_preferences.putBool("dhcpFlag", credentials.dhcp))
    {
        if (_log == WiFiLog::ENABLE)
        {
            LOG_ERROR("[WiFi] Failed to save DHCP flag");
        }
        success = false;
    }

    _preferences.end();

    if (!success)
    {
        ERRORS_LIST.addError(ErrorCode::NVS_SAVE_ERROR);
    }

    return success;
}

bool WiFiCaptivePortal::_loadCredentials(WiFiItems &credentials)
{
    if (!_beginCredentials())
    {
        return false;
    }

    credentials.ssid = _preferences.getString("ssid", "");
    credentials.password = _preferences.getString("password", "");
    credentials.dhcp = _preferences.getBool("dhcpFlag", true);

    _preferences.end();

    return !credentials.ssid.isEmpty(); // Consideramos válido se pelo menos o SSID existe
}

bool WiFiCaptivePortal::_saveSettings(WiFiItems &settings)
{
    JsonDocument docFile;

    String configPath = String(configFolder.data()) + "/configWiFi.json";
    File configFile = LittleFS.open(configPath, "w");
    if (!configFile)
    {
        if (_log == WiFiLog::ENABLE)
        {
            LOG_ERROR("Failed to open file %s for writing", configPath.c_str());
        }
        ERRORS_LIST.addError(ErrorCode::FILE_NOT_CREATED);
        _server.send(404, "text/plain", "Config page not found");
        return false;
    }

    docFile["dhcp"] = settings.dhcp;
    docFile["mDns"] = settings.mDns;

    if (!settings.dhcp)
    {
        docFile["ip"] = settings.ip.toString();
        docFile["gateway"] = settings.gateway.toString();
        docFile["subnet"] = settings.subnet.toString();

        WiFi.config(settings.ip, settings.gateway, settings.subnet);
    }
    else
    {
        docFile["ip"] = WiFi.localIP().toString();
        docFile["gateway"] = WiFi.gatewayIP().toString();
        docFile["subnet"] = WiFi.subnetMask().toString();

        WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
    }

    if (serializeJson(docFile, configFile) == 0)
    {
        if (_log == WiFiLog::ENABLE)
        {
            LOG_ERROR("Failed to write to file");
        }
        ERRORS_LIST.addError(ErrorCode::FILE_NOT_CREATED);
        _server.send(404, "text/plain", "Config page not found");
        return false;
    }

    configFile.close();

    return true;
}

void WiFiCaptivePortal::_handleRoot()
{
    // Define os caminhos dos arquivos
    const String indexPath = String(captivePortalFolder.data()) + "/index.html";
    const String cssPath = String(captivePortalFolder.data()) + "/style.css";
    const String jsPath = String(captivePortalFolder.data()) + "/script.js";
    const String iconPath = String(captivePortalFolder.data()) + "/images/icon.png";

    // Verifica se o arquivo HTML principal existe
    if (!LittleFS.exists(indexPath))
    {
        _logError(F("Index file not found"), indexPath, ErrorCode::FILE_NOT_FOUND);
        _server.send(200, "text/html",
                     "<!DOCTYPE html><html><head><title>Erro</title></head>"
                     "<body><h1>Portal Captivo</h1><p>Página não encontrada</p></body></html>");
        return;
    }

    // Carrega o HTML principal
    String html = _loadFromLittleFS(indexPath);
    if (html.isEmpty())
    {
        _logError(F("Failed to load index file"), indexPath, ErrorCode::FILE_READ_ERROR);
        _server.send(500, "text/plain", "Erro ao carregar a página");
        return;
    }

    // Processa CSS incorporado
    _embedFileContent(html, cssPath, "<style>", "</style>", "</head>");

    // Processa JavaScript incorporado
    _embedFileContent(html, jsPath, "<script>", "</script>", "</body>");

    // Envia a resposta
    _server.send(200, "text/html", html);
}

// Função auxiliar para registrar erros
void WiFiCaptivePortal::_logError(const __FlashStringHelper *message, const String &path, ErrorCode code)
{
    if (_log == WiFiLog::ENABLE)
    {
        LOG_ERROR("%s: %s", message, path.c_str());
    }
    ERRORS_LIST.addError(code);
}

// Função auxiliar para incorporar arquivos
void WiFiCaptivePortal::_embedFileContent(String &html, const String &filePath, const String &prefix,
                                          const String &suffix, const String &insertBefore)
{
    if (LittleFS.exists(filePath))
    {
        String content = _loadFromLittleFS(filePath);
        if (!content.isEmpty())
        {
            html.replace(insertBefore, prefix + content + suffix + insertBefore);
        }
        else
        {
            _logError(F("Failed to load file"), filePath, ErrorCode::FILE_READ_ERROR);
        }
    }
    else
    {
        _logError(F("File not found"), filePath, ErrorCode::FILE_NOT_FOUND);
    }
}

void WiFiCaptivePortal::_handleScanWifi()
{
    if (_log == WiFiLog::ENABLE)
    {
        LOG_DEBUG("Handling WiFi scan request");
    }

    // Realiza o scan das redes WiFi
    int numNetworks = WiFi.scanNetworks(false, true); // scanNetworks(async, showHidden)

    // Cria o documento JSON
    JsonDocument doc; // Tamanho ajustado conforme necessidade
    JsonArray networks = doc.to<JsonArray>();

    for (int i = 0; i < numNetworks; ++i)
    {
        JsonObject network = networks.add<JsonObject>();

        network["ssid"] = WiFi.SSID(i);
        network["rssi"] = WiFi.RSSI(i);
        network["channel"] = WiFi.channel(i);
        network["open"] = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN);

        // Adiciona ícone baseado na segurança (opcional)
        network["icon"] = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "🔓" : "🔒";

        if (_log == WiFiLog::ENABLE)
        {
            LOG_DEBUG("Found network: %s (%ddBm)", WiFi.SSID(i).c_str(), WiFi.RSSI(i));
        }
    }

    // Serializa o JSON para string
    String response;
    serializeJson(doc, response);

    // Envia a resposta
    _server.send(200, "application/json", response);

    // Limpa os resultados do scan
    WiFi.scanDelete();
}

void WiFiCaptivePortal::_handleConfig()
{
    if (_log == WiFiLog::ENABLE)
    {
        LOG_DEBUG("Loading config page...");
    }
    String configPath = String(captivePortalFolder.data()) + "/config.json";
    if (!LittleFS.exists(configPath))
    {
        if (_log == WiFiLog::ENABLE)
        {
            LOG_ERROR("%s not found", configPath.c_str());
        }
        ERRORS_LIST.addError(ErrorCode::FILE_NOT_FOUND);
        _server.send(404, "text/plain", "Config page not found");
        return;
    }

    String html = _loadFromLittleFS(configPath);
    _server.send(200, "text/html", html);
    return;
}

void WiFiCaptivePortal::_handleSaveWiFiSettings()
{
    if (_log == WiFiLog::ENABLE)
    {
        LOG_INFO("Received connect request");
    }
    JsonDocument doc, docFile;
    DeserializationError error = deserializeJson(doc, _server.arg("plain"));
    if (error)
    {
        _server.send(400, "application/json", "{\"success\":false,\"message\":\"JSON inválido\"}");
        if (_log == WiFiLog::ENABLE)
        {
            LOG_ERROR("JSON inválido: %s", error.c_str());
        }
        return;
    }
    WiFiItems config;
    // 1. Obter dados básicos
    config.ssid = doc["ssid"].as<String>();
    config.password = doc["password"].as<String>();
    config.dhcp = doc["dhcp"].as<bool>();
    config.mDns = doc["mDns"].as<String>();
    if (!config.dhcp)
    {
        if (!config.ip.fromString(doc["ip"].as<String>()))
        {
            LOG_ERROR("IP estático inválido");
            return;
        }

        if (!config.gateway.fromString(doc["gateway"].as<String>()))
        {
            LOG_ERROR("Gateway inválido");
            return;
        }

        if (!config.subnet.fromString(doc["subnet"].as<String>()))
        {
            LOG_ERROR("Máscara de sub-rede inválida");
            return;
        }
    }

    if (_log == WiFiLog::ENABLE)
    {
        LOG_DEBUG("SSID: %s", config.ssid.c_str());
        LOG_DEBUG("Password: %s", config.password.c_str());
        LOG_DEBUG("DHCP Mode: %d", config.dhcp);
        LOG_DEBUG("mDns: %s", config.mDns.c_str());
        if (!config.dhcp)
        {
            LOG_DEBUG("IP: %s", config.ip.toString().c_str());
            LOG_DEBUG("Gateway: %s", config.gateway.toString().c_str());
            LOG_DEBUG("Subnet: %s", config.subnet.toString().c_str());
        }
    }

    // 2. Conectar à rede
    WiFi.begin(config.ssid.c_str(), config.password.c_str());

    // Aguardar conexão (com timeout)
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20)
    {
        delay(500);
        attempts++;
    }

    // 3. Verificar se a conexão foi bem sucedida

    if (WiFi.status() == WL_CONNECTED)
    {
        // Executar ação após conexão bem sucedida

        if (_log == WiFiLog::ENABLE)
        {
            LOG_INFO("Connected to WiFi. IP: %s", WiFi.localIP().toString().c_str());
        }

        JsonDocument responseDoc;
        responseDoc["message"] = WiFi.status() == WL_CONNECTED ? "Conectado com sucesso a rede " + config.ssid +
                                                                     "\n IP: " + WiFi.localIP().toString()
                                                               : "Falha na conexão";

        _saveCredentials(config);
        _saveSettings(config);

        String response;
        serializeJson(responseDoc, response);
        _server.send(200, "application/json", response);
        config.clear();
    }
    else
    {
        // Caso a conexão falhe
        JsonDocument responseDoc;
        responseDoc["message"] = "Falha na conexão";

        String response;
        serializeJson(responseDoc, response);
        _server.send(200, "application/json", response);
    }

    // 4. Iniciar mDNS
    if (!MDNS.begin(config.mDns))
    { // "esp32" será o nome do seu dispositivo
        if (_log == WiFiLog::ENABLE)
        {
            LOG_ERROR("Erro ao iniciar mDNS");
        }
        ERRORS_LIST.addError(ErrorCode::MDNS_ERROR);
    }
    else
    {
        if (_log == WiFiLog::ENABLE)
        {
            LOG_INFO("mDNS iniciado com sucesso, com hostname: %s", config.mDns.c_str());
        }
    }
    if (_log == WiFiLog::ENABLE)
    {
        LOG_INFO("mDNS iniciado");
    }

    // Adicione serviços (opcional)
    MDNS.addService("http", "tcp", 80); // Serviço web na porta 80

    end();
}