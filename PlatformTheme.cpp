#include "PlatformTheme.h"
#include <wx/settings.h>

#ifdef __WXMSW__
#ifndef DWMWA_MICA_EFFECT
#define DWMWA_MICA_EFFECT 1029
#endif
#endif

std::vector<std::function<void(bool)>> PlatformTheme::s_themeCallbacks;
bool PlatformTheme::s_isDarkMode = false;

bool PlatformTheme::IsDarkMode() {
#ifdef __WXMSW__
    HKEY hKey;
    DWORD value;
    DWORD size = sizeof(value);
    
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                     L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                     0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueExW(hKey, L"AppsUseDarkTheme", NULL, NULL,
                           reinterpret_cast<LPBYTE>(&value), &size) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return value == 1;
        }
        RegCloseKey(hKey);
    }
    return false;
#elif defined(__WXGTK__)
    return GetGtkThemePreference();
#elif defined(__WXOSX__)
    return GetMacOSThemePreference();
#else
    wxColour bg = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
    return ThemeUtils::GetPerceivedBrightness(bg) < 128;
#endif
}

bool PlatformTheme::IsHighContrastMode() {
#ifdef __WXMSW__
    HIGHCONTRAST hc;
    hc.cbSize = sizeof(HIGHCONTRAST);
    if (SystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof(HIGHCONTRAST), &hc, 0)) {
        return (hc.dwFlags & HCF_HIGHCONTRASTON) != 0;
    }
    return false;
#else
    wxColour bg = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
    float brightness = ThemeUtils::GetPerceivedBrightness(bg);
    return brightness < 0.1 || brightness > 0.9;
#endif
}

void PlatformTheme::ApplyToWindow(wxWindow* window, bool isDark) {
#ifdef __WXMSW__
    if (!window) return;
    
    HWND hwnd = (HWND)window->GetHandle();
    if (!hwnd) return;

    EnableDarkMode(hwnd, isDark);
    SetupWindowTheme(hwnd);
    
    EnumChildWindows(hwnd, EnumChildProc, (LPARAM)isDark);
    
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    
    RedrawWindow(hwnd, nullptr, nullptr,
                RDW_FRAME | RDW_INVALIDATE | RDW_ERASE | RDW_INTERNALPAINT | RDW_ALLCHILDREN);
#endif

    window->Refresh();
    window->Update();
}

#ifdef __WXMSW__
void PlatformTheme::EnableDarkMode(HWND hwnd, bool enable) {
    if (!hwnd) return;
    
    BOOL value = enable ? TRUE : FALSE;
    DwmSetWindowAttribute(hwnd, 19, &value, sizeof(value)); // DWMWA_USE_IMMERSIVE_DARK_MODE

    if (enable) {
        SetWindowTheme(hwnd, L"DarkMode_Explorer", nullptr);
    } else {
        SetWindowTheme(hwnd, L"Explorer", nullptr);
    }

    // Apply dark mode to common controls
    if (enable) {
        SetWindowTheme(hwnd, L"DarkMode_Explorer", nullptr);
        wchar_t className[256];
        GetClassName(hwnd, className, sizeof(className)/sizeof(wchar_t));
        
        if (wcscmp(className, L"Button") == 0 ||
            wcscmp(className, L"ComboBox") == 0 ||
            wcscmp(className, L"Edit") == 0 ||
            wcscmp(className, L"ListBox") == 0) {
            SetWindowTheme(hwnd, L"DarkMode_CFD", nullptr);
        }
    }
}

void PlatformTheme::ApplyMicaEffect(HWND hwnd) {
    if (!hwnd) return;
    
    BOOL value = TRUE;
    DwmSetWindowAttribute(hwnd, DWMWA_MICA_EFFECT, &value, sizeof(value));
}

