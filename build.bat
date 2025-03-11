@echo off
setlocal EnableDelayedExpansion

:: Add MSYS2 MinGW64 to PATH
set "PATH=C:\msys64\mingw64\bin;%PATH%"

:: Check if required tools are available
where gcc >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Error: gcc not found in PATH. Ensure MSYS2 MinGW64 is installed at C:\msys64\mingw64.
    exit /b 1
)
where g++ >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Error: g++ not found in PATH. Ensure MSYS2 MinGW64 is installed at C:\msys64\mingw64.
    exit /b 1
)
where cmake >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Error: cmake not found in PATH. Install CMake and add it to PATH.
    exit /b 1
)
where mingw32-make >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Error: mingw32-make not found in PATH. Ensure MSYS2 MinGW64 is installed.
    exit /b 1
)

:: Create build directory if it doesn't exist
if not exist "build" (
    mkdir build
    if %ERRORLEVEL% NEQ 0 (
        echo Error: Failed to create build directory.
        exit /b 1
    )
)

:: Change to build directory
cd build
if %ERRORLEVEL% NEQ 0 (
    echo Error: Failed to change to build directory.
    exit /b 1
)

:: Configure with CMake, forcing MinGW Makefiles and specifying compilers
cmake -B . -S .. -G "MinGW Makefiles" ^
    -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake ^
    -DCMAKE_C_COMPILER=C:/msys64/mingw64/bin/gcc.exe ^
    -DCMAKE_CXX_COMPILER=C:/msys64/mingw64/bin/g++.exe ^
    -DCMAKE_RC_COMPILER=C:/msys64/mingw64/bin/windres.exe ^
    -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% NEQ 0 (
    echo Error: CMake configuration failed.
    exit /b 1
)

:: Build the project
mingw32-make
if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    exit /b 1
)

:: Build succeeded, proceed with DLL copying
echo Build completed successfully!
echo Copying required DLLs...

:: List of required DLLs (updated based on your dependencies)
set "DLL_LIST=libstdc++-6.dll libgcc_s_seh-1.dll libwinpthread-1.dll libcurl-4.dll zlib1.dll libcrypto-3-x64.dll libssl-3-x64.dll libwx_baseu-3.2-0.dll libwx_mswu_core-3.2-0.dll libssh2-1.dll libidn2-0.dll libunistring-5.dll libintl-8.dll libiconv-2.dll libzstd.dll libbrotlidec.dll libbrotlicommon.dll libnghttp2-14.dll libssp-0.dll libyaml-cpp-0.8.dll libarchive-19.dll libmongoc-1.0-0.dll libbson-1.0-0.dll libmongocxx-3.10.dll libbsoncxx-3.10.dll python39.dll"

:: Copy DLLs from MSYS2 MinGW64
for %%F in (%DLL_LIST%) do (
    if exist "C:\msys64\mingw64\bin\%%~F" (
        copy "C:\msys64\mingw64\bin\%%~F" . >nul
        if !ERRORLEVEL! EQU 0 (
            echo Copied %%~F successfully
        ) else (
            echo Warning: Failed to copy %%~F
        )
    ) else (
        echo Warning: Could not find %%~F in C:\msys64\mingw64\bin
    )
)

echo.
echo All operations completed successfully!

endlocal
exit /b 0