#include "ThemeConfig.h"
#include <fstream>
#include <wx/button.h>
#include <wx/textctrl.h>
#include <wx/stattext.h>
#include <wx/gauge.h>
#include "CustomTitleBar.h"
#include "CustomStatusBar.h"

ThemeConfig& ThemeConfig::Get() {
    static ThemeConfig instance;
    return instance;
}

void ThemeConfig::LoadThemes(const wxString& filename) {
    std::ifstream fin(filename.ToStdString());
    if (!fin.is_open()) {
        wxLogError("Failed to open theme file: %s", filename);
        return;
    }

    try {
        m_themes = YAML::Load(fin);
        m_themeColors.clear();

        for (const auto& theme : m_themes) {
            wxString themeName = wxString::FromUTF8(theme.first.as<std::string>());
            ThemeColors colors;
            ParseThemeColors(theme.second, colors);
            m_themeColors[themeName.Lower()] = colors;
        }
    }
    catch (const YAML::Exception& e) {
        wxLogError("Error parsing theme file: %s", e.what());
    }
}

wxColour ThemeConfig::ParseColor(const YAML::Node& node) const {
    if (!node.IsScalar()) return *wxBLACK;
    
    std::string hexColor = node.as<std::string>();
    if (hexColor.empty() || hexColor[0] != '#') return *wxBLACK;
    
    unsigned int r, g, b;
    sscanf(hexColor.c_str() + 1, "%02x%02x%02x", &r, &g, &b);
    return wxColour(r, g, b);
}

void ThemeConfig::ParseThemeColors(const YAML::Node& themeNode, ThemeColors& colors) {
    colors.titleBar = ParseColor(themeNode["titleBar"]);
    colors.statusBar = ParseColor(themeNode["statusBar"]);
    colors.background = ParseColor(themeNode["background"]);
    colors.text = ParseColor(themeNode["text"]);
    colors.secondaryText = ParseColor(themeNode["secondaryText"]);
    colors.border = ParseColor(themeNode["border"]);
    
    const auto& buttonNode = themeNode["button"];
    if (buttonNode) {
        colors.button.background = ParseColor(buttonNode["background"]);
        colors.button.hover = ParseColor(buttonNode["hover"]);
        colors.button.pressed = ParseColor(buttonNode["pressed"]);
        colors.button.text = ParseColor(buttonNode["text"]);
    }
    
    const auto& inputNode = themeNode["input"];
    if (inputNode) {
        colors.input.background = ParseColor(inputNode["background"]);
        colors.input.border = ParseColor(inputNode["border"]);
        colors.input.text = ParseColor(inputNode["text"]);
        colors.input.placeholder = ParseColor(inputNode["placeholder"]);
    }
    
    const auto& gaugeNode = themeNode["gauge"];
    if (gaugeNode) {
        colors.gauge.background = ParseColor(gaugeNode["background"]);
        colors.gauge.fill = ParseColor(gaugeNode["fill"]);
    }
    
    const auto& panelNode = themeNode["panel"];
    if (panelNode) {
        colors.panel.background = ParseColor(panelNode["background"]);
        colors.panel.border = ParseColor(panelNode["border"]);
    }
}

void ThemeConfig::ApplyTheme(wxWindow* window, const wxString& themeName) {
    wxString lowerThemeName = themeName.Lower();
    if (!m_themeColors.count(lowerThemeName)) {
        wxLogError("Theme not found: %s", themeName);
        return;
    }

    const ThemeColors& colors = m_themeColors[lowerThemeName];
    ApplyColorsRecursively(window, colors);
}

void ThemeConfig::ApplyColorsRecursively(wxWindow* window, const ThemeColors& colors) {
    if (!window) return;

    ApplyThemeToControl(window, colors);

    wxWindowList& children = window->GetChildren();
    for (wxWindow* child : children) {
        ApplyColorsRecursively(child, colors);
    }
}

void ThemeConfig::ApplyThemeToControl(wxWindow* control, const ThemeColors& colors) {
    if (!control) return;

    if (auto* titleBar = dynamic_cast<CustomTitleBar*>(control)) {
        titleBar->SetBackgroundColour(colors.titleBar);
    }
    else if (auto* statusBar = dynamic_cast<CustomStatusBar*>(control)) {
        statusBar->SetBackgroundColour(colors.statusBar);
    }
    else if (auto* button = dynamic_cast<wxButton*>(control)) {
        button->SetBackgroundColour(colors.button.background);
        button->SetForegroundColour(colors.button.text);
    }
    else if (auto* textCtrl = dynamic_cast<wxTextCtrl*>(control)) {
        textCtrl->SetBackgroundColour(colors.input.background);
        textCtrl->SetForegroundColour(colors.input.text);
    }
    else if (auto* staticText = dynamic_cast<wxStaticText*>(control)) {
        staticText->SetForegroundColour(colors.text);
    }
    else if (auto* gauge = dynamic_cast<wxGauge*>(control)) {
        gauge->SetBackgroundColour(colors.gauge.background);
        gauge->SetForegroundColour(colors.gauge.fill);
    }
    else if (auto* panel = dynamic_cast<wxPanel*>(control)) {
        panel->SetBackgroundColour(colors.panel.background);
    }

    control->Refresh();
}

bool ThemeConfig::HasTheme(const wxString& themeName) const {
    return m_themeColors.count(themeName.Lower()) > 0;
}

wxArrayString ThemeConfig::GetAvailableThemes() const {
    wxArrayString themes;
    for (const auto& [name, _] : m_themeColors) {
        themes.Add(name);
    }
    return themes;
}

const ThemeColors& ThemeConfig::GetThemeColors(const wxString& themeName) const {
    static const ThemeColors defaultColors;
    wxString lowerThemeName = themeName.Lower();
    auto it = m_themeColors.find(lowerThemeName);
    return it != m_themeColors.end() ? it->second : defaultColors;
}