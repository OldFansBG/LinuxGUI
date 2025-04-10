#ifndef SECONDWINDOW_H
#define SECONDWINDOW_H

#include <wx/wx.h>
#include <wx/thread.h>
// #include <wx/listctrl.h> // No longer needed directly here after separating MongoDBPanel
#include "DesktopTab.h"
#include "ConfigTab.h"
#include "LinuxTerminalPanel.h"
#include "OSDetector.h"
#include "ContainerManager.h"
#include "FlatpakStore.h"
#include "WindowIDs.h"         // <<< Make sure this is included for the IDs
#include <mongocxx/client.hpp> // Keep MongoDB includes needed by SecondWindow itself
#include <mongocxx/instance.hpp>

#ifdef _WIN32
#include "WinTerminalManager.h"
#endif

// Forward declarations
class OverlayFrame;
class MongoDBPanel; // <<< Forward Declaration

// --- SecondWindow Definition ---
class SecondWindow : public wxFrame
{
public:
    SecondWindow(wxWindow *parent,
                 const wxString &title,
                 const wxString &isoPath,
                 const wxString &projectDir,
                 DesktopTab *desktopTab);

    virtual ~SecondWindow();

    void CloseOverlay();
    wxString GetProjectDir() const { return m_projectDir; }
    void ExecuteDockerCommand(const wxString &containerId);
    DesktopTab *GetDesktopTab() const { return m_desktopTab; }
    FlatpakStore *GetFlatpakStore() const { return m_flatpakStore; }
    SQLTab *GetSQLTab() const { return m_sqlTab; }

private:
    wxPanel *m_mainPanel;
    wxPanel *m_terminalTab;
    SQLTab *m_sqlTab;
    LinuxTerminalPanel *m_terminalPanel;
    FlatpakStore *m_flatpakStore;
    MongoDBPanel *m_mongoPanel; // <<< Pointer to the panel

    wxString m_isoPath;
    wxString m_projectDir;
    wxString m_containerId;
    bool m_threadRunning;

    OverlayFrame *m_overlay;
    OSDetector m_osDetector;

    DesktopTab *m_desktopTab;

    static mongocxx::instance m_mongoInstance;

    wxThread *m_cleanupThread;
    void CleanupContainerAsync();

    bool m_isClosing;
    wxTimer *m_closeTimer;

    wxWindow *m_lastTab;

#ifdef _WIN32
    WinTerminalManager *m_winTerminalManager;
#endif

    void CreateControls();
    void StartPythonExecutable();
    void CleanupThread();

    // Event Handlers for SecondWindow
    void OnClose(wxCloseEvent &event);
    void OnNext(wxCommandEvent &event);
    void OnTabChanged(wxCommandEvent &event);
    void OnMongoDBButton(wxCommandEvent &event);
    void OnMongoDBPanelClose(wxCommandEvent &event); // Handles close request
    void OnCloseTimer(wxTimerEvent &event);
    void DoDestroy();

    wxDECLARE_EVENT_TABLE();
};

#endif // SECONDWINDOW_H