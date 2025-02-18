@echo off
setlocal

:: Set MSYS2 paths
set "MSYS2_BIN=C:\msys64\mingw64\bin"
set "PATH=%MSYS2_BIN%;%PATH%"

:: Create build directory
if not exist "build" mkdir build
cd build || exit /b 1

:: Configure CMake
cmake .. -G "MinGW Makefiles" ^
    -DCMAKE_C_COMPILER="%MSYS2_BIN%\gcc.exe" ^
    -DCMAKE_CXX_COMPILER="%MSYS2_BIN%\g++.exe" ^
    -DCURL_INCLUDE_DIR="C:/msys64/mingw64/include" ^
    -DCURL_LIBRARY="C:/msys64/mingw64/lib/libcurl.dll.a" ^
    -DOPENSSL_ROOT_DIR="C:/msys64/mingw64" ^
    -DOPENSSL_INCLUDE_DIR="C:/msys64/mingw64/include" ^
    -DOPENSSL_CRYPTO_LIBRARY="C:/msys64/mingw64/lib/libcrypto-3-x64.dll.a" ^
    -DCMAKE_PREFIX_PATH="C:/msys64/mingw64"

:: Build
mingw32-make || exit /b 1

:: Check if build was successful


endlocal