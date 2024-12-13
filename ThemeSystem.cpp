#include "ThemeSystem.h"
#include "PlatformTheme.h"
#include "ThemeRoles.h"
#include <wx/settings.h>
#include <wx/config.h>

ThemeSystem::ThemeSystem()
    : m_currentTheme(ThemeVariant::Light)
    , m_animationsEnabled(true)
    , m_transitionDuration(200) {
    InitializePlatformTheme();
}

void ThemeSystem::InitializePlatformTheme() {
    DetectSystemTheme();
    PlatformTheme::RegisterThemeChangeCallback([this](bool isDark) {
        ApplyTheme(isDark ? ThemeVariant::Dark : ThemeVariant::Light);
    });
}

void ThemeSystem::DetectSystemTheme() {
    bool isDark = PlatformTheme::IsDarkMode();
    bool isHighContrast = PlatformTheme::IsHighContrastMode();
    
    wxMessageBox(wxString::Format("Detected: isDark=%d, isHighContrast=%d", isDark, isHighContrast));
    
    if (isHighContrast) {
        m_currentTheme = isDark ? ThemeVariant::HighContrastDark : ThemeVariant::HighContrastLight;
    } else {
        m_currentTheme = isDark ? ThemeVariant::Dark : ThemeVariant::Light;
    }
    
    ApplyTheme(m_currentTheme);
}

void ThemeSystem::ApplyTheme(ThemeVariant variant) {
    ThemeColors newColors;
    switch (variant) {
        case ThemeVariant::Dark:
            newColors = CreateDarkTheme();
            break;
        case ThemeVariant::Light:
            newColors = CreateLightTheme();
            break;
        case ThemeVariant::HighContrastDark:
            newColors = CreateHighContrastDarkTheme();
            break;
        case ThemeVariant::HighContrastLight:
            newColors = CreateHighContrastLightTheme();
            break;
        default:
            return;
    }

    if (m_animationsEnabled) {
        StartColorTransition(newColors);
    } else {
        m_colors = newColors;
        NotifyThemeChanged();
    }

    m_currentTheme = variant;
    SaveThemePreference();
    
    for (auto window : m_registeredControls) {
        if (window) {
            PlatformTheme::ApplyToWindow(window, variant == ThemeVariant::Dark || 
                                               variant == ThemeVariant::HighContrastDark);
        }
    }
}

wxColour ThemeSystem::GetColor(ColorRole role) const {
    return m_colors.get(role);
}

void ThemeSystem::SetColor(ColorRole role, const wxColour& color) {
    m_colors.set(role, color);
    NotifyThemeChanged();
}

void ThemeSystem::RegisterControl(wxWindow* window) {
    if (window && std::find(m_registeredControls.begin(), 
                           m_registeredControls.end(), window) == m_registeredControls.end()) {
        m_registeredControls.push_back(window);
        StyleControl(window);
    }
}

void ThemeSystem::UnregisterControl(wxWindow* window) {
    auto it = std::find(m_registeredControls.begin(), m_registeredControls.end(), window);
    if (it != m_registeredControls.end()) {
        m_registeredControls.erase(it);
    }
}

void ThemeSystem::StyleControl(wxWindow* window) {
    if (!window) return;

    window->SetBackgroundColour(GetColor(ColorRole::Background));
    window->SetForegroundColour(GetColor(ColorRole::Foreground));

    if (auto* button = wxDynamicCast(window, wxButton)) {
        button->SetBackgroundColour(GetColor(ColorRole::Button));
        button->SetForegroundColour(GetColor(ColorRole::ButtonText));
    }
    else if (auto* textCtrl = wxDynamicCast(window, wxTextCtrl)) {
        textCtrl->SetBackgroundColour(GetColor(ColorRole::Input));
        textCtrl->SetForegroundColour(GetColor(ColorRole::InputText));
    }
    else if (auto* panel = wxDynamicCast(window, wxPanel)) {
        panel->SetBackgroundColour(GetColor(ColorRole::Background));
    }
    
    for (wxWindow* child : window->GetChildren()) {
        StyleControl(child);
    }

    window->Refresh();
    window->Update();
}

void ThemeSystem::NotifyThemeChanged() {
    for (auto& [owner, callback] : m_themeChangeListeners) {
        if (callback) {
            callback(m_currentTheme);
        }
    }

    for (auto* window : m_registeredControls) {
        if (window) {
            StyleControl(window);
        }
    }
}

