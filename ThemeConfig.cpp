#include "ThemeConfig.h"
#include <fstream>
#include "CustomTitleBar.h"
ThemeConfig& ThemeConfig::Get() {
    static ThemeConfig instance;
    return instance;
}

void ThemeConfig::LoadThemes(const wxString& filename) {
    std::ifstream fin(filename.ToStdString());
    if (fin.is_open()) {
        m_themes = YAML::Load(fin);
    }
}

wxColour ThemeConfig::HexToColor(const std::string& hex) const {
    unsigned int r, g, b;
    sscanf(hex.c_str(), "#%02x%02x%02x", &r, &g, &b);
    return wxColour(r, g, b);
}

void ThemeConfig::ApplyTheme(wxWindow* window, const wxString& themeName) {
    if (!m_themes[themeName.ToStdString()]) {
        wxLogError("Theme not found: %s", themeName);
        return;
    }
    
    const auto& theme = m_themes[themeName.ToStdString()];
    if (CustomTitleBar* titleBar = dynamic_cast<CustomTitleBar*>(window)) {
        wxColour color = HexToColor(theme["titleBar"].as<std::string>());
        titleBar->SetBackgroundColour(color);
    }
    
    window->Refresh();
    window->Update();
}

wxColour ThemeConfig::GetTitleBarColor(const wxString& themeName) const {
    if (m_themes[themeName.ToStdString()]) {
        const auto& theme = m_themes[themeName.ToStdString()];
        return HexToColor(theme["titleBar"].as<std::string>());
    }
    return *wxBLUE;  // fallback color
}