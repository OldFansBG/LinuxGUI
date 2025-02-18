#ifndef SECONDWINDOW_H
#define SECONDWINDOW_H

#include <wx/wx.h>
#include <wx/event.h>

enum {
    ID_TERMINAL_TAB = 1001,
    ID_SQL_TAB,
    ID_NEXT_BUTTON,
    ID_LOG_TEXTCTRL
};

#include "SQLTab.h"
#include "LinuxTerminalPanel.h"
#include "OSDetector.h"
#include "ContainerManager.h"

// Forward declaration for OverlayFrame
class OverlayFrame;

class SecondWindow : public wxFrame {
public:
    SecondWindow(wxWindow* parent, 
                const wxString& title, 
                const wxString& isoPath,
                const wxString& projectDir);
    virtual ~SecondWindow();

    void CloseOverlay();  // Method to close the overlay

private:
    // UI Elements
    wxPanel* m_mainPanel;
    wxPanel* m_terminalTab;
    wxPanel* m_sqlTab;
    wxTextCtrl* m_logTextCtrl;
    LinuxTerminalPanel* m_terminalPanel;

    // Data Members
    wxString m_isoPath;
    wxString m_projectDir;
    wxString m_containerId;
    bool m_threadRunning;

    // Overlay
    OverlayFrame* m_overlay;

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