#include "ThemeManager.h"
#include <algorithm>

ThemeManager& ThemeManager::Get() {
    static ThemeManager instance;
    return instance;
}

ThemeManager::ThemeManager() : m_currentTheme(Theme::Light) {
    UpdateColors();
}

void ThemeManager::SetTheme(Theme theme) {
    m_currentTheme = theme;
    UpdateColors();
    NotifyThemeChanged();
}

void ThemeManager::StyleControl(wxWindow* window) {
    if (!window) return;

    window->SetBackgroundColour(m_colors.background);
    window->SetForegroundColour(m_colors.foreground);

    StyleSpecificControl(window);

    for (wxWindow* child : window->GetChildren()) {
        StyleControl(child);
    }
}

void ThemeManager::AddObserver(wxWindow* window) {
    if (window && std::find(m_observers.begin(), m_observers.end(), window) == m_observers.end()) {
        m_observers.push_back(window);
        StyleControl(window);
    }
}

void ThemeManager::RemoveObserver(wxWindow* window) {
    m_observers.erase(
        std::remove(m_observers.begin(), m_observers.end(), window),
        m_observers.end()
    );
}

void ThemeManager::UpdateColors() {
    if (m_currentTheme == Theme::Dark) {
        m_colors = {
            ThemeColors::DARK_BACKGROUND,
            ThemeColors::DARK_TEXT,
            ThemeColors::DARK_BORDER,
            ThemeColors::DARK_BUTTON_BG,
            ThemeColors::DARK_BUTTON_TEXT,
            ThemeColors::DARK_BUTTON_HOVER,
            ThemeColors::DARK_PRIMARY_BG,
            ThemeColors::DARK_PRIMARY_TEXT,
            ThemeColors::DARK_TITLEBAR,
            ThemeColors::DARK_HIGHLIGHT,
            ThemeColors::CLOSE_BUTTON_HOVER
        };
    } else {
        m_colors = {
            ThemeColors::LIGHT_BACKGROUND,
            ThemeColors::LIGHT_TEXT,
            ThemeColors::LIGHT_BORDER,
            ThemeColors::LIGHT_BUTTON_BG,
            ThemeColors::LIGHT_BUTTON_TEXT,
            ThemeColors::LIGHT_BUTTON_HOVER,
            ThemeColors::LIGHT_PRIMARY_BG,
            ThemeColors::LIGHT_PRIMARY_TEXT,
            ThemeColors::LIGHT_TITLEBAR,
            ThemeColors::LIGHT_PRIMARY_BG,
            ThemeColors::CLOSE_BUTTON_HOVER
        };
    }
}

void ThemeManager::NotifyThemeChanged() {
    for (wxWindow* window : m_observers) {
        if (window) {
            StyleControl(window);
            window->Refresh();
            window->Update();
        }
    }
}

void ThemeManager::StyleSpecificControl(wxWindow* window) {
    if (auto* panel = wxDynamicCast(window, wxPanel)) {
        panel->SetBackgroundColour(m_colors.background);
    }
    else if (auto* button = wxDynamicCast(window, wxButton)) {
        button->SetBackgroundColour(m_colors.buttonBg);
        button->SetForegroundColour(m_colors.buttonText);
    }
    else if (auto* textCtrl = wxDynamicCast(window, wxTextCtrl)) {
        textCtrl->SetBackgroundColour(m_colors.buttonBg);
        textCtrl->SetForegroundColour(m_colors.foreground);
    }
    else if (auto* staticText = wxDynamicCast(window, wxStaticText)) {
        staticText->SetForegroundColour(m_colors.foreground);
    }
    else if (auto* gauge = wxDynamicCast(window, wxGauge)) {
        gauge->SetBackgroundColour(m_colors.buttonBg);
        gauge->SetForegroundColour(m_colors.highlight);
    }
    else if (auto* choice = wxDynamicCast(window, wxChoice)) {
        choice->SetBackgroundColour(m_colors.buttonBg);
        choice->SetForegroundColour(m_colors.foreground);
    }
}