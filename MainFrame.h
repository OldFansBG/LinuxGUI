#ifndef MAINFRAME_H
#define MAINFRAME_H

#include <wx/wx.h>
#include <wx/statline.h>
#include <wx/bmpbuttn.h>
#include <yaml-cpp/yaml.h>
#include <vector>
#include <string>
#include "MyButton.h"
#include "CustomTitleBar.h"
#include "CustomStatusBar.h"
#include "SecondWindow.h"
#include "WindowIDs.h"
#include "SettingsManager.h"
#include "DesktopTab.h"

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
   ~MainFrame();

   void SetStatusText(const wxString& text);

private:
   bool SearchSquashFS(const wxString& isoPath, wxArrayString& foundFiles);
   void CreateFrameControls();
   void OpenSecondWindow();
   wxString m_currentISOPath;

   void OnBrowseISO(wxCommandEvent& event);
   void OnBrowseWorkDir(wxCommandEvent& event);
   void OnDetect(wxCommandEvent& event);
   void OnNextButton(wxCommandEvent& event);  // Changed from OnExtract
   void OnCancel(wxCommandEvent& event);
   void OnSettings(wxCommandEvent& event);
   void UpdateExtractionProgress(int progress, const wxString& status);

   bool SearchReleaseFile(const wxString& isoPath, wxString& releaseContent);
   bool SearchGrubEnv(const wxString& isoPath, wxString& distributionName);
   wxString ExtractNameFromGrubEnv(const wxString& content);
   wxString DetectDistribution(const wxString& releaseContent);
   
   bool LoadConfig();
   void CreateSettingsMenu();
   
   wxPanel* CreateLogoPanel(wxWindow* parent);
   wxPanel* CreateDetectionPanel(wxWindow* parent);
   wxPanel* CreateProjectPanel(wxWindow* parent);
   wxPanel* CreateProgressPanel(wxWindow* parent);

   // Control members
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

   // Settings manager
   SettingsManager m_settingsManager;

   // Add DesktopTab member
   DesktopTab* m_desktopTab;

   void OnGUIDetected(wxCommandEvent& event); // Add this handler

   // Event table declaration
   DECLARE_EVENT_TABLE()
};

#endif // MAINFRAME_H