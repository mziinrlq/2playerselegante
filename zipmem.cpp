#include "zipmem.h"
#include <windows.h>
#include <string>

bool create_zip(const std::wstring& folderPath, const std::wstring& zipPath) {
    DeleteFileW(zipPath.c_str());
    std::wstring cmd = L"tar.exe -a -c -f \"" + zipPath + L"\" -C \"" + folderPath + L"\" *";
    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi;
    if (CreateProcessW(NULL, (LPWSTR)cmd.c_str(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, 60000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    return true;
}