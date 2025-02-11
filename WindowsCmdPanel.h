#pragma once
#include <wx/wx.h>
#include <wx/thread.h>
#include <wx/msgdlg.h>
#include <wx/log.h>
#ifdef _WIN32
    #include <windows.h>
    #define PY_SSIZE_T_CLEAN
    #include <Python.h>
#endif

class PythonWorkerThread;
class InitTimer;

class WindowsCmdPanel : public wxPanel {
public:
    enum {
        ID_PYTHON_WORK_COMPLETE = wxID_HIGHEST + 100
    };

    WindowsCmdPanel(wxWindow* parent);
    virtual ~WindowsCmdPanel();
    
    void StartPythonWorkerThread(const char* code);

private:
    friend class PythonWorkerThread;
    
    void CreateCmdWindow();
    void OnSize(wxSizeEvent& event);
    void CleanupTimer();
    void ContinueInitialization();
    void EmbedCmdWindow();
    void InitializePythonEnvironment();
    bool ExecutePythonCode(const char* code);
    void HandlePythonError();
    void RunEmbeddedPythonCode();
    void OnPythonWorkComplete(wxCommandEvent& event);

    bool m_initializationComplete;
    InitTimer* m_initTimer;
    int m_initStep;
    PythonWorkerThread* m_workerThread;
    HWND m_hwndCmd;
    DWORD m_dwProcessId;

    wxDECLARE_EVENT_TABLE();
};

class PythonWorkerThread : public wxThread {
public:
    PythonWorkerThread(WindowsCmdPanel* panel, const char* code);
    virtual wxThread::ExitCode Entry() override;

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