#ifndef THEMECONFIG_H
#define THEMECONFIG_H

#include <wx/wx.h>
#include <yaml-cpp/yaml.h>
#include <string>
#include <map>
#include "WindowIDs.h"

struct ThemeColors {
    struct ButtonColors {
        wxColour background;
        wxColour hover;
        wxColour pressed;
        wxColour text;
    };

    struct InputColors {
        wxColour background;
        wxColour border;
        wxColour text;
        wxColour placeholder;
    };

    struct GaugeColors {
        wxColour background;
        wxColour fill;
    };

    struct PanelColors {
        wxColour background;
        wxColour border;
    };

    wxColour titleBar;
    wxColour statusBar;
    wxColour background;
    wxColour text;
    wxColour secondaryText;
    wxColour border;
    ButtonColors button;
    ButtonColors extractButton;
    InputColors input;
    GaugeColors gauge;
    PanelColors panel;
};

class ThemeConfig {
public:
    static ThemeConfig& Get();
    
    void LoadThemes(const wxString& filename = "themes.json");
    void ApplyTheme(wxWindow* window, const wxString& themeName);
    void ApplyThemeToControl(wxWindow* control, const ThemeColors& colors);
    const ThemeColors& GetThemeColors(const wxString& themeName) const;
    
    bool HasTheme(const wxString& themeName) const;
    wxArrayString GetAvailableThemes() const;
    wxString GetCurrentTheme() const { return m_currentTheme; }
    void SetCurrentTheme(const wxString& themeName) { m_currentTheme = themeName.Lower(); }

private:
    ThemeConfig() = default;
    ThemeConfig(const ThemeConfig&) = delete;
    ThemeConfig& operator=(const ThemeConfig&) = delete;

    wxColour ParseColor(const YAML::Node& node) const;
    void ParseThemeColors(const YAML::Node& themeNode, ThemeColors& colors);
    void ApplyColorsRecursively(wxWindow* window, const ThemeColors& colors);
    void ParseButtonColors(const YAML::Node& buttonNode, ThemeColors::ButtonColors& colors);

    YAML::Node m_themes;
    std::map<wxString, ThemeColors> m_themeColors;
    wxString m_currentTheme = "light";
};

#endif // THEMECONFIG_H