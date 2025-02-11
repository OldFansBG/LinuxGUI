#ifndef PYTHONWORKERTHREAD_H
#define PYTHONWORKERTHREAD_H

#include <wx/wx.h>
#include <wx/thread.h>
#include <string>

// Forward declarations
class SecondWindow;

class PythonWorkerThread : public wxThread {
public:
    PythonWorkerThread(SecondWindow* handler, const wxString& isoPath);
    virtual ~PythonWorkerThread();

protected:
    virtual ExitCode Entry() override;

private:
    // Python Related Methods
    bool ExecutePythonCode(const char* code);
    const char* GeneratePythonCode();
    void HandlePythonError();
    void SendLogUpdate(const wxString& message);

    // Member Variables
    SecondWindow* m_handler;
    wxString m_isoPath;
    std::string m_generatedCode;  // Store the generated code
};

// Custom Event Declarations (if not already in CustomEvents.h)
wxDECLARE_EVENT(PYTHON_TASK_COMPLETED, wxCommandEvent);
wxDECLARE_EVENT(PYTHON_LOG_UPDATE, wxCommandEvent);

#endif // PYTHONWORKERTHREAD_H