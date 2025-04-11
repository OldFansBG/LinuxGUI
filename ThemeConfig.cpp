#include "ThemeConfig.h"
#include <fstream>
#include <wx/button.h>
#include <wx/textctrl.h>
#include <wx/stattext.h>
#include <wx/gauge.h>
#include <wx/statbox.h>
#include "CustomTitleBar.h" // Includes MyButton definition
#include "CustomStatusBar.h"
#include "SystemTheme.h"
#include "SettingsDialog.h" // Include ToggleSwitch if needed here

#ifdef __WXMSW__
#include <windows.h>
#endif

// ... (Get, LoadThemes, Parse* methods remain the same as the previous good version) ...
ThemeConfig &ThemeConfig::Get()
{
    static ThemeConfig instance;
    return instance;
}

void ThemeConfig::LoadThemes(const wxString &filename)
{
    std::ifstream fin(filename.ToStdString());
    if (!fin.is_open())
    {
        wxLogWarning("Failed to open theme file: %s", filename);
        m_themeColors.clear();
        // Consider adding default themes programmatically here as fallback
        return;
    }

    try
    {
        m_themes = YAML::Load(fin);
        m_themeColors.clear();

        wxLogDebug("Loading themes from file: %s", filename);
        for (const auto &theme : m_themes)
        {
            wxString themeName = wxString::FromUTF8(theme.first.as<std::string>());
            ThemeColors colors;
            ParseThemeColors(theme.second, colors);
            m_themeColors[themeName.Lower()] = colors;
            wxLogDebug("Loaded theme: %s", themeName);
        }
        // Ensure default themes exist
        if (m_themeColors.find("light") == m_themeColors.end())
        {
            wxLogWarning("Default 'light' theme missing in %s", filename);
        }
        if (m_themeColors.find("dark") == m_themeColors.end())
        {
            wxLogWarning("Default 'dark' theme missing in %s", filename);
        }
    }
    catch (const YAML::Exception &e)
    {
        wxLogError("Error parsing theme file '%s': %s", filename, e.what());
    }
}

void ThemeConfig::ParseButtonColors(const YAML::Node &buttonNode, ThemeColors::ButtonColors &colors)
{
    if (buttonNode)
    {
        colors.background = ParseColor(buttonNode["background"]);
        colors.hover = ParseColor(buttonNode["hover"]);
        colors.pressed = ParseColor(buttonNode["pressed"]);
        colors.text = ParseColor(buttonNode["text"]);
    }
    else
    {
        colors.background = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
        colors.text = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT);
        colors.hover = colors.background;
        colors.pressed = colors.background;
    }
}

wxColour ThemeConfig::ParseColor(const YAML::Node &node) const
{
    wxColour defaultColour = *wxBLACK;
    if (!node || !node.IsScalar())
        return defaultColour;
    std::string hexColor = node.as<std::string>();
    if (hexColor.empty() || hexColor[0] != '#' || (hexColor.length() != 7 && hexColor.length() != 9))
    {
        wxLogWarning("Invalid color format: '%s'. Expected #RRGGBB.", hexColor);
        return defaultColour;
    }
    unsigned int r, g, b;
#ifdef _WIN32
    int result = sscanf_s(hexColor.c_str() + 1, "%02x%02x%02x", &r, &g, &b);
#else
    int result = sscanf(hexColor.c_str() + 1, "%02x%02x%02x", &r, &g, &b);
#endif
    if (result == 3)
    {
        return wxColour(r, g, b);
    }
    else
    {
        wxLogWarning("Failed to parse color: '%s'", hexColor);
        return defaultColour;
    }
}

