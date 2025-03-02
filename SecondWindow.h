#ifndef SECONDWINDOW_H
#define SECONDWINDOW_H

#include <wx/wx.h>
#include "DesktopTab.h"
#include "SQLTab.h"
#include "LinuxTerminalPanel.h"
#include "OSDetector.h"
#include "ContainerManager.h"
#include "FlatpakStore.h"

#ifdef _WIN32
#include "WinTerminalManager.h"
#endif

class OverlayFrame;

// Define control IDs
enum {
    ID_TERMINAL_TAB = 1001, // Start from wxID_HIGHEST + 1 to avoid conflicts
    ID_SQL_TAB,
    ID_NEXT_BUTTON
};

class SecondWindow : public wxFrame {
public:
    SecondWindow(wxWindow* parent,
                const wxString& title,
                const wxString& isoPath,
                const wxString& projectDir,
                DesktopTab* desktopTab); // Add DesktopTab parameter

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
    FlatpakStore* m_flatpakStore; // Added FlatpakStore member

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