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

enum {
    ID_TERMINAL_TAB = 1001,
    ID_SQL_TAB,
    ID_NEXT_BUTTON
};

class SecondWindow : public wxFrame {
public:
    SecondWindow(wxWindow* parent,
                const wxString& title,
                const wxString& isoPath,
                const wxString& projectDir,
                DesktopTab* desktopTab);

    virtual ~SecondWindow();

    void CloseOverlay();
    wxString GetProjectDir() const { return m_projectDir; }
    void ExecuteDockerCommand(const wxString& containerId);
    DesktopTab* GetDesktopTab() const { return m_desktopTab; }
    FlatpakStore* GetFlatpakStore() const { return m_flatpakStore; }
    SQLTab* GetSQLTab() const { return m_sqlTab; } // Added getter for m_sqlTab

private:
    wxPanel* m_mainPanel;
    wxPanel* m_terminalTab;
    SQLTab* m_sqlTab;
    LinuxTerminalPanel* m_terminalPanel;
    FlatpakStore* m_flatpakStore;

    wxString m_isoPath;
    wxString m_projectDir;
    wxString m_containerId;
    bool m_threadRunning;

    OverlayFrame* m_overlay;
    OSDetector m_osDetector;

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