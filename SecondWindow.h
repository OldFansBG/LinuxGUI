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

class SecondWindow : public wxFrame {
public:
    SecondWindow(wxWindow* parent, const wxString& title, const wxString& isoPath);
    ~SecondWindow();

private:
    void CreateControls();
    void OnClose(wxCloseEvent& event);
    void OnNext(wxCommandEvent& event);
    void OnTabChanged(wxCommandEvent& event);
    void OnPythonTaskCompleted(wxCommandEvent& event);
    void OnPythonLogUpdate(wxCommandEvent& event);

    wxString m_isoPath;
    wxString m_containerId;
    wxPanel* m_mainPanel;
    wxPanel* m_terminalTab;
    SQLTab* m_sqlTab;
    LinuxTerminalPanel* m_terminalPanel;
    wxTextCtrl* m_logTextCtrl;
    OSDetector m_osDetector;

    PythonWorkerThread* m_workerThread; // Track the worker thread
    bool m_threadRunning; // Flag to track thread status

    enum {
        ID_TERMINAL_TAB = wxID_HIGHEST + 1,
        ID_SQL_TAB,
        ID_NEXT_BUTTON,
        ID_LOG_TEXTCTRL
    };

    DECLARE_EVENT_TABLE()
};

#endif // SECONDWINDOW_H