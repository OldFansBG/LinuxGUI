#pragma once
#include <wx/wx.h>
#include <wx/process.h>
#include "DockerTransfer.h"
#include "ScriptManager.h"
#include "CustomEvents.h"

#ifdef _WIN32
    #include <Windows.h>
    #define PY_SSIZE_T_CLEAN
    #include <Python.h>
#endif

class InitTimer;

class WindowsCmdPanel : public wxPanel {
public:
    WindowsCmdPanel(wxWindow* parent);
    virtual ~WindowsCmdPanel();
    void ContinueInitialization();

private:
    void CreateCmdWindow();
    void OnSize(wxSizeEvent& event);
    void CleanupTimer();
    void InitializePythonEnvironment();
    void RunEmbeddedPythonCode();
    bool ExecutePythonCode(const char* code);
    void HandlePythonError();

    wxString m_isoPath;
    HWND m_hwndCmd;
    InitTimer* m_initTimer;
    int m_initStep;
    bool m_step6Completed;

    wxDECLARE_EVENT_TABLE();
};

class InitTimer : public wxTimer {
public:
    InitTimer(WindowsCmdPanel* panel) : m_panel(panel) {}
    void Notify() override { m_panel->ContinueInitialization(); }
private:
    WindowsCmdPanel* m_panel;
};