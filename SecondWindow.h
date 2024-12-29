#ifndef SECONDWINDOW_H
#define SECONDWINDOW_H

#include <wx/wx.h>
#include <wx/statline.h>
#include <wx/notebook.h>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/wfstream.h>
#include <wx/progdlg.h>
#include <wx/stdpaths.h>
#include <vector>
#include "OSDetector.h"
#include "WindowsCmdPanel.h"
#include "LinuxTerminalPanel.h"
#include "ContainerManager.h"  // Add this include

class SecondWindow : public wxFrame {
public:
    SecondWindow(wxWindow* parent, const wxString& title, const wxString& isoPath);
    ~SecondWindow();

private:
    void CreateControls();
    void OnClose(wxCloseEvent& event);
    void OnNext(wxCommandEvent& event);
    bool RetryFailedOperation(const wxString& operation, int maxAttempts = 3);
    bool ExecuteOperation(const wxString& operation);
    bool RunScript(const wxString& containerId, const wxString& script);
    bool CopyISOFromContainer(const wxString& containerId, const wxString& destPath);
    void ShowCompletionDialog(const wxString& isoPath);
    wxString GetFileSizeString(const wxString& filePath);
    void LogToFile(const wxString& logPath, const wxString& message);
    
    void OnTabChanged(wxCommandEvent& event);
    void OnSQLTabChanged(wxCommandEvent& event);
    void ShowSQLTab(int tabId);
    void CreateSQLPanel();
    void CreateDesktopTab();
    void CreateAppsTab(); 
    void CreateSystemTab();
    void CreateCustomizeTab();
    void CreateHardwareTab();
    void UnbindSQLEvents();

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
    wxStaticLine* separator;
    int m_currentSqlTab;
    std::vector<wxButton*> m_sqlTabButtons;
    wxString m_containerId;

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