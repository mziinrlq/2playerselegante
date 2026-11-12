#ifndef CRYPTO_H
#define CRYPTO_H

#include <windows.h>
#include <string>
#include <vector>

bool dpapi_decrypt(const std::vector<BYTE>& in, std::vector<BYTE>& out);
std::vector<BYTE> b64_decode(const std::string& s);
bool aes_gcm_decrypt(const std::vector<BYTE>& key,
                     const std::vector<BYTE>& nonce,
                     const std::vector<BYTE>& ct,
                     const std::vector<BYTE>& tag,
                     std::vector<BYTE>& plain);

#endif