ThemeSystem::ThemeColors ThemeSystem::CreateLightTheme() {
    ThemeColors theme;
    theme.set(ColorRole::Background, wxColour(255, 255, 255))
         .set(ColorRole::BackgroundAlternate, wxColour(245, 245, 245))
         .set(ColorRole::Foreground, wxColour(0, 0, 0))
         .set(ColorRole::ForegroundDisabled, wxColour(119, 119, 119))
         .set(ColorRole::Button, wxColour(240, 240, 240))
         .set(ColorRole::ButtonText, wxColour(0, 0, 0))
         .set(ColorRole::ButtonHover, wxColour(229, 229, 229))
         .set(ColorRole::ButtonPressed, wxColour(204, 204, 204))
         .set(ColorRole::ButtonDisabled, wxColour(240, 240, 240))
         .set(ColorRole::Input, wxColour(255, 255, 255))
         .set(ColorRole::InputText, wxColour(0, 0, 0))
         .set(ColorRole::InputBorder, wxColour(204, 204, 204))
         .set(ColorRole::TitleBar, wxColour(0, 120, 212))
         .set(ColorRole::TitleBarText, wxColour(255, 255, 255))
         .set(ColorRole::Border, wxColour(204, 204, 204))
         .set(ColorRole::Primary, wxColour(0, 120, 212))
         .set(ColorRole::Secondary, wxColour(0, 99, 177))
         .set(ColorRole::Success, wxColour(0, 164, 0))
         .set(ColorRole::Warning, wxColour(255, 140, 0))
         .set(ColorRole::Error, wxColour(232, 17, 35))
         .set(ColorRole::Info, wxColour(0, 120, 212))
         .set(ColorRole::FocusOutline, wxColour(0, 120, 212));
    return theme;
}

ThemeSystem::ThemeColors ThemeSystem::CreateDarkTheme() {
    ThemeColors theme;
    theme.set(ColorRole::Background, wxColour(32, 32, 32))
         .set(ColorRole::BackgroundAlternate, wxColour(40, 40, 40))
         .set(ColorRole::Foreground, wxColour(255, 255, 255))
         .set(ColorRole::ForegroundDisabled, wxColour(153, 153, 153))
         .set(ColorRole::Button, wxColour(51, 51, 51))
         .set(ColorRole::ButtonText, wxColour(255, 255, 255))
         .set(ColorRole::ButtonHover, wxColour(68, 68, 68))
         .set(ColorRole::ButtonPressed, wxColour(85, 85, 85))
         .set(ColorRole::ButtonDisabled, wxColour(51, 51, 51))
         .set(ColorRole::Input, wxColour(51, 51, 51))
         .set(ColorRole::InputText, wxColour(255, 255, 255))
         .set(ColorRole::InputBorder, wxColour(85, 85, 85))
         .set(ColorRole::TitleBar, wxColour(32, 32, 32))
         .set(ColorRole::TitleBarText, wxColour(255, 255, 255))
         .set(ColorRole::Border, wxColour(85, 85, 85))
         .set(ColorRole::Primary, wxColour(65, 173, 255))
         .set(ColorRole::Secondary, wxColour(0, 127, 235))
         .set(ColorRole::Success, wxColour(98, 255, 98))
         .set(ColorRole::Warning, wxColour(255, 186, 0))
         .set(ColorRole::Error, wxColour(255, 82, 82))
         .set(ColorRole::Info, wxColour(65, 173, 255))
         .set(ColorRole::FocusOutline, wxColour(65, 173, 255));
    return theme;
}

ThemeSystem::ThemeColors ThemeSystem::CreateHighContrastLightTheme() {
    ThemeColors theme;
    theme.set(ColorRole::Background, wxColour(255, 255, 255))
         .set(ColorRole::Foreground, wxColour(0, 0, 0))
         .set(ColorRole::Button, wxColour(0, 0, 0))
         .set(ColorRole::ButtonText, wxColour(255, 255, 255))
         .set(ColorRole::ButtonHover, wxColour(64, 64, 64))
         .set(ColorRole::ButtonPressed, wxColour(128, 128, 128))
         .set(ColorRole::Input, wxColour(255, 255, 255))
         .set(ColorRole::InputText, wxColour(0, 0, 0))
         .set(ColorRole::InputBorder, wxColour(0, 0, 0))
         .set(ColorRole::Border, wxColour(0, 0, 0))
         .set(ColorRole::Primary, wxColour(0, 0, 255))
         .set(ColorRole::Secondary, wxColour(0, 0, 192))
         .set(ColorRole::Success, wxColour(0, 128, 0))
         .set(ColorRole::Warning, wxColour(255, 128, 0))
         .set(ColorRole::Error, wxColour(255, 0, 0))
         .set(ColorRole::Info, wxColour(0, 0, 255));
    return theme;
}

