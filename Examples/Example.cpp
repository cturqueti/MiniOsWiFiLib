
#define VERSAO "2.0"
#include "WiFiLib.h"
#include <Arduino.h>
#include <ErrorLib.h>
#include <LittleFS.h>
#include <LogLibrary.h>
#include <SPI.h>
#include <WiFiItems.h>

// Definição dos pinos para a serial alternativa (RX, TX)
#define UART_RX_PIN 44
#define UART_TX_PIN 43

// Velocidades de comunicação serial
#define MAIN_SERIAL_BAUDRATE 115200

// Instância da serial alternativa
HardwareSerial HardwareSerialPort(0); // UART0

void setup()
{
    HardwareSerialPort.begin(MAIN_SERIAL_BAUDRATE, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
    Log::begin(&HardwareSerialPort);
    Log::enableColors(false);

    vTaskDelay(pdMS_TO_TICKS(2000));
    LOG_INFO("[MAIN] Iniciando MiniOS v%s", VERSAO);

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
