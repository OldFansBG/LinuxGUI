#ifndef SECONDWINDOW_H
#define SECONDWINDOW_H

#include <wx/wx.h>
#include <wx/statline.h>
#include <wx/thread.h>
#include "OSDetector.h"
#include "LinuxTerminalPanel.h"
#include "ContainerManager.h"
#include "SQLTab.h"
#include "PythonWorkerThread.h"
#include "CustomEvents.h"
#include <Python.h>

class SecondWindow : public wxFrame {
public:
    SecondWindow(wxWindow* parent, const wxString& title, const wxString& isoPath);
    virtual ~SecondWindow();

private:
    // UI Creation
    void CreateControls();
    
    // Thread Management
    void StartPythonThread();
    void CleanupThread();
    
    // Event Handlers
    void OnClose(wxCloseEvent& event);
    void OnNext(wxCommandEvent& event);
    void OnTabChanged(wxCommandEvent& event);
    void OnPythonTaskCompleted(wxCommandEvent& event);
    void OnPythonLogUpdate(wxCommandEvent& event);

    // Member Variables
    wxString m_isoPath;
    wxString m_containerId;
    
    // UI Components
    wxPanel* m_mainPanel;
    wxPanel* m_terminalTab;
    SQLTab* m_sqlTab;
    LinuxTerminalPanel* m_terminalPanel;
    wxTextCtrl* m_logTextCtrl;
    OSDetector m_osDetector;

    // Thread Management
    PythonWorkerThread* m_workerThread;
    bool m_threadRunning;

    // Control IDs
    enum {
        ID_TERMINAL_TAB = wxID_HIGHEST + 1,
        ID_SQL_TAB,
        ID_NEXT_BUTTON,
        ID_LOG_TEXTCTRL
    };

    DECLARE_EVENT_TABLE()
};

#endif // SECONDWINDOW_H