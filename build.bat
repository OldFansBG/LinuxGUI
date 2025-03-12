@echo off
setlocal

:: Ensure build directory exists
if not exist "build" mkdir build

:: Add MSYS2 to PATH
set PATH=C:\msys64\mingw64\bin;%PATH%

cd build

:: Configure with CMake (source dir is parent of build)
cmake .. -G "MinGW Makefiles" ^
    -DCMAKE_C_COMPILER=C:/msys64/mingw64/bin/gcc.exe ^
    -DCMAKE_CXX_COMPILER=C:/msys64/mingw64/bin/g++.exe ^
    -DCMAKE_PREFIX_PATH=C:/msys64/mingw64

:: Build
mingw32-make

:: Check if build was successful
if %ERRORLEVEL% EQU 0 (
    echo Build completed successfully!
    echo Copying required DLLs...

    :: Copy standard DLLs from MSYS2
    for %%F in (
        "libstdc++-6.dll"
        "libgcc_s_seh-1.dll"
        "libwinpthread-1.dll"
        "libjsoncpp-26.dll"
        "libcurl-4.dll"
        "zlib1.dll"
        "libcrypto-3-x64.dll"
        "libssl-3-x64.dll"
        "libwx_baseu-3.0.dll"
        "libwx_mswu_core-3.0.dll"
        "libssh2-1.dll"
        "libidn2-0.dll"
        "libunistring-5.dll"
        "libintl-8.dll"
        "libiconv-2.dll"
        "libzstd.dll"
        "libbrotlidec.dll"
        "libbrotlicommon.dll"
        "libnghttp2-14.dll"
        "libssp-0.dll"
    ) do (
        if exist "C:\msys64\mingw64\bin\%%~F" (
            copy "C:\msys64\mingw64\bin\%%~F" . > nul && echo Copied %%~F successfully
        ) else (
            echo Warning: Could not find %%~F
        )
    )

    echo.
    echo All files are in: %CD%
) else (
    echo Build failed!
    exit /b 1
)

endlocal