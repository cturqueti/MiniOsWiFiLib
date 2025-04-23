
#define VERSAO "1.0"
#include "ErrorLib.h"
#include "WiFiLib.h"
#include <Arduino.h>
#include <LittleFS.h>
#include <LogLibrary.h>
#include <SPI.h>
#include <WiFiItems.h>
// #include "ServicesManager.h"
// #include "Utils.h"

// Definição dos pinos para a serial alternativa (RX, TX)
#define UART_RX_PIN 44
#define UART_TX_PIN 43

// Velocidades de comunicação serial
#define MAIN_SERIAL_BAUDRATE 115200

// Instância da serial alternativa
HardwareSerial HardwareSerialPort(0); // UART0

// WiFiCredentialsJSON json;

void setup()
{
    HardwareSerialPort.begin(MAIN_SERIAL_BAUDRATE, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
    Log::begin(&HardwareSerialPort);
    Log::enableColors(false);

    vTaskDelay(pdMS_TO_TICKS(2000));
    LOG_INFO("[MAIN] Iniciando MiniOS v%s", VERSAO);

    // auto& littleFs = LittleFSLib::getInstance(true);
    // auto& nvs = WiFiCredentialsNVS::getInstance(WiFiLog::ENABLE);

    auto &wifiCommon = WiFiLib::getInstance(WiFiLog::ENABLE);
    if (wifiCommon.isConfigLoaded())
    {
        LOG_DEBUG("[MAIN] Configuracoes carregadas");
    }
    else
    {
        LOG_DEBUG("[MAIN] Configuracoes nao carregadas");
    }

    wifiCommon.begin();
}

void loop()
{
    WiFiLib &wifiCommon = WiFiLib::getInstance();
}
