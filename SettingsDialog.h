// SettingsDialog.h
#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <wx/wx.h>
#include <wx/statbox.h>
#include <wx/checkbox.h>
#include "ThemeConfig.h"

// --- ToggleSwitch Declaration ---
class ToggleSwitch : public wxControl
{
public:
    ToggleSwitch(wxWindow *parent, wxWindowID id = wxID_ANY, bool initialState = false,
                 const wxPoint &pos = wxDefaultPosition, const wxSize &size = wxSize(50, 24));
    bool GetValue() const { return m_value; }
    void SetValue(bool value);

private:
    bool m_value;
    wxColour m_trackOnColor; // Colors will be set based on theme
    wxColour m_trackOffColor;
    wxColour m_thumbColor;
    void OnPaint(wxPaintEvent &event);
    void OnMouse(wxMouseEvent &event);
    wxDECLARE_EVENT_TABLE();
};

// --- SettingsDialog Declaration ---
class SettingsDialog : public wxDialog
{
public:
    explicit SettingsDialog(wxWindow *parent);
    wxString GetSelectedTheme() const; // Returns the theme selected *within* the dialog
    bool UseDocker() const;
    bool UseWSL() const;

private:
    // Event Handlers
    void OnOK(wxCommandEvent &event);
    void OnCancel(wxCommandEvent &event);
    void OnThemeButtonClick(wxCommandEvent &event);
    void OnToggleChanged(wxCommandEvent &event);
    void OnPaint(wxPaintEvent &event);

    // UI Creation & Theme Application
    void CreateContent();
    void ApplyTheme(const wxString &theme); // Applies theme to dialog controls

    // UI Elements References
    wxPanel *m_mainPanel;
    wxStaticText *m_titleText;
    wxStaticBoxSizer *m_containerSizer; // Keep if needed for styling box
    wxButton *m_lightButton;
    wxButton *m_darkButton;
    ToggleSwitch *m_dockerToggle;
#ifdef __WXMSW__
    ToggleSwitch *m_wslToggle;
#endif
    wxButton *m_cancelButton;
    wxButton *m_okButton;

    // State within the dialog
    wxString m_selectedThemeInDialog; // Tracks selection before OK is pressed

    // Cached theme colors (optional, could get directly from ThemeConfig)
    wxColour m_bgColor;
    wxColour m_textColor;
    wxColour m_labelColor;
    wxColour m_primaryColor;
    wxColour m_secondaryColor;
    wxColour m_inputBgColor;
    wxColour m_inputFgColor;

    wxDECLARE_EVENT_TABLE();
};

wxDECLARE_EVENT(EVT_TOGGLE_SWITCH, wxCommandEvent);

#endif // SETTINGSDIALOG_H