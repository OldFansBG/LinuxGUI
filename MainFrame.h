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

private:
   void CreateFrameControls();
   void OpenSecondWindow();
   wxString m_currentISOPath;

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
   
   // UI Creation functions
   wxPanel* CreateLogoPanel(wxWindow* parent);
   wxPanel* CreateDetectionPanel(wxWindow* parent);
   wxPanel* CreateProjectPanel(wxWindow* parent);
   wxPanel* CreateProgressPanel(wxWindow* parent);

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