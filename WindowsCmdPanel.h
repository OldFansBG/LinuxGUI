#ifndef WINDOWSCMDPANEL_H
#define WINDOWSCMDPANEL_H

#include <wx/wx.h>
#include <wx/process.h>
#include "DockerTransfer.h"
#include "ScriptManager.h"
#include "CustomEvents.h"

#ifdef _WIN32
    #include <windows.h>
    #include <Python.h>
#endif

class InitTimer;

class WindowsCmdPanel : public wxPanel {
public:
    WindowsCmdPanel(wxWindow* parent);
    virtual ~WindowsCmdPanel();
    void SetISOPath(const wxString& path);
    void ContinueInitialization();
    wxString GetFileSizeString(const wxString& filePath);
    void ShowCompletionDialog(const wxString& isoPath);

private:
    void CreateCmdWindow();
    void OnSize(wxSizeEvent& event);
    void CleanupTimer();
    bool CheckISOExistsInContainer(const wxString& containerId);
    void InitializePythonEnvironment();
    void RunEmbeddedPythonCode();
    bool ExecutePythonCode(const char* code);
    void HandlePythonError();

#ifdef _WIN32
    HWND m_hwndCmd;
#endif

    wxString m_isoPath;
    wxString m_containerId;
    bool m_step6Completed;
    InitTimer* m_initTimer;
    int m_initStep;

    wxDECLARE_EVENT_TABLE();
};

class InitTimer : public wxTimer {
public:
    InitTimer(WindowsCmdPanel* panel) : m_panel(panel) {}
    void Notify() override { m_panel->ContinueInitialization(); }
private:
    WindowsCmdPanel* m_panel;
};

#endif // WINDOWSCMDPANEL_H