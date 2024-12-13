#ifndef THEME_SYSTEM_H
#define THEME_SYSTEM_H

#include <wx/wx.h>
#include <memory>
#include <vector>
#include <functional>
#include <map>
#include "ThemeRoles.h"  // Change forward declaration to direct include
// Forward declare ColorRole enum
enum class ColorRole;

class ThemeSystem {
public:
    virtual ~ThemeSystem() = default;

    enum class ThemeVariant {
        Light,
        Dark,
        HighContrastLight,
        HighContrastDark,
        Custom
    };

    struct ThemeColors {
        std::map<ColorRole, wxColour> colors;
        std::map<ColorRole, wxColour> rawColors; 
        
        ThemeColors& set(ColorRole role, const wxColour& color) {
            rawColors[role] = color;
            colors[role] = color;
            return *this;
        }
        
        wxColour get(ColorRole role) const {
            auto it = colors.find(role);
            return it != colors.end() ? it->second : wxColour(0, 0, 0);
        }
    };

    static ThemeSystem& Get() {
        static ThemeSystem instance;
        return instance;
    }

    bool IsDarkMode() const;
    bool IsHighContrastMode() const;
    void DetectSystemTheme();
    void ApplyTheme(ThemeVariant variant);
    void SaveThemePreference();
    void LoadThemePreference();

    wxColour GetColor(ColorRole role) const;
    void SetColor(ColorRole role, const wxColour& color);
    void ResetColors();
    
    void StyleControl(wxWindow* window);
    void RegisterControl(wxWindow* window);
    void UnregisterControl(wxWindow* window);
    
    void EnableAnimations(bool enable) { m_animationsEnabled = enable; }
    void SetTransitionDuration(int milliseconds) { m_transitionDuration = milliseconds; }
    
    bool AreAnimationsEnabled() const { return m_animationsEnabled; }
    int GetTransitionDuration() const { return m_transitionDuration; }
    ThemeVariant GetCurrentTheme() const { return m_currentTheme; }

    ThemeColors CreateLightTheme();
    ThemeColors CreateDarkTheme();
    ThemeColors CreateHighContrastLightTheme();
    ThemeColors CreateHighContrastDarkTheme();

    void RegisterCustomTheme(const std::string& name, const ThemeColors& colors);
    void ApplyCustomTheme(const std::string& name);

    using ThemeChangeCallback = std::function<void(ThemeVariant)>;
    void AddThemeChangeListener(void* owner, ThemeChangeCallback callback);
    void RemoveThemeChangeListener(void* owner);

private:
    ThemeSystem();
    ThemeSystem(const ThemeSystem&) = delete;
    ThemeSystem& operator=(const ThemeSystem&) = delete;

    void InitializePlatformTheme();
    void RegisterSystemThemeListener();
    void UpdateSystemTitlebar(wxWindow* window);
    wxColour AdjustColorForContrast(const wxColour& color, const wxColour& against);
    wxColour InterpolateColors(const wxColour& start, const wxColour& end, float progress);

    void NotifyThemeChanged();
    void ProcessPendingUpdates();
    void StartColorTransition(const ThemeColors& target);

    ThemeVariant m_currentTheme;
    ThemeColors m_colors;
    bool m_animationsEnabled;
    int m_transitionDuration;
    std::vector<wxWindow*> m_registeredControls;
    std::map<std::string, ThemeColors> m_customThemes;
    std::map<void*, ThemeChangeCallback> m_themeChangeListeners;
};

namespace ThemeUtils {
    wxColour Lighten(const wxColour& color, int amount);
    wxColour Darken(const wxColour& color, int amount);
    wxColour WithAlpha(const wxColour& color, unsigned char alpha);
    bool HasSufficientContrast(const wxColour& fg, const wxColour& bg);
    float GetPerceivedBrightness(const wxColour& color);
}

inline ThemeSystem& Theme() { return ThemeSystem::Get(); }

#endif // THEME_SYSTEM_H