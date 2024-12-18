#ifndef SECONDWINDOW_H
#define SECONDWINDOW_H

#include <wx/wx.h>
#include "OSDetector.h"
#include "WindowsCmdPanel.h"
#include "LinuxTerminalPanel.h" 
#include <wx/notebook.h>

class SecondWindow : public wxFrame {
public:
   SecondWindow(wxWindow* parent, const wxString& title, const wxString& isoPath);

private:
   ~SecondWindow();
   void CreateControls();
   void OnClose(wxCloseEvent& event);
   void OnTabChanged(wxCommandEvent& event);
   void OnNext(wxCommandEvent& event);
   void OnSQLTabChanged(wxCommandEvent& event);
   
   void ShowSQLTab(int tabId);
   void CreateSQLPanel();
   void CreateDesktopTab();
   void CreateAppsTab(); 
   void CreateSystemTab();
   void CreateCustomizeTab();
   void CreateHardwareTab();

   OSDetector m_osDetector;
   WindowsCmdPanel* m_cmdPanel;
   LinuxTerminalPanel* m_terminalPanel;
   wxString m_isoPath;
   wxPanel* m_mainPanel;
   wxPanel* m_terminalTab;
   wxPanel* m_sqlTab;
   wxSizer* m_terminalSizer;
   wxSizer* m_sqlSizer;
   wxPanel* m_sqlContent;
   int m_currentSqlTab;
   std::vector<wxButton*> m_sqlTabButtons;
   void UnbindSQLEvents();

   enum {
       ID_TERMINAL_TAB = wxID_HIGHEST + 1,
       ID_SQL_TAB,
       ID_NEXT_BUTTON,
       ID_SQL_DESKTOP,
       ID_SQL_APPS,
       ID_SQL_SYSTEM,
       ID_SQL_CUSTOMIZE,
       ID_SQL_HARDWARE
   };

   DECLARE_EVENT_TABLE()
};

#endif // SECONDWINDOW_H