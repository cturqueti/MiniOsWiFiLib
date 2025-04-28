#ifndef AES_DECRYPTOR_H
#define AES_DECRYPTOR_H

#include "mbedtls/gcm.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

class AESDecryptor
{
public:
    AESDecryptor();

    void begin(const char *keyPath = "/chave_aes.key");
    void end();

    bool loadKey();
    bool decryptToJson(const String &encryptedData, JsonDocument &jsonDoc);
    bool decryptToJson(const uint8_t *encryptedData, size_t length, JsonDocument &jsonDoc);
    const uint8_t *getKey() const { return _aesKey; }
    size_t getKeyLength() const { return _keyLength; }

private:
    const char *_keyPath;
    uint8_t _aesKey[32]; // Para AES-256
    size_t _keyLength;
    mbedtls_gcm_context _aes;

    bool _base64Decode(const String &input, uint8_t *output, size_t *outputLength);
    bool _decryptAESGCM(const uint8_t *encryptedData, size_t dataLen, uint8_t *output, size_t *outputLen);
};

#endif