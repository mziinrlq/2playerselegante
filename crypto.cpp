#include "crypto.h"
#include <bcrypt.h>
#include <wincrypt.h>
#include <cstring>

#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "crypt32.lib")

bool dpapi_decrypt(const std::vector<BYTE>& in, std::vector<BYTE>& out) {
    if (in.empty()) return false;
    DATA_BLOB dIn{ (DWORD)in.size(), (BYTE*)in.data() };
    DATA_BLOB dOut{};
    if (!CryptUnprotectData(&dIn, nullptr, nullptr, nullptr, nullptr, 0, &dOut))
        return false;
    out.assign(dOut.pbData, dOut.pbData + dOut.cbData);
    LocalFree(dOut.pbData);
    return true;
}

std::vector<BYTE> b64_decode(const std::string& s) {
    std::vector<BYTE> out;
    if (s.empty()) return out;
    DWORD n = 0;
    if (!CryptStringToBinaryA(s.c_str(), (DWORD)s.size(), CRYPT_STRING_BASE64, nullptr, &n, nullptr, nullptr)) return out;
    out.resize(n);
    CryptStringToBinaryA(s.c_str(), (DWORD)s.size(), CRYPT_STRING_BASE64, out.data(), &n, nullptr, nullptr);
    out.resize(n);
    return out;
}

bool aes_gcm_decrypt(const std::vector<BYTE>& key, const std::vector<BYTE>& nonce, const std::vector<BYTE>& ct, const std::vector<BYTE>& tag, std::vector<BYTE>& plain) {
    BCRYPT_ALG_HANDLE alg = nullptr;
    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_AES_ALGORITHM, nullptr, 0) != 0) return false;
    BCRYPT_KEY_HANDLE keyH = nullptr;
    NTSTATUS st = BCryptGenerateSymmetricKey(alg, &keyH, nullptr, 0, (PUCHAR)key.data(), (ULONG)key.size(), 0);
    if (st == 0) {
        BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO info;
        BCRYPT_INIT_AUTH_MODE_INFO(info);
        info.pbNonce = (PUCHAR)nonce.data();
        info.cbNonce = (ULONG)nonce.size();
        info.pbTag = (PUCHAR)tag.data();
        info.cbTag = (ULONG)tag.size();
        ULONG outLen = 0;
        plain.resize(ct.size());
        st = BCryptDecrypt(keyH, (PUCHAR)ct.data(), (ULONG)ct.size(), &info, nullptr, 0, plain.data(), (ULONG)plain.size(), &outLen, 0);
        plain.resize(outLen);
        BCryptDestroyKey(keyH);
    }
    BCryptCloseAlgorithmProvider(alg, 0);
    return st == 0;
}