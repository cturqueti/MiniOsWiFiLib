#include "WiFiItems.h"

WiFiItems::WiFiItems()
    : ssid(""), password(""), dhcp(true), mDns(), ip(), gateway(), subnet(255, 255, 255, 0), configLoaded(false),
      connectionStatus(0) {}

void WiFiItems::clear() {
    ssid.clear();
    clearPassword(); // Usa o método seguro
    dhcp = true;     // DHCP ativado por padrão
    ip = IPAddress();
    gateway = IPAddress();
    subnet = IPAddress(255, 255, 255, 0); // Máscara padrão
    mDns.clear();
    configLoaded = false;
    connectionStatus = 0;
}

void WiFiItems::clearPassword() {
    // Sobrescreve o buffer antes de limpar
    for (size_t i = 0; i < password.length(); i++) {
        password[i] = '\0'; // Sobrescreve cada caractere com null
    }
    password = "";       // Limpa a string
    password.reserve(0); // Libera a memória alocada
}

String WiFiItems::toJson() const {
    JsonDocument doc;
    doc["ssid"] = ssid;
    doc["dhcp"] = dhcp;
    doc["ip"] = ip.toString();
    doc["gateway"] = gateway.toString();
    doc["subnet"] = subnet.toString();
    doc["mDns"] = mDns;

    String output;
    serializeJson(doc, output);
    return output;
}