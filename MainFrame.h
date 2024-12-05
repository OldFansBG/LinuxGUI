#ifndef MAINFRAME_H
#define MAINFRAME_H

#include <wx/wx.h>
#include <wx/statline.h>
#include <wx/bmpbuttn.h>
#include <yaml-cpp/yaml.h>
#include <vector>
#include <string>
#include <wx/settings.h>
#include <wx/filename.h>
#include "MyButton.h"
#include "CustomTitleBar.h"
#include "CustomStatusBar.h"
#include "ThemeColors.h"
#include "ISOExtractor.h"
#include "ThemeColors.h"
#include "ISOExtractor.h"
#include "SecondWindow.h"
#ifdef __WXMSW__
   #include <dwmapi.h>
   #include <windows.h>
   #pragma comment(lib, "dwmapi.lib")
   #pragma comment(lib, "advapi32.lib")
   
   #ifndef DWMWA_MICA_EFFECT
       #define DWMWA_MICA_EFFECT 1029
   #endif
#endif

class MainFrame : public wxFrame {
public:
    MainFrame(const wxString& title);
    void SetStatusText(const wxString& text);
    void OnThemeChanged();
    void ApplyTheme(bool isDarkMode);

private:
    void ApplyThemeToWindow(wxWindow* window, bool isDarkMode);
    void OpenSecondWindow();
    wxString m_currentISOPath;  // Store the ISO path for SecondWindow
    // Event handlers
    void OnBrowseISO(wxCommandEvent& event);
    void OnBrowseWorkDir(wxCommandEvent& event);
    void OnDetect(wxCommandEvent& event);
    void OnExtract(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnSettings(wxCommandEvent& event);
    void UpdateExtractionProgress(int progress, const wxString& status);

    // ISO Analysis functions
    bool SearchReleaseFile(const wxString& isoPath, wxString& releaseContent);
    bool SearchGrubEnv(const wxString& isoPath, wxString& distributionName);
    wxString ExtractNameFromGrubEnv(const wxString& content);
    wxString DetectDistribution(const wxString& releaseContent);
    
    // Configuration and UI setup
    bool LoadConfig();
    void CreateSettingsMenu();
    void CreateFrameControls();
    
    // UI Creation functions
    wxPanel* CreateLogoPanel(wxWindow* parent);
    wxPanel* CreateDetectionPanel(wxWindow* parent);
    wxPanel* CreateProjectPanel(wxWindow* parent);
    wxPanel* CreateProgressPanel(wxWindow* parent);

    // Styling functions
    void StylePanel(wxPanel* panel) {
        bool isDarkMode = GetBackgroundColour().GetLuminance() < 0.5;
        if (isDarkMode) {
            panel->SetBackgroundColour(ThemeColors::DARK_BACKGROUND);
            panel->SetForegroundColour(ThemeColors::DARK_TEXT);
        } else {
            panel->SetBackgroundColour(ThemeColors::LIGHT_BACKGROUND);
            panel->SetForegroundColour(ThemeColors::LIGHT_TEXT);
        }
    }

    void StyleMyButton(MyButton* button) {
        bool isDarkMode = GetBackgroundColour().GetLuminance() < 0.5;
        if (isDarkMode) {
            button->SetBackgroundColour(ThemeColors::DARK_BUTTON_BG);
            button->SetForegroundColour(ThemeColors::DARK_BUTTON_TEXT);
            button->SetHoverColor(ThemeColors::DARK_BUTTON_HOVER);
        } else {
            button->SetBackgroundColour(ThemeColors::LIGHT_BUTTON_BG);
            button->SetForegroundColour(ThemeColors::LIGHT_BUTTON_TEXT);
            button->SetHoverColor(ThemeColors::LIGHT_BUTTON_HOVER);
        }
    }

    void StyleButton(wxButton* button, bool isPrimary = false) {
        bool isDarkMode = GetBackgroundColour().GetLuminance() < 0.5;
        if (isPrimary) {
            button->SetBackgroundColour(isDarkMode ? ThemeColors::DARK_PRIMARY_BG : ThemeColors::LIGHT_PRIMARY_BG);
            button->SetForegroundColour(isDarkMode ? ThemeColors::DARK_PRIMARY_TEXT : ThemeColors::LIGHT_PRIMARY_TEXT);
        } else {
            button->SetBackgroundColour(isDarkMode ? ThemeColors::DARK_BUTTON_BG : ThemeColors::LIGHT_BUTTON_BG);
            button->SetForegroundColour(isDarkMode ? ThemeColors::DARK_BUTTON_TEXT : ThemeColors::LIGHT_BUTTON_TEXT);
        }
    }

    void StyleTextCtrl(wxTextCtrl* ctrl) {
        bool isDarkMode = GetBackgroundColour().GetLuminance() < 0.5;
        if (isDarkMode) {
            ctrl->SetBackgroundColour(ThemeColors::DARK_BUTTON_BG);
            ctrl->SetForegroundColour(ThemeColors::DARK_TEXT);
        } else {
            ctrl->SetBackgroundColour(ThemeColors::LIGHT_BACKGROUND);
            ctrl->SetForegroundColour(ThemeColors::LIGHT_TEXT);
        }
    }

    void StyleText(wxStaticText* text) {
        bool isDarkMode = GetBackgroundColour().GetLuminance() < 0.5;
        text->SetForegroundColour(isDarkMode ? ThemeColors::DARK_TEXT : ThemeColors::LIGHT_TEXT);
    }

    void StyleGauge(wxGauge* gauge) {
        bool isDarkMode = GetBackgroundColour().GetLuminance() < 0.5;
        if (isDarkMode) {
            gauge->SetBackgroundColour(ThemeColors::DARK_BUTTON_BG);
            gauge->SetForegroundColour(ThemeColors::DARK_HIGHLIGHT);
        } else {
            gauge->SetBackgroundColour(ThemeColors::LIGHT_BUTTON_BG);
            gauge->SetForegroundColour(ThemeColors::LIGHT_PRIMARY_BG);
        }
    }

    // Member variables
    wxTextCtrl* m_isoPathCtrl;
    wxTextCtrl* m_distroCtrl;
    wxTextCtrl* m_projectNameCtrl;
    wxTextCtrl* m_versionCtrl;
    wxTextCtrl* m_workDirCtrl;
    wxGauge* m_progressGauge;
    wxStaticText* m_statusText;
    MyButton* m_settingsButton;
    CustomTitleBar* m_titleBar;
    CustomStatusBar* m_statusBar;
    YAML::Node m_config;
    ISOExtractor* m_currentExtractor;

    enum {
        ID_BROWSE_ISO = 1,
        ID_BROWSE_WORKDIR,
        ID_DETECT,
        ID_EXTRACT,
        ID_CANCEL,
        ID_SETTINGS
    };

    DECLARE_EVENT_TABLE()
};

#endif // MAINFRAME_H