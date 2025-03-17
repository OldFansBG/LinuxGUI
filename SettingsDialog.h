#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H
#include <wx/wx.h>
#include "ThemeConfig.h"
#include <wxSVG/svg.h> // Include wxSVG headers
class ToggleSwitch : public wxControl
{
public:
    ToggleSwitch(wxWindow *parent, wxWindowID id = wxID_ANY, bool initialState = false,
                 const wxPoint &pos = wxDefaultPosition, const wxSize &size = wxSize(50, 24));
    bool GetValue() const { return m_value; }
    void SetValue(bool value);

private:
    bool m_value;
    wxColour m_trackOnColor = wxColour(79, 70, 229);
    wxColour m_trackOffColor = wxColour(75, 85, 99);
    wxColour m_thumbColor = wxColour(255, 255, 255);
    void OnPaint(wxPaintEvent &event);
    void OnMouse(wxMouseEvent &event);
    wxDECLARE_EVENT_TABLE();
};
class SettingsDialog : public wxDialog
{
public:
    explicit SettingsDialog(wxWindow *parent);
    wxString GetSelectedTheme() const;
    bool UseDocker() const;
    bool UseWSL() const;

private:
    void OnOK(wxCommandEvent &event);
    void OnCancel(wxCommandEvent &event);
    void OnThemeButtonClick(wxCommandEvent &event);
    void OnToggleChanged(wxCommandEvent &event);
    void OnPaint(wxPaintEvent &event);
    void CreateContent();
    void ApplyTheme(const wxString &theme);
    void SetupColors();
    wxPanel *m_mainPanel;
    wxButton *m_lightButton;
    wxButton *m_darkButton;
    ToggleSwitch *m_dockerToggle;
#ifdef __WXMSW__
    ToggleSwitch *m_wslToggle;
#endif
    wxButton *m_cancelButton;
    wxButton *m_okButton;
    wxColour m_normalItemColor;
    wxColour m_hoverItemColor;
    wxColour m_bgColor = wxColour(30, 41, 59);
    wxColour m_textColor = wxColour(229, 231, 235);
    wxColour m_primaryColor = wxColour(79, 70, 229);
    wxColour m_secondaryColor = wxColour(75, 85, 99);
    DECLARE_EVENT_TABLE()
};
wxDECLARE_EVENT(EVT_TOGGLE_SWITCH, wxCommandEvent);
#endif