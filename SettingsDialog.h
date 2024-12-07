// SettingsDialog.h
#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <wx/wx.h>
#include "ThemeManager.h"

class SettingsDialog : public wxDialog 
{
public:
    explicit SettingsDialog(wxWindow* parent);

private:
    void OnOK(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void CreateControls();
    
    wxPanel* m_mainPanel;
    wxChoice* m_themeChoice;

    DECLARE_EVENT_TABLE()
};
#endif // SETTINGSDIALOG_H