ThemeSystem::ThemeColors ThemeSystem::CreateHighContrastDarkTheme() {
    ThemeColors theme;
    theme.set(ColorRole::Background, wxColour(0, 0, 0))
         .set(ColorRole::Foreground, wxColour(255, 255, 255))
         .set(ColorRole::Button, wxColour(255, 255, 255))
         .set(ColorRole::ButtonText, wxColour(0, 0, 0))
         .set(ColorRole::ButtonHover, wxColour(192, 192, 192))
         .set(ColorRole::ButtonPressed, wxColour(128, 128, 128))
         .set(ColorRole::Input, wxColour(0, 0, 0))
         .set(ColorRole::InputText, wxColour(255, 255, 255))
         .set(ColorRole::InputBorder, wxColour(255, 255, 255))
         .set(ColorRole::Border, wxColour(255, 255, 255))
         .set(ColorRole::Primary, wxColour(0, 255, 255))
         .set(ColorRole::Secondary, wxColour(0, 192, 192))
         .set(ColorRole::Success, wxColour(0, 255, 0))
         .set(ColorRole::Warning, wxColour(255, 255, 0))
         .set(ColorRole::Error, wxColour(255, 0, 0))
         .set(ColorRole::Info, wxColour(0, 255, 255));
    return theme;
}

void ThemeSystem::StartColorTransition(const ThemeColors& target) {
    const int steps = 10;
    for (int i = 0; i <= steps; i++) {
        float progress = static_cast<float>(i) / steps;
        
        for (const auto& [role, targetColor] : target.colors) {
            auto currentColor = m_colors.get(role);
            m_colors.set(role, InterpolateColors(currentColor, targetColor, progress));
        }
        
        NotifyThemeChanged();
        wxMilliSleep(m_transitionDuration / steps);
    }
}

wxColour ThemeSystem::InterpolateColors(const wxColour& start, const wxColour& end, float progress) {
    return wxColour(
        start.Red() + (end.Red() - start.Red()) * progress,
        start.Green() + (end.Green() - start.Green()) * progress,
        start.Blue() + (end.Blue() - start.Blue()) * progress,
        start.Alpha() + (end.Alpha() - start.Alpha()) * progress
    );
}

void ThemeSystem::SaveThemePreference() {
    wxConfig config;
    config.Write("/Theme/Current", static_cast<int>(m_currentTheme));
    config.Write("/Theme/AnimationsEnabled", m_animationsEnabled);
    config.Write("/Theme/TransitionDuration", m_transitionDuration);
}

void ThemeSystem::LoadThemePreference() {
    wxConfig config;
    int theme;
    if (config.Read("/Theme/Current", &theme)) {
        m_currentTheme = static_cast<ThemeVariant>(theme);
    }
    config.Read("/Theme/AnimationsEnabled", &m_animationsEnabled, true);
    config.Read("/Theme/TransitionDuration", &m_transitionDuration, 200);
}

namespace ThemeUtils {
    wxColour Lighten(const wxColour& color, int amount) {
        return wxColour(
            std::min(color.Red() + amount, 255),
            std::min(color.Green() + amount, 255),
            std::min(color.Blue() + amount, 255),
            color.Alpha()
        );
    }

    wxColour Darken(const wxColour& color, int amount) {
        return wxColour(
            std::max(color.Red() - amount, 0),
            std::max(color.Green() - amount, 0),
            std::max(color.Blue() - amount, 0),
            color.Alpha()
        );
    }

    wxColour WithAlpha(const wxColour& color, unsigned char alpha) {
        return wxColour(color.Red(), color.Green(), color.Blue(), alpha);
    }

    bool HasSufficientContrast(const wxColour& fg, const wxColour& bg) {
        float l1 = GetPerceivedBrightness(fg);
        float l2 = GetPerceivedBrightness(bg);
        float ratio = (std::max(l1, l2) + 0.05f) / (std::min(l1, l2) + 0.05f);
        return ratio >= 4.5f;
    }

    float GetPerceivedBrightness(const wxColour& color) {
        return (color.Red() * 0.299f + color.Green() * 0.587f + color.Blue() * 0.114f) / 255.0f;
    }
}
void ThemeSystem::AddThemeChangeListener(void* owner, ThemeChangeCallback callback) {
    if (owner && callback) {
        m_themeChangeListeners[owner] = callback;
    }
}

void ThemeSystem::RemoveThemeChangeListener(void* owner) {
    if (owner) {
        auto it = m_themeChangeListeners.find(owner);
        if (it != m_themeChangeListeners.end()) {
            m_themeChangeListeners.erase(it);
        }
    }
}