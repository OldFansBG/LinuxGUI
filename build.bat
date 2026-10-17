@echo off
setlocal

:: Builds LinuxISOPro with the MSYS2 UCRT64 toolchain (see README.md for the packages to install).
:: Override MSYS2_ROOT if MSYS2 is not installed in C:\msys64.
if "%MSYS2_ROOT%"=="" set MSYS2_ROOT=C:\msys64
set UCRT64=%MSYS2_ROOT%\ucrt64

if not exist "%UCRT64%\bin\g++.exe" (
    echo MSYS2 UCRT64 toolchain not found in %UCRT64%.
    echo Install MSYS2 from https://www.msys2.org and run the pacman command from README.md.
    exit /b 1
)

set PATH=%UCRT64%\bin;%PATH%

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=%UCRT64%
if errorlevel 1 goto :failed

cmake --build build
if errorlevel 1 goto :failed

:: Copy the MSYS2 DLLs the executable depends on, so build\ can be run or zipped as-is.
echo Copying runtime DLLs...
set MSYSTEM=UCRT64
set CHERE_INVOKING=1
"%MSYS2_ROOT%\usr\bin\bash.exe" -lc "cd build && ldd LinuxISOPro.exe | grep -i '/ucrt64/' | awk '{print $3}' | xargs -r -I{} cp -u {} ."

echo.
echo Build completed successfully: %CD%\build\LinuxISOPro.exe
echo Remember to put script.exe (see README.md) next to LinuxISOPro.exe.
exit /b 0

:failed
echo Build failed!
exit /b 1
