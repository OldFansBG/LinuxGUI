#ifndef THEMECONFIG_H
#define THEMECONFIG_H

#include <wx/wx.h>
#include <yaml-cpp/yaml.h>
#include <string>

class ThemeConfig {
public:
    static ThemeConfig& Get();
    
    void LoadThemes(const wxString& filename = "themes.json");
    void ApplyTheme(wxWindow* window, const wxString& themeName);
    wxColour GetTitleBarColor(const wxString& themeName) const;

private:
    ThemeConfig() {}
    YAML::Node m_themes;
    wxColour HexToColor(const std::string& hex) const;
};

#endif // THEMECONFIG_H