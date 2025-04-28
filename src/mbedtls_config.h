#ifndef MBEDTLS_CONFIG_H
#define MBEDTLS_CONFIG_H

#include "mbedtls/esp_config.h"

// Habilitar apenas os recursos necessários
#define MBEDTLS_AES_C
#define MBEDTLS_GCM_C
#define MBEDTLS_CIPHER_MODE_CBC
#define MBEDTLS_CIPHER_C

// Usar hardware acceleration do ESP32
#define MBEDTLS_AES_ROM_TABLES
#define MBEDTLS_AES_USE_HARDWARE_ONLY

#endif /* MBEDTLS_CONFIG_H */