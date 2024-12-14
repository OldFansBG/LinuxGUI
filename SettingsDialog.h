#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <wx/wx.h>

class SettingsDialog : public wxDialog {
public:
    explicit SettingsDialog(wxWindow* parent);

private:
    void OnOK(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnThemeSelect(wxCommandEvent& event);
    void OnAnimationToggle(wxCommandEvent& event);
    void CreateControls();
    
    wxPanel* m_mainPanel;
    wxChoice* m_themeChoice;
    wxCheckBox* m_animationsCheckbox;
    wxSlider* m_transitionSlider;
    wxPanel* m_previewPanel;
    
    DECLARE_EVENT_TABLE()
};

#endif // SETTINGSDIALOG_H