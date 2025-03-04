#include "MainFrame.h"
#include <wx/wx.h>
#include <windows.h>
#include <string>

bool isDockerDesktopInstalled() {
    HKEY hKey;
    const wchar_t* regPath = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Docker Desktop";
    
    // Try to open the registry key
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, regPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return true;
    }
    
    // Also check current user in case it was installed for current user only
    if (RegOpenKeyExW(HKEY_CURRENT_USER, regPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return true;
    }
    
    return false;
}

bool isDockerEngineRunning() {
    const wchar_t* dockerPipe = L"\\\\.\\pipe\\docker_engine";
    
    HANDLE hPipe = CreateFileW(
        dockerPipe,          // Named pipe path
        GENERIC_READ | GENERIC_WRITE, // Access mode
        0,                   // No sharing
        nullptr,            // Default security attributes
        OPEN_EXISTING,      // Open existing pipe
        0,                  // Default attributes
        nullptr            // No template file
    );

    if (hPipe == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        if (error == ERROR_PIPE_BUSY) {
            return true;  // Docker is running but busy
        }
        return false;    // Docker is not running
    }

    CloseHandle(hPipe);
    return true;
}

class ISOAnalyzerApp : public wxApp {
public:
    bool OnInit() {
        wxInitAllImageHandlers();

        // First check if Docker Desktop is installed
        if (!isDockerDesktopInstalled()) {
            int answer = wxMessageBox(
                "Docker Desktop is not installed. This application requires Docker Desktop to function properly.\n\n"
                "Would you like to abort the application?",
                "Docker Not Found",
                wxYES_NO | wxICON_ERROR
            );
            
            if (answer == wxYES) {
                return false;  // Exit application
            }
            return true;  // Continue without Docker
        }

        // Then check if Docker is running
        if (!isDockerEngineRunning()) {
            int answer = wxMessageBox(
                "Docker Desktop is not running. The application requires Docker to function properly.\n\n"
                "Please start Docker Desktop and try again.\n\n"
                "Would you like to abort the application?",
                "Docker Not Running",
                wxYES_NO | wxICON_ERROR
            );
            
            if (answer == wxYES) {
                return false;  // Exit application
            }
        }

        MainFrame* frame = new MainFrame("LinuxISOPro");
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(ISOAnalyzerApp);