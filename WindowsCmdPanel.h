#pragma once
#include <wx/wx.h>
#include <wx/thread.h>  // Add threading support
#include "DockerTransfer.h"
#include "ScriptManager.h"
#include <iostream>

#ifdef _WIN32
    #include <windows.h>
    #define PY_SSIZE_T_CLEAN
    #include <Python.h>
#endif

class PythonWorkerThread;  // Forward declaration
class InitTimer;  // Add this line

class WindowsCmdPanel : public wxPanel {
public:
    WindowsCmdPanel(wxWindow* parent);
    virtual ~WindowsCmdPanel();
    
    void ContinueInitialization();
    bool ExecutePythonCode(const char* code);  // Made public for external access

    // Thread event handlers
    void OnPythonWorkComplete(wxCommandEvent& event);
    void OnPythonWorkProgress(wxCommandEvent& event);

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
    PythonWorkerThread* m_workerThread;  // Worker thread for Python execution

    // Event IDs for thread communication
    enum {
        ID_PYTHON_WORK_COMPLETE = wxID_HIGHEST + 100,
        ID_PYTHON_WORK_PROGRESS
    };

    wxDECLARE_EVENT_TABLE();
};

// Thread class for Python execution
class PythonWorkerThread : public wxThread {
public:
    PythonWorkerThread(WindowsCmdPanel* panel, const char* code);
    virtual ExitCode Entry() override;

private:
    WindowsCmdPanel* m_panel;
    const char* m_code;
};

class InitTimer : public wxTimer {
public:
    explicit InitTimer(WindowsCmdPanel* panel);
    void Notify() override;
    
private:
    WindowsCmdPanel* m_panel;
};