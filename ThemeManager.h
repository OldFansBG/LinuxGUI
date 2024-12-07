#ifndef THEME_MANAGER_H
#define THEME_MANAGER_H

#include <wx/wx.h>
#include "ThemeColors.h"
#include <vector>

class ThemeManager {
public:
    static ThemeManager& Get();

    enum class Theme {
        Light,
        Dark
    };

    struct Colors {
        wxColour background;
        wxColour foreground;
        wxColour border;
        wxColour buttonBg;
        wxColour buttonText;
        wxColour buttonHover;
        wxColour primaryBg;
        wxColour primaryText;
        wxColour titlebarBg;
        wxColour highlight;
        wxColour closeButtonHover;
    };

    void SetTheme(Theme theme);
    const Colors& GetColors() const { return m_colors; }
    bool IsDarkTheme() const { return m_currentTheme == Theme::Dark; }
    void StyleControl(wxWindow* window);
    void AddObserver(wxWindow* window);
    void RemoveObserver(wxWindow* window);

private:
    ThemeManager();
    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;

    void UpdateColors();
    void NotifyThemeChanged();
    void StyleSpecificControl(wxWindow* window);

    Theme m_currentTheme;
    Colors m_colors;
    std::vector<wxWindow*> m_observers;
};

#endif // THEME_MANAGER_H