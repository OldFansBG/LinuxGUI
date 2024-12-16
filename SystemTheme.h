#ifndef SYSTEMTHEME_H
#define SYSTEMTHEME_H

#include <wx/wx.h>
#include "ThemeConfig.h"

#ifdef __WXMSW__
#include <windows.h>
#endif

class SystemTheme {
public:
    static bool IsDarkMode() {
#ifdef __WXMSW__
        // Windows dark mode detection
        HKEY hKey;
        const wchar_t* subKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";
        const wchar_t* valueName = L"AppsUseLightTheme";
        DWORD value = 0;
        DWORD size = sizeof(value);
        
        if (RegOpenKeyExW(HKEY_CURRENT_USER, subKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            if (RegQueryValueExW(hKey, valueName, NULL, NULL, 
                               reinterpret_cast<LPBYTE>(&value), &size) == ERROR_SUCCESS) {
                RegCloseKey(hKey);
                return value == 0; // 0 means dark theme
            }
            RegCloseKey(hKey);
        }
#elif defined(__WXGTK__)
        // Linux dark mode detection
        wxString cmd = "gsettings get org.gnome.desktop.interface color-scheme";
        wxArrayString output;
        wxExecute(cmd, output, wxEXEC_SYNC);
        
        if (!output.IsEmpty()) {
            wxString result = output[0].Lower();
            return result.Contains("dark");
        }
#endif
        return false; // Default to light theme if detection fails
    }

    static void RegisterForThemeChanges(wxWindow* window) {
#ifdef __WXMSW__
        window->Bind(wxEVT_SYS_COLOUR_CHANGED, &SystemTheme::OnSystemThemeChanged);
#endif
    }

private:
#ifdef __WXMSW__
    static void OnSystemThemeChanged(wxSysColourChangedEvent& event) {
        ThemeConfig::Get().UpdateSystemTheme();
        event.Skip();
    }
#endif
};

#endif // SYSTEMTHEME_H