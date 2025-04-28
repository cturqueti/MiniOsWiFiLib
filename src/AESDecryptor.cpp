#include "AESDecryptor.h"
#include "esp32/aes.h"
#include "esp32/sha.h"
// #include <mbedtls/gcm.h>

AESDecryptor::AESDecryptor() : _keyLength(0)
{
    memset(_aesKey, 0, sizeof(_aesKey));
}

void AESDecryptor::begin(const char *keyPath)
{
    _keyPath = keyPath;
    loadKey();
}

void AESDecryptor::end()
{
    memset(_aesKey, 0, sizeof(_aesKey));
}

bool AESDecryptor::loadKey()
{
    File keyFile = LittleFS.open(_keyPath, "r");
    if (!keyFile)
    {
        Serial.println("Falha ao abrir arquivo de chave");
        return false;
    }

    _keyLength = keyFile.read(_aesKey, sizeof(_aesKey));
    keyFile.close();

    if (_keyLength != 16 && _keyLength != 24 && _keyLength != 32)
    {
        Serial.println("Tamanho de chave inválido");
        memset(_aesKey, 0, sizeof(_aesKey));
        _keyLength = 0;
        return false;
    }

    return true;
}

bool AESDecryptor::decryptToJson(const String &encryptedData, JsonDocument &jsonDoc)
{
    // Decodificar Base64
    size_t decodedLength = (encryptedData.length() * 3) / 4;
    uint8_t decodedData[decodedLength];

    if (!_base64Decode(encryptedData, decodedData, &decodedLength))
    {
        Serial.println("Falha ao decodificar Base64");
        return false;
    }

    return decryptToJson(decodedData, decodedLength, jsonDoc);
}

bool AESDecryptor::decryptToJson(const uint8_t *encryptedData, size_t length, JsonDocument &jsonDoc)
{
    // Buffer para dados descriptografados
    uint8_t decrypted[1024];
    size_t decryptedLen = 0;

    if (!_decryptAESGCM(encryptedData, length, decrypted, &decryptedLen))
    {
        Serial.println("Falha ao descriptografar");
        return false;
    }

    // Parse JSON
    DeserializationError error = deserializeJson(jsonDoc, decrypted, decryptedLen);
    if (error)
    {
        Serial.print("Falha ao parsear JSON: ");
        Serial.println(error.c_str());
        return false;
    }

    return true;
}

bool AESDecryptor::_base64Decode(const String &input, uint8_t *output, size_t *outputLength)
{

    size_t inputLength = input.length();
    size_t outputPos = 0;

    for (size_t i = 0; i < inputLength; i += 4)
    {
        uint32_t value = 0;
        for (size_t j = 0; j < 4; j++)
        {
            char c = input[i + j];
            if (c >= 'A' && c <= 'Z')
            {
                value |= (c - 'A' + 10) << (18 - 6 * j);
            }
            else if (c >= 'a' && c <= 'z')
            {
                value |= (c - 'a' + 10) << (18 - 6 * j);
            }
            else if (c >= '0' && c <= '9')
            {
                value |= (c - '0') << (18 - 6 * j);
            }
            else if (c == '+')
            {
                value |= 0x3F << (18 - 6 * j);
            }
            else if (c == '/')
            {
                value |= 0x3E << (18 - 6 * j);
            }
        }

        if (outputPos + 3 > *outputLength)
        {
            return false;
        }

        output[outputPos++] = (value >> 16) & 0xFF;
        output[outputPos++] = (value >> 8) & 0xFF;
        output[outputPos++] = value & 0xFF;
    }

    *outputLength = outputPos;

    return true;
}

bool AESDecryptor::_decryptAESGCM(const uint8_t *encryptedData, size_t dataLen, uint8_t *output, size_t *outputLen)
{
    if (dataLen < 12 + 16)
    { // IV (12) + tag (16)
        Serial.println("Dados criptografados muito curtos");
        return false;
    }

    const uint8_t *iv = encryptedData;
    const uint8_t *ciphertext = encryptedData + 12;
    size_t ciphertext_len = dataLen - 12 - 16;
    const uint8_t *tag = encryptedData + 12 + ciphertext_len;
    int ret;

    mbedtls_gcm_init(&_aes);

    ret = mbedtls_gcm_setkey(&_aes, MBEDTLS_CIPHER_ID_AES, _aesKey, _keyLength * 8); // bits, não bytes
    if (ret != 0)
    {
        Serial.printf("Erro ao configurar chave AES: -0x%04X\n", -ret);
        mbedtls_gcm_free(&_aes);
        return false;
    }
    ret = mbedtls_gcm_auth_decrypt(
        &_aes,
        ciphertext_len,
        iv, 12,
        NULL, 0, // Sem AAD (Additional Authenticated Data)
        tag, 16,
        ciphertext,
        output);

    mbedtls_gcm_free(&_aes);

    if (ret != 0)
    {
        Serial.printf("Erro na descriptografia AES-GCM: -0x%04X\n", -ret);
        return false;
    }

    *outputLen = ciphertext_len;
    return true;
}