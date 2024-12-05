// SettingsDialog.h
#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <wx/wx.h>
#include "ThemeColors.h"

class SettingsDialog : public wxDialog 
{
public:
    explicit SettingsDialog(wxWindow* parent);

private:
    void OnOK(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void ApplyTheme(bool isDark);
    void ApplyThemeToWindow(wxWindow* window, bool isDark);
    
    wxPanel* m_mainPanel;
    wxChoice* m_themeChoice;
    bool m_isDark;

    DECLARE_EVENT_TABLE()
};

#endif // SETTINGSDIALOG_H