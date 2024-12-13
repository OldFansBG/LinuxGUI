#ifndef PLATFORM_THEME_H
#define PLATFORM_THEME_H

#include <wx/wx.h>
#include "ThemeSystem.h"
#include <functional>

#ifdef __WXMSW__
    #include <windows.h>
    #include <dwmapi.h>
#endif

class PlatformTheme {
public:
    static bool IsDarkMode();
    static bool IsHighContrastMode();
    static void ApplyToWindow(wxWindow* window, bool isDark);
    static void RegisterThemeChangeCallback(std::function<void(bool)> callback);

#ifdef __WXMSW__
    static void EnableDarkMode(HWND hwnd, bool enable);
    static void ApplyMicaEffect(HWND hwnd);
    static void SetTitleBarColor(HWND hwnd, const wxColour& color);
    static void SetupWindowTheme(HWND hwnd);
    static BOOL CALLBACK EnumChildProc(HWND hwnd, LPARAM lParam);
    static bool SetupDarkModeColors(HWND hwnd);
    static void ApplyDarkModeToControl(HWND hwnd);
#endif

#ifdef __WXGTK__
    static void SetGtkTheme(bool isDark);
    static void SetupGtkSettings();
    static bool GetGtkThemePreference();
    static void MonitorGtkTheme();
#endif

#ifdef __WXOSX__
    static void SetupMacOSAppearance(bool isDark);
    static bool GetMacOSThemePreference();
    static void MonitorMacOSTheme();
#endif

private:
    static void MonitorSystemThemeChanges();
    static void OnSystemThemeChanged(bool isDark);
    
    static std::vector<std::function<void(bool)>> s_themeCallbacks;
    static bool s_isDarkMode;

    // Private constructor to prevent instantiation
    PlatformTheme() = delete;
    PlatformTheme(const PlatformTheme&) = delete;
    PlatformTheme& operator=(const PlatformTheme&) = delete;
};

#endif // PLATFORM_THEME_H