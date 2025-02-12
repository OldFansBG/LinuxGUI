#ifndef SECONDWINDOW_H
#define SECONDWINDOW_H

#include <wx/wx.h>
#include <wx/event.h>

//---------------------------------------------------------------------
// Define control IDs if not defined elsewhere (e.g., in a resource header)
enum {
    ID_TERMINAL_TAB = 1001,
    ID_SQL_TAB,
    ID_NEXT_BUTTON,
    ID_LOG_TEXTCTRL
};

//---------------------------------------------------------------------
// Include external dependencies if available.
// Otherwise, comment these includes and use the forward declarations below.
#include "SQLTab.h"              // Your SQLTab class header
#include "LinuxTerminalPanel.h"  // Your LinuxTerminalPanel class header
#include "OSDetector.h"          // Your OSDetector class header
#include "ContainerManager.h"    // Your ContainerManager class header

// If you do not have these header files, you can alternatively
// forward-declare them as shown below:
// class SQLTab;
// class LinuxTerminalPanel;
// class OSDetector;
// class ContainerManager;

//---------------------------------------------------------------------

class SecondWindow : public wxFrame {
public:
    SecondWindow(wxWindow* parent, const wxString& title, const wxString& isoPath);
    virtual ~SecondWindow();

private:
    // UI Elements
    wxPanel* m_mainPanel;
    wxPanel* m_terminalTab;
    wxPanel* m_sqlTab;
    wxTextCtrl* m_logTextCtrl;
    LinuxTerminalPanel* m_terminalPanel;

    // Data Members
    wxString m_isoPath;
    wxString m_containerId;
    bool m_threadRunning;

    // An instance of OSDetector (if used in your project)
    OSDetector m_osDetector;

    // Methods
    void CreateControls();
    void StartPythonExecutable();
    void CleanupThread();

    // Event Handlers
    void OnClose(wxCloseEvent& event);
    void OnNext(wxCommandEvent& event);
    void OnTabChanged(wxCommandEvent& event);

    wxDECLARE_EVENT_TABLE();
};

#endif // SECONDWINDOW_H