void ThemeConfig::ParseThemeColors(const YAML::Node &themeNode, ThemeColors &colors)
{
    colors.titleBar = ParseColor(themeNode["titleBar"]);
    colors.statusBar = ParseColor(themeNode["statusBar"]);
    colors.background = ParseColor(themeNode["background"]);
    colors.text = ParseColor(themeNode["text"]);
    colors.secondaryText = ParseColor(themeNode["secondaryText"]);
    colors.border = ParseColor(themeNode["border"]);
    ParseButtonColors(themeNode["button"], colors.button);
    ParseButtonColors(themeNode["extractButton"], colors.extractButton);
    const auto &inputNode = themeNode["input"];
    if (inputNode)
    {
        colors.input.background = ParseColor(inputNode["background"]);
        colors.input.border = ParseColor(inputNode["border"]);
        colors.input.text = ParseColor(inputNode["text"]);
        colors.input.placeholder = ParseColor(inputNode["placeholder"]);
    }
    else
    { // Fallbacks
        colors.input.background = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
        colors.input.text = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
        colors.input.border = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW);
        colors.input.placeholder = wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT);
    }
    const auto &gaugeNode = themeNode["gauge"];
    if (gaugeNode)
    {
        colors.gauge.background = ParseColor(gaugeNode["background"]);
        colors.gauge.fill = ParseColor(gaugeNode["fill"]);
    }
    else
    { // Fallbacks
        colors.gauge.background = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
        colors.gauge.fill = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
    }
    const auto &panelNode = themeNode["panel"];
    if (panelNode)
    {
        colors.panel.background = ParseColor(panelNode["background"]);
        colors.panel.border = ParseColor(panelNode["border"]);
    }
    else
    { // Fallbacks
        colors.panel.background = colors.background;
        colors.panel.border = colors.border;
    }
}

void ThemeConfig::ApplyTheme(wxWindow *window, const wxString &themeName)
{
    wxString lowerThemeName = themeName.Lower();
    auto it = m_themeColors.find(lowerThemeName);
    if (it == m_themeColors.end())
    {
        wxLogError("Theme not found: %s. Applying default 'light' theme.", themeName);
        lowerThemeName = "light";
        it = m_themeColors.find(lowerThemeName);
        if (it == m_themeColors.end())
        {
            wxLogError("Default 'light' theme also not found! Cannot apply theme.");
            return;
        }
    }

    const ThemeColors &colors = it->second;
    m_currentTheme = lowerThemeName; // Update global current theme state

    if (window)
    {
        ApplyColorsRecursively(window, colors);
        window->Refresh();
        window->Update();
    }
}

void ThemeConfig::ApplyColorsRecursively(wxWindow *window, const ThemeColors &colors)
{
    if (!window)
        return;

    ApplyThemeToControl(window, colors);

    wxWindowList &children = window->GetChildren();
    for (wxWindow *child : children)
    {
        // Apply recursively
        ApplyColorsRecursively(child, colors);
    }
}

