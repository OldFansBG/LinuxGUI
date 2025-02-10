#pragma once
#include <wx/wx.h>
#include <wx/process.h>
#include "DockerTransfer.h"
#include "ScriptManager.h"
#include <iostream>

#ifdef _WIN32
    #include <windows.h>
    #define PY_SSIZE_T_CLEAN
    #include <Python.h>
#endif

class InitTimer;

class WindowsCmdPanel : public wxPanel {
public:
    WindowsCmdPanel(wxWindow* parent);
    virtual ~WindowsCmdPanel();
    
    void ContinueInitialization();
    bool ExecutePythonCode(const char* code);  // Made public for external access

private:
    // Core functionality methods
    void CreateCmdWindow();
    void OnSize(wxSizeEvent& event);
    void CleanupTimer();
    
    // Python/Docker operations
    void InitializePythonEnvironment();
    void RunEmbeddedPythonCode();
    void HandlePythonError();
    bool m_initializationComplete;

    // UI/Feedback methods
    wxString GetFileSizeString(const wxString& filePath);
    void ShowCompletionDialog(const wxString& isoPath);

    // Member variables
    wxString m_isoPath;
    InitTimer* m_initTimer;
    int m_initStep;

    wxDECLARE_EVENT_TABLE();
};

class InitTimer : public wxTimer {
public:
    explicit InitTimer(WindowsCmdPanel* panel);
    void Notify() override;
    
private:
    WindowsCmdPanel* m_panel;
};