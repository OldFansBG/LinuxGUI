#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <wx/wx.h>
#include "ThemeConfig.h"

class SettingsDialog : public wxDialog
{
public:
    explicit SettingsDialog(wxWindow *parent);
    wxString GetSelectedTheme() const { return m_themeChoice->GetStringSelection().Lower(); }
    bool UseDocker() const { return m_dockerCheckbox->GetValue(); }
    bool UseWSL() const { return m_wslCheckbox ? m_wslCheckbox->GetValue() : false; }

private:
    void OnOK(wxCommandEvent &event);
    void OnCancel(wxCommandEvent &event);
    void OnThemeSelect(wxCommandEvent &event);
    void OnContainerTypeChanged(wxCommandEvent &event);
    void CreateControls();
    void PreviewTheme(const wxString &themeName);

    wxPanel *m_mainPanel;
    wxChoice *m_themeChoice;
    wxPanel *m_previewPanel;
    wxCheckBox *m_dockerCheckbox;
    wxCheckBox *m_wslCheckbox;

    DECLARE_EVENT_TABLE()
};

#endif // SETTINGSDIALOG_H