void ThemeConfig::ApplyThemeToControl(wxWindow *control, const ThemeColors &colors)
{
    if (!control)
        return;

    // Set default background and foreground for most controls
    // Use panel background as the default BG for many controls
    control->SetBackgroundColour(colors.panel.background);
    control->SetForegroundColour(colors.text); // Default text color

    // --- Specific Control Styling ---

    if (auto *frame = dynamic_cast<wxFrame *>(control))
    {
        frame->SetBackgroundColour(colors.background); // Main window background
    }
    else if (auto *dialog = dynamic_cast<wxDialog *>(control))
    {
        dialog->SetBackgroundColour(colors.background); // Dialog background
    }
    else if (auto *titleBar = dynamic_cast<CustomTitleBar *>(control))
    {
        titleBar->SetBackgroundColour(colors.titleBar);
        // Theme the text inside the title bar
        if (wxStaticText *titleText = titleBar->GetTitleTextWidget())
        {
            titleText->SetForegroundColour(colors.text);     // Use primary text color for title
            titleText->SetBackgroundColour(colors.titleBar); // Match title bar BG
        }
        // Refresh the internal buttons (they will pick up theme in their OnPaint)
        titleBar->RefreshButtons();
        titleBar->Refresh(); // Refresh the title bar itself
    }
    else if (auto *statusBar = dynamic_cast<CustomStatusBar *>(control))
    {
        statusBar->SetBackgroundColour(colors.statusBar);
        // Theme text inside status bar (assuming CustomStatusBar has a method or direct access)
        // statusBar->GetStatusTextWidget()->SetForegroundColour(colors.text); // Example
        // statusBar->GetStatusTextWidget()->SetBackgroundColour(colors.statusBar);
        statusBar->Refresh();
    }
    else if (auto *button = dynamic_cast<wxButton *>(control))
    {
        // Exclude buttons inside CustomTitleBar if MyButton handles itself
        if (!dynamic_cast<CustomTitleBar *>(control->GetParent()))
        {
            if (control->GetId() == ID_NEXT)
            { // Specific button ID
                button->SetBackgroundColour(colors.extractButton.background);
                button->SetForegroundColour(colors.extractButton.text);
            }
            else
            { // Generic buttons
                button->SetBackgroundColour(colors.button.background);
                button->SetForegroundColour(colors.button.text);
            }
        }
    }
    else if (auto *myButton = dynamic_cast<MyButton *>(control))
    {
        // MyButton now gets theme colors in its OnPaint, just needs refresh
        myButton->Refresh();
    }
    else if (auto *textCtrl = dynamic_cast<wxTextCtrl *>(control))
    {
        textCtrl->SetBackgroundColour(colors.input.background);
        textCtrl->SetForegroundColour(colors.input.text);
    }
    else if (auto *staticText = dynamic_cast<wxStaticText *>(control))
    {
        // Make labels use secondary text color, unless it's the title bar text
        if (!dynamic_cast<CustomTitleBar *>(control->GetParent()))
        {
            staticText->SetForegroundColour(colors.secondaryText);
            // Match background of its parent panel
            wxWindow *parent = control->GetParent();
            if (parent)
            {
                control->SetBackgroundColour(parent->GetBackgroundColour());
            }
        }
        else
        {
            // Title bar text is handled when styling CustomTitleBar
            control->SetForegroundColour(colors.text);
            control->SetBackgroundColour(colors.titleBar);
        }
    }
    else if (auto *gauge = dynamic_cast<wxGauge *>(control))
    {
        // Note: Gauge appearance is heavily OS-dependent
        // Try setting both FG and BG, but it might not work as expected visually
        gauge->SetBackgroundColour(colors.gauge.background);
        gauge->SetForegroundColour(colors.gauge.fill);
    }
    else if (auto *staticBox = dynamic_cast<wxStaticBox *>(control))
    {
        staticBox->SetForegroundColour(colors.secondaryText); // Label color
        // Background is usually system-drawn, but set it anyway
        staticBox->SetBackgroundColour(colors.panel.background);
    }
    else if (auto *choice = dynamic_cast<wxChoice *>(control))
    {
        choice->SetBackgroundColour(colors.input.background);
        choice->SetForegroundColour(colors.input.text);
    }
    else if (auto *checkbox = dynamic_cast<wxCheckBox *>(control))
    {
        checkbox->SetForegroundColour(colors.text); // Text label color
                                                    // Checkbox BG usually matches parent
        wxWindow *parent = control->GetParent();
        if (parent)
        {
            checkbox->SetBackgroundColour(parent->GetBackgroundColour());
        }
    }
    else if (auto *toggle = dynamic_cast<ToggleSwitch *>(control))
    {
        // Toggle switch redraws itself using theme colors in OnPaint
        toggle->Refresh();
    }
    // Handle generic panels that aren't special types
    else if (auto *panel = dynamic_cast<wxPanel *>(control))
    {
        // Only apply if it's not one of the special panels already handled
        if (!dynamic_cast<CustomTitleBar *>(control) && !dynamic_cast<CustomStatusBar *>(control))
        {
            panel->SetBackgroundColour(colors.panel.background);
        }
    }

    // Don't refresh every single control, refresh the top-level window once
    // control->Refresh();
}

// ... (HasTheme, GetAvailableThemes, ParseThemeColors remain the same) ...
bool ThemeConfig::HasTheme(const wxString &themeName) const
{
    return m_themeColors.count(themeName.Lower()) > 0;
}