void PlatformTheme::SetTitleBarColor(HWND hwnd, const wxColour& color) {
    if (!hwnd) return;
    
    COLORREF colorRef = RGB(color.Red(), color.Green(), color.Blue());
    DwmSetWindowAttribute(hwnd, DWMWA_CAPTION_COLOR, &colorRef, sizeof(COLORREF));
}

void PlatformTheme::SetupWindowTheme(HWND hwnd) {
    if (!hwnd) return;
    
    BOOL value = TRUE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
    
    // Remove window border
    LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
    style &= ~(WS_BORDER | WS_DLGFRAME | WS_THICKFRAME);
    SetWindowLongPtr(hwnd, GWL_STYLE, style);
    
    // Remove extended window styles
    LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
    exStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);
    SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle);
}

BOOL CALLBACK PlatformTheme::EnumChildProc(HWND hwnd, LPARAM lParam) {
    if (!hwnd) return TRUE;
    
    bool isDark = (BOOL)lParam == TRUE;
    EnableDarkMode(hwnd, isDark);
    ApplyDarkModeToControl(hwnd);
    return TRUE;
}

void PlatformTheme::ApplyDarkModeToControl(HWND hwnd) {
    if (!hwnd) return;
    
    wchar_t className[256];
    GetClassName(hwnd, className, sizeof(className)/sizeof(wchar_t));
    
    if (wcscmp(className, L"Button") == 0 ||
        wcscmp(className, L"Edit") == 0 ||
        wcscmp(className, L"ListBox") == 0 ||
        wcscmp(className, L"ComboBox") == 0 ||
        wcscmp(className, L"ScrollBar") == 0) {
        SetWindowTheme(hwnd, L"DarkMode_Explorer", nullptr);
    }
}

bool PlatformTheme::SetupDarkModeColors(HWND hwnd) {
    if (!hwnd) return false;
    
    HIGHCONTRAST hc = { sizeof(HIGHCONTRAST) };
    if (SystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof(HIGHCONTRAST), &hc, 0)) {
        if (hc.dwFlags & HCF_HIGHCONTRASTON) {
            return false;
        }
    }
    return true;
}
#endif

#ifdef __WXGTK__
void PlatformTheme::SetGtkTheme(bool isDark) {
    if (isDark) {
        system("gsettings set org.gnome.desktop.interface gtk-theme 'Adwaita-dark'");
    } else {
        system("gsettings set org.gnome.desktop.interface gtk-theme 'Adwaita'");
    }
}

bool PlatformTheme::GetGtkThemePreference() {
    wxArrayString output;
    wxExecute("gsettings get org.gnome.desktop.interface gtk-theme", output);
    if (!output.IsEmpty()) {
        return output[0].Contains("dark");
    }
    return false;
}

void PlatformTheme::MonitorGtkTheme() {
    // This would be implemented using GSettings signals in a real application
}
#endif

#ifdef __WXOSX__
bool PlatformTheme::GetMacOSThemePreference() {
    // This would need to be implemented using NSAppearance
    return false;
}

void PlatformTheme::MonitorMacOSTheme() {
    // This would be implemented using NSAppearance notifications
}
#endif

void PlatformTheme::RegisterThemeChangeCallback(std::function<void(bool)> callback) {
    s_themeCallbacks.push_back(callback);
    MonitorSystemThemeChanges();
}

void PlatformTheme::MonitorSystemThemeChanges() {
#ifdef __WXMSW__
    RegisterWindowMessageW(L"DWMCOLORIZATIONCOLORCHANGED");
    RegisterWindowMessageW(L"DWMCOMPOSITIONCHANGED");
    RegisterWindowMessageW(L"THEMECHANGED");
#elif defined(__WXGTK__)
    MonitorGtkTheme();
#elif defined(__WXOSX__)
    MonitorMacOSTheme();
#endif
}

void PlatformTheme::OnSystemThemeChanged(bool isDark) {
    s_isDarkMode = isDark;
    for (const auto& callback : s_themeCallbacks) {
        if (callback) {
            callback(isDark);
        }
    }
}