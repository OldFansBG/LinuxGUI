@echo off
setlocal enabledelayedexpansion

:: Initialize constants
set "OUTPUT_DIR=I:\Files\Desktop\LinuxGUI\iso"
set "LOG_FILE=%TEMP%\iso_copy.log"
set "CONTAINER_ID_FILE=%TEMP%\container_id.txt"
set "TEMP_ISO=%TEMP%\temp_custom.iso"
set "FINAL_ISO=%OUTPUT_DIR%\custom.iso"
set "MD5_FILE=%OUTPUT_DIR%\custom.iso.md5"
set "LOCK_FILE=%TEMP%\iso_copy.lock"

:: Check if another instance is running
if exist "%LOCK_FILE%" (
    echo Another instance is running, waiting...
    timeout /t 10 >nul
    if exist "%LOCK_FILE%" (
        echo Lock file still exists after timeout
        exit /b 1
    )
)

:: Create lock file
echo %date% %time% > "%LOCK_FILE%"

:: Initialize logging
call :log "Starting ISO copy process"
call :log "Waiting for system to stabilize..."
timeout /t 5 >nul

:: Stop Docker service
call :log "Stopping Docker service..."
net stop com.docker.service >nul 2>&1
timeout /t 5 >nul

:: Cleanup existing files with extra safety
call :log "Cleaning up existing files..."
if exist "%TEMP_ISO%" (
    call :safe_delete "%TEMP_ISO%"
)
if exist "%FINAL_ISO%" (
    call :safe_delete "%FINAL_ISO%"
)

:: Start Docker service
call :log "Starting Docker service..."
net start com.docker.service >nul 2>&1
timeout /t 10 >nul

:: Get container ID
if exist "%CONTAINER_ID_FILE%" (
    set /p CONTAINER_ID=<"%CONTAINER_ID_FILE%"
    call :log "Using container ID from file: !CONTAINER_ID!"
) else (
    for /f "tokens=*" %%i in ('docker ps -qf "ancestor=ubuntu"') do set "CONTAINER_ID=%%i"
    call :log "Found container ID: !CONTAINER_ID!"
)

if "!CONTAINER_ID!"=="" (
    call :error "No container found"
    goto :CLEANUP_END
)

:: Ensure output directory exists
if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%" 2>nul

:: Copy with retry mechanism
set max_attempts=3
set current_attempt=0

:COPY_RETRY
set /a current_attempt+=1
call :log "Copy attempt !current_attempt! of %max_attempts%"

:: Clear target before copy
if exist "%TEMP_ISO%" call :safe_delete "%TEMP_ISO%"

:: Wait before copy
timeout /t 5 >nul

:: Try to copy
call :log "Attempting copy from container..."
docker cp "!CONTAINER_ID!:/output/custom.iso" "%TEMP_ISO%" >"%TEMP%\copy_output.txt" 2>&1
if errorlevel 1 (
    type "%TEMP%\copy_output.txt" >> "%LOG_FILE%"
    if !current_attempt! lss %max_attempts% (
        call :log "Copy failed, waiting 15 seconds before retry..."
        timeout /t 15 >nul
        goto :COPY_RETRY
    )
    call :error "Failed to copy ISO after %max_attempts% attempts"
    goto :CLEANUP_END
)

:: Verify copy
if not exist "%TEMP_ISO%" (
    call :error "Copy command succeeded but file not found"
    goto :CLEANUP_END
)

:: Move file to final location
call :log "Moving ISO to final location..."
if exist "%FINAL_ISO%" call :safe_delete "%FINAL_ISO%"
timeout /t 5 >nul

move /y "%TEMP_ISO%" "%FINAL_ISO%" >nul 2>&1
if errorlevel 1 (
    call :error "Failed to move ISO to final location"
    goto :CLEANUP_END
)

:: Create MD5 checksum
call :log "Creating MD5 checksum..."
certutil -hashfile "%FINAL_ISO%" MD5 > "%MD5_FILE%" 2>nul

call :log "ISO copy completed successfully"
goto :CLEANUP_SUCCESS

:safe_delete
set "file_to_delete=%~1"
call :log "Attempting to delete: !file_to_delete!"
del /F /Q "!file_to_delete!" >nul 2>&1
if exist "!file_to_delete!" (
    timeout /t 5 >nul
    del /F /Q "!file_to_delete!" >nul 2>&1
)
if exist "!file_to_delete!" (
    call :log "Warning: Could not delete !file_to_delete!"
)
exit /b

:CLEANUP_SUCCESS
if exist "%LOCK_FILE%" del /F /Q "%LOCK_FILE%" >nul 2>&1
exit /b 0

:CLEANUP_END
if exist "%LOCK_FILE%" del /F /Q "%LOCK_FILE%" >nul 2>&1
if exist "%TEMP_ISO%" call :safe_delete "%TEMP_ISO%"
exit /b 1

:log
echo [%date% %time%] %*
echo [%date% %time%] %* >> "%LOG_FILE%"
exit /b 0

:error
echo [%date% %time%] ERROR: %*
echo [%date% %time%] ERROR: %* >> "%LOG_FILE%"
exit /b 0