wxArrayString ThemeConfig::GetAvailableThemes() const
{
    wxArrayString themes;
    for (const auto &[name, _] : m_themeColors)
    {
        themes.Add(name);
    }
    if (std::find(themes.begin(), themes.end(), "light") == themes.end() && HasTheme("light"))
        themes.Add("light");
    if (std::find(themes.begin(), themes.end(), "dark") == themes.end() && HasTheme("dark"))
        themes.Add("dark");
    return themes;
}

const ThemeColors &ThemeConfig::GetThemeColors(const wxString &themeName) const
{
    static const ThemeColors defaultColors = []
    {
        ThemeColors dc;
        dc.background = wxSystemSettings::GetColour(wxSYS_COLOUR_APPWORKSPACE);
        dc.text = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
        dc.secondaryText = wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT);
        dc.border = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW);
        dc.titleBar = wxSystemSettings::GetColour(wxSYS_COLOUR_ACTIVECAPTION);
        dc.statusBar = wxSystemSettings::GetColour(wxSYS_COLOUR_MENUBAR);
        dc.button.background = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
        dc.button.text = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT);
        dc.extractButton = dc.button; // Default extract same as normal button
        dc.input.background = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
        dc.input.text = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
        dc.gauge.background = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
        dc.gauge.fill = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
        dc.panel.background = dc.background;
        return dc;
    }();

    wxString lowerThemeName = themeName.Lower();
    auto it = m_themeColors.find(lowerThemeName);
    if (it != m_themeColors.end())
    {
        return it->second;
    }
    else
    {
        wxLogWarning("GetThemeColors: Theme '%s' not found, returning default.", themeName);
        auto lightIt = m_themeColors.find("light");
        if (lightIt != m_themeColors.end())
        {
            return lightIt->second; // Prefer 'light' if 'themeName' is invalid
        }
        return defaultColors; // Absolute fallback
    }
}

bool ThemeConfig::IsSystemDarkMode() const
{
    return SystemTheme::IsDarkMode();
}

void ThemeConfig::UpdateSystemTheme()
{
    wxString systemThemeName = IsSystemDarkMode() ? "dark" : "light";
    wxLogInfo("System theme detected: %s", systemThemeName);

    if (!HasTheme(systemThemeName))
    {
        wxLogWarning("System theme '%s' is not defined. Using '%s'.", systemThemeName, m_currentTheme);
        systemThemeName = "light";
        if (!HasTheme(systemThemeName))
        {
            wxLogError("Fallback 'light' theme not found!");
            return;
        }
    }

    if (m_currentTheme != systemThemeName)
    {
        wxLogInfo("Applying system theme: %s", systemThemeName);
        m_currentTheme = systemThemeName;

        for (wxWindow *window : m_registeredWindows)
        {
            if (window)
            {
                window->CallAfter([this, window, systemThemeName]()
                                  {
                    if (window) { // Check again inside CallAfter
                       ApplyTheme(window, systemThemeName);
                    } });
            }
        }
    }
    else
    {
        wxLogInfo("System theme (%s) already applied.", systemThemeName);
    }
}

void ThemeConfig::RegisterWindow(wxWindow *window)
{
    if (window && std::find(m_registeredWindows.begin(), m_registeredWindows.end(), window) == m_registeredWindows.end())
    {
        m_registeredWindows.push_back(window);
        wxLogDebug("Registered window %p for theme updates.", window);
    }
}

void ThemeConfig::UnregisterWindow(wxWindow *window)
{
    auto it = std::find(m_registeredWindows.begin(), m_registeredWindows.end(), window);
    if (it != m_registeredWindows.end())
    {
        m_registeredWindows.erase(it);
        wxLogDebug("Unregistered window %p from theme updates.", window);
    }
}

void ThemeConfig::InitializeWithSystemTheme()
{
    LoadThemes();
    UpdateSystemTheme();
}

void ThemeConfig::SetCurrentTheme(const wxString &themeName)
{
    wxString lowerName = themeName.Lower();
    if (HasTheme(lowerName))
    {
        m_currentTheme = lowerName;
        wxLogInfo("Current theme set to: %s", m_currentTheme);
    }
    else
    {
        wxLogWarning("Attempted to set unknown theme: %s", themeName);
    }
}