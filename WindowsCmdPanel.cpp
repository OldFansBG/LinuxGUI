#include "WindowsCmdPanel.h"
#include "ContainerManager.h"
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/msgdlg.h>
#include <wx/log.h>
#include <fstream>
#include <sstream>

#ifdef _WIN32
    #include <Python.h>
#endif

wxBEGIN_EVENT_TABLE(WindowsCmdPanel, wxPanel)
    EVT_SIZE(WindowsCmdPanel::OnSize)
wxEND_EVENT_TABLE()

// Constructor
WindowsCmdPanel::WindowsCmdPanel(wxWindow* parent)
    : wxPanel(parent), m_hwndCmd(nullptr), m_initTimer(nullptr),
      m_initStep(0), m_step6Completed(false) {
    // Initialize logging
    static bool logInitialized = false;
    if (!logInitialized) {
        wxLog::SetActiveTarget(new wxLogStderr());
        logInitialized = true;
        wxLogMessage("Logging initialized in WindowsCmdPanel");
    }

    // Start the initialization sequence
    CreateCmdWindow(); // Start the timer and initialization
}

// Destructor
WindowsCmdPanel::~WindowsCmdPanel() {
#ifdef _WIN32
    if (m_hwndCmd) {
        wxLogMessage("Closing CMD window");
        ::SendMessage(m_hwndCmd, WM_CLOSE, 0, 0);
    }
#endif

    wxString containerId = ContainerManager::Get().GetCurrentContainerId();
    if (!containerId.IsEmpty()) {
        wxLogMessage("Cleaning up container: %s", containerId);
        ContainerManager::Get().CleanupContainer(containerId);
    }

    CleanupTimer();
}

// Handle window resize
void WindowsCmdPanel::OnSize(wxSizeEvent& event) {
#ifdef _WIN32
    if (m_hwndCmd) {
        wxSize size = GetClientSize();
        wxLogMessage("Resizing CMD window to %dx%d", size.GetWidth(), size.GetHeight());
        ::MoveWindow(m_hwndCmd, 0, 0, size.GetWidth(), size.GetHeight(), TRUE);
    }
#endif
    event.Skip();
}

// Create CMD window
void WindowsCmdPanel::CreateCmdWindow() {
#ifdef _WIN32
    wxLogMessage("Starting CreateCmdWindow");

    // Admin rights check
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;

    wxLogMessage("Checking for admin rights");
    if (AllocateAndInitializeSid(&ntAuthority, 2,
                                SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
                                0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }

    if (!isAdmin) {
        wxLogError("Admin rights check failed");
        wxMessageBox("This application requires administrator privileges.\n"
                     "Please run as administrator.", "Admin Rights Required",
                     wxICON_ERROR | wxOK);
        return;
    }
    wxLogMessage("Admin rights confirmed");

    // Start initialization sequence
    wxLogMessage("Starting initialization sequence");
    m_initStep = 0;
    m_initTimer = new InitTimer(this);
    m_initTimer->Start(100); // Start first step after 100ms
#endif
}

// Initialize Python environment
void WindowsCmdPanel::InitializePythonEnvironment() {
#ifdef _WIN32
    wxLogMessage("Initializing Python environment");
    try {
        if (Py_IsInitialized()) {
            wxLogMessage("Python already initialized");
            return;
        }

        // Set Python home and environment variables
        std::wstring pythonHome = L"I:\\ProgramFiles\\Anaconda\\envs\\myenv";
        Py_SetPythonHome(pythonHome.c_str());
        _wputenv_s(L"PYTHONHOME", pythonHome.c_str());
        _wputenv_s(L"PYTHONPATH", (pythonHome + L"\\Lib;" + pythonHome + L"\\DLLs").c_str());

        // Initialize Python
        Py_Initialize();
        if (!Py_IsInitialized()) {
            wxLogError("Failed to initialize Python");
            return;
        }

        // Initialize GIL
        PyEval_InitThreads();
        PyThreadState* state = PyEval_SaveThread(); // Release GIL

        wxLogMessage("Python initialized");
    }
    catch (...) {
        wxLogError("Exception during Python init");
    }
#endif
}

// Execute Python code
bool WindowsCmdPanel::ExecutePythonCode(const char* code) {
#ifdef _WIN32
    wxLogMessage("Executing Python code");

    // Acquire the GIL
    PyGILState_STATE gstate = PyGILState_Ensure();

    PyObject* mainModule = PyImport_AddModule("__main__");
    if (!mainModule) {
        wxLogError("Failed to get Python main module");
        PyGILState_Release(gstate); // Release GIL before returning
        return false;
    }

    PyObject* globalDict = PyModule_GetDict(mainModule);
    PyObject* result = PyRun_String(code, Py_file_input, globalDict, globalDict);

    if (!result) {
        HandlePythonError();
        PyGILState_Release(gstate);
        return false;
    }

    Py_XDECREF(result);
    PyGILState_Release(gstate); // Release the GIL
    return true;
#else
    return false;
#endif
}

// Handle Python errors
void WindowsCmdPanel::HandlePythonError() {
#ifdef _WIN32
    if (PyErr_Occurred()) {
        wxLogError("Python error occurred");
        PyObject *type, *value, *traceback;
        PyErr_Fetch(&type, &value, &traceback);
        PyErr_NormalizeException(&type, &value, &traceback);

        // Convert error to string
        PyObject* str = PyObject_Str(value);
        const char* errorMsg = PyUnicode_AsUTF8(str);
        wxLogError("Python Error: %s", errorMsg);

        // Cleanup
        Py_XDECREF(str);
        Py_XDECREF(type);
        Py_XDECREF(value);
        Py_XDECREF(traceback);
    }
#endif
}

// Run embedded Python code
void WindowsCmdPanel::RunEmbeddedPythonCode() {
#ifdef _WIN32
    wxLogMessage("Running embedded Python code");

    if (!Py_IsInitialized()) {
        wxLogMessage("Initializing Python environment before running code");
        InitializePythonEnvironment();
    }

    const char* pythonCode = R"(
import docker
import sys
import traceback

try:
    client = docker.from_env()
    # Verify Docker connection
    client.ping()
    container = client.containers.run(
        "ubuntu:latest",
        command="sleep infinity",
        name="my_unique_container",
        detach=True,
        privileged=True
    )
    print(f"[PYTHON] Container created: {container.id}")
except Exception as e:
    print(f"[PYTHON ERROR] {str(e)}")
    print("[PYTHON ERROR] Traceback:")
    traceback.print_exc()
    sys.exit(1)
    )";

    if (!ExecutePythonCode(pythonCode)) {
        wxLogError("Failed to execute Python code");
    }
#endif
}

// Continue initialization
// Continue initialization
void WindowsCmdPanel::ContinueInitialization() {
    wxLogMessage("Continuing initialization step %d", m_initStep);

    switch (m_initStep) {
        case 0: { // Step 0: Run embedded Python code
            wxLogMessage("Step 0: Running embedded Python code");
            RunEmbeddedPythonCode();
            m_initStep++; // Move to next step
            break;
        }
        case 1: { // Step 1: Finalize initialization
            wxLogMessage("Step 1: Initialization complete");
            CleanupTimer(); // Stop the timer
            break;
        }
        // Add more steps if needed
    }
}

// Cleanup timer
void WindowsCmdPanel::CleanupTimer() {
    if (m_initTimer) {
        wxLogMessage("Cleaning up initialization timer");
        m_initTimer->Stop();
        delete m_initTimer;
        m_initTimer = nullptr;
    }
}