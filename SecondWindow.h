// SecondWindow.h
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

#ifdef _WIN32
#include <wx/timer.h>
#endif

class OverlayFrame;

class SecondWindow : public wxFrame {
public:
    SecondWindow(wxWindow* parent, 
                const wxString& title, 
                const wxString& isoPath,
                const wxString& projectDir);
    virtual ~SecondWindow();

    void CloseOverlay();

private:
    wxPanel* m_mainPanel;
    wxPanel* m_terminalTab;
    SQLTab* m_sqlTab;
    wxTextCtrl* m_logTextCtrl;
    LinuxTerminalPanel* m_terminalPanel;

    wxString m_isoPath;
    wxString m_projectDir;
    wxString m_containerId;
    bool m_threadRunning;

    OverlayFrame* m_overlay;
    OSDetector m_osDetector;

#ifdef _WIN32
    wxTimer* m_winTerminalTimer;
    int m_winTerminalStep;
    void OnWinTerminalTimer(wxTimerEvent& event); // Declare the timer handler
#endif

    void CreateControls();
    void StartPythonExecutable();
    void CleanupThread();

    void OnClose(wxCloseEvent& event);
    void OnNext(wxCommandEvent& event);
    void OnTabChanged(wxCommandEvent& event);

    wxDECLARE_EVENT_TABLE();
};

#endif // SECONDWINDOW_H