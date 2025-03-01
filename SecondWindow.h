// SecondWindow.h
#ifndef SECONDWINDOW_H
#define SECONDWINDOW_H

#include <wx/wx.h>
#include "DesktopTab.h" // Include DesktopTab header

enum {
    ID_TERMINAL_TAB = 1001,
    ID_SQL_TAB,
    ID_NEXT_BUTTON
};

#include "SQLTab.h"
#include "LinuxTerminalPanel.h"
#include "OSDetector.h"
#include "ContainerManager.h"

#ifdef _WIN32
#include "WinTerminalManager.h"
#endif

class OverlayFrame;

class SecondWindow : public wxFrame {
public:
    // Updated constructor with DesktopTab parameter
    SecondWindow(wxWindow* parent,
                const wxString& title,
                const wxString& isoPath,
                const wxString& projectDir,
                DesktopTab* desktopTab); // Add this parameter

    virtual ~SecondWindow();

    void CloseOverlay();

    // Public getter for m_projectDir
    wxString GetProjectDir() const { return m_projectDir; }

    // Public method to execute Docker commands
    void ExecuteDockerCommand(const wxString& containerId);

    // Getter for DesktopTab
    DesktopTab* GetDesktopTab() const { return m_desktopTab; }

private:
    wxPanel* m_mainPanel;
    wxPanel* m_terminalTab;
    SQLTab* m_sqlTab;
    LinuxTerminalPanel* m_terminalPanel;

    wxString m_isoPath;
    wxString m_projectDir; // Private member
    wxString m_containerId;
    bool m_threadRunning;

    OverlayFrame* m_overlay;
    OSDetector m_osDetector;

    // Add DesktopTab member variable
    DesktopTab* m_desktopTab;

#ifdef _WIN32
    WinTerminalManager* m_winTerminalManager;
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