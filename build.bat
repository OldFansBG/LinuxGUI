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
if %ERRORLEVEL% EQU 0 (
    echo Build completed successfully!
    echo Copying required DLLs...

    :: Copy Python 3.9 DLL from Anaconda environment
    if exist "C:\Users\PC\anaconda3\envs\myenv\python39.dll" (
        copy "C:\Users\PC\anaconda3\envs\myenv\python39.dll" . > nul && echo Copied python39.dll successfully
    ) else (
        echo Error: python39.dll not found in Anaconda environment!
        exit /b 1
    )

    :: Copy standard DLLs from MSYS2
    for %%F in (
        "libstdc++-6.dll"
        "libgcc_s_seh-1.dll"
        "libwinpthread-1.dll"
        "libcurl-4.dll"
        "zlib1.dll"
        "libcrypto-3-x64.dll"
        "libssl-3-x64.dll"
        "libwx_baseu-3.0.dll"
        "libwx_mswu_core-3.0.dll"
        "libarchive-13.dll"
        "libssh2-1.dll"
        "libidn2-0.dll"
        "libunistring-5.dll"
        "libintl-8.dll"
        "libiconv-2.dll"
        "libzstd.dll"
        "libbrotlidec.dll"
        "libbrotlicommon.dll"
        "libnghttp2-14.dll"
        "libnghttp3-9.dll" 
        "libpsl-5.dll"      
        "libssp-0.dll"
    ) do (
        if exist "%MSYS2_BIN%\%%~F" (
            copy "%MSYS2_BIN%\%%~F" . > nul && echo Copied %%~F successfully
        ) else (
            echo Warning: Could not find %%~F
        )
    )

    :: Copy Python standard library from Anaconda environment
    if exist "C:\Users\PC\anaconda3\envs\myenv\Lib" (
        xcopy /E /I /Y "C:\Users\PC\anaconda3\envs\myenv\Lib" "Lib" > nul && echo Copied Python standard library
    ) else (
        echo Error: Python standard library not found in Anaconda environment!
        exit /b 1
    )

    echo.
    echo All operations completed.
) else (
    echo Build failed!
    exit /b 1
)

endlocal