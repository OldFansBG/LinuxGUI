#ifndef PYTHONWORKERTHREAD_H
#define PYTHONWORKERTHREAD_H

#include <wx/thread.h>
#include <wx/event.h>
#include <wx/string.h>
#include "CustomEvents.h" // Include the custom events header

// Forward declaration of SecondWindow to avoid circular dependency
class SecondWindow;

class PythonWorkerThread : public wxThread {
public:
    PythonWorkerThread(SecondWindow* handler, const wxString& isoPath);
    virtual ~PythonWorkerThread();

protected:
    virtual ExitCode Entry() override;

private:
    SecondWindow* m_handler; // Pointer to the main window for event posting
    wxString m_isoPath;      // Path to the ISO file

    // Python-related methods
    void InitializePython();
    bool ExecutePythonCode(const char* code);
    void HandlePythonError();
    const char* GeneratePythonCode();
    void SendLogUpdate(const wxString& message); // Send log updates to the GUI
};

#endif // PYTHONWORKERTHREAD_H