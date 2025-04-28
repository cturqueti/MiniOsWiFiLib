#ifndef ESP32_MBEDTLS_CONFIG_H
#define ESP32_MBEDTLS_CONFIG_H

/* System support */
#define MBEDTLS_PLATFORM_C
#define MBEDTLS_PLATFORM_MEMORY
#define MBEDTLS_MEMORY_BUFFER_ALLOC_C

/* Crypto features */
#define MBEDTLS_AES_C
#define MBEDTLS_GCM_C
#define MBEDTLS_CIPHER_C

/* Hardware acceleration */
#define MBEDTLS_AES_ROM_TABLES
#define MBEDTLS_AES_USE_HARDWARE_ONLY

#endif /* ESP32_MBEDTLS_CONFIG_H */