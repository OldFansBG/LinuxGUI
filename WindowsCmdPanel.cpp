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
    EVT_COMMAND(ID_PYTHON_WORK_COMPLETE, wxEVT_COMMAND_TEXT_UPDATED, WindowsCmdPanel::OnPythonWorkComplete)
wxEND_EVENT_TABLE()

WindowsCmdPanel::WindowsCmdPanel(wxWindow* parent)
    : wxPanel(parent), 
      m_initTimer(nullptr), 
      m_initStep(0),
      m_initializationComplete(false),
      m_workerThread(nullptr),
      m_hwndCmd(nullptr),
      m_dwProcessId(0) {
    static bool logInitialized = false;
    if (!logInitialized) {
        wxLog::SetActiveTarget(new wxLogStderr());
        logInitialized = true;
    }
    CreateCmdWindow();
}

WindowsCmdPanel::~WindowsCmdPanel() {
    wxString containerId = ContainerManager::Get().GetCurrentContainerId();
    if (!containerId.IsEmpty()) {
        ContainerManager::Get().CleanupContainer(containerId);
    }
    CleanupTimer();
}

void WindowsCmdPanel::OnSize(wxSizeEvent& event) {
    if (m_hwndCmd) {
        ::SetWindowPos(m_hwndCmd, nullptr, 0, 0, 
                     GetClientSize().GetWidth(), GetClientSize().GetHeight(),
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }
    event.Skip();
}

void WindowsCmdPanel::CreateCmdWindow() {
#ifdef _WIN32
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;

    if (AllocateAndInitializeSid(&ntAuthority, 2,
                                SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
                                0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }

    if (!isAdmin) {
        wxMessageBox("This application requires administrator privileges.", "Admin Rights Required", wxICON_ERROR | wxOK);
        return;
    }

    m_initStep = 0;
    m_initTimer = new InitTimer(this);
    m_initTimer->Start(100);
#endif
}

void WindowsCmdPanel::InitializePythonEnvironment() {
#ifdef _WIN32
    if (Py_IsInitialized()) return;

    std::wstring pythonHome = L"I:\\ProgramFiles\\Anaconda\\envs\\myenv";
    std::wstring pythonPath = pythonHome + L";" + pythonHome + L"\\Lib;" + pythonHome + L"\\DLLs";

    Py_SetPythonHome(pythonHome.c_str());
    _wputenv_s(L"PYTHONHOME", pythonHome.c_str());
    _wputenv_s(L"PYTHONPATH", pythonPath.c_str());

    Py_InitializeEx(1);
    if (Py_IsInitialized()) {
        PyEval_InitThreads();
        PyEval_SaveThread();
    }
#endif
}

bool WindowsCmdPanel::ExecutePythonCode(const char* code) {
#ifdef _WIN32
    PyGILState_STATE gstate = PyGILState_Ensure();
    PyObject* mainModule = PyImport_AddModule("__main__");
    bool success = false;

    if (mainModule) {
        PyObject* globalDict = PyModule_GetDict(mainModule);
        PyObject* result = PyRun_String(code, Py_file_input, globalDict, globalDict);
        success = (result != nullptr);
        Py_XDECREF(result);
    }

    if (!success) HandlePythonError();
    PyGILState_Release(gstate);
    return success;
#else
    return false;
#endif
}

void WindowsCmdPanel::HandlePythonError() {
#ifdef _WIN32
    if (PyErr_Occurred()) {
        PyObject *type, *value, *traceback;
        PyErr_Fetch(&type, &value, &traceback);
        PyErr_NormalizeException(&type, &value, &traceback);

        PyObject* tbModule = PyImport_ImportModule("traceback");
        PyObject* formatFunc = PyObject_GetAttrString(tbModule, "format_exception");
        PyObject* tbList = PyObject_CallFunctionObjArgs(formatFunc, type, value, traceback, NULL);
        PyObject* tbStr = PyUnicode_Join(PyUnicode_FromString(""), tbList);
        const char* fullError = PyUnicode_AsUTF8(tbStr);

        wxLogError("Python Error Details:\n%s", fullError);

        Py_XDECREF(tbStr);
        Py_XDECREF(tbList);
        Py_XDECREF(formatFunc);
        Py_XDECREF(tbModule);
        Py_XDECREF(type);
        Py_XDECREF(value);
        Py_XDECREF(traceback);
    }
#endif
}

void WindowsCmdPanel::RunEmbeddedPythonCode() {
#ifdef _WIN32
    const char* pythonCode = R"PYCODE(
import docker
import os
import time
import traceback
import sys
import tarfile
import io

def copy_iso_to_container(container, host_path, container_dest_path):
    try:
        print(f"[PROGRESS] 20% - Starting ISO copy: {os.path.basename(host_path)}")
        if not os.path.exists(host_path):
            raise FileNotFoundError(f"ISO file not found: {host_path}")
        
        tarstream = io.BytesIO()
        with tarfile.open(fileobj=tarstream, mode="w") as tar:
            tar.add(host_path, arcname=os.path.basename(container_dest_path))
        tarstream.seek(0)
        
        dest_dir = os.path.dirname(container_dest_path) or "/"
        container.put_archive(dest_dir, tarstream)
        print("[PROGRESS] 30% - ISO copy completed")

    except Exception as e:
        print(f"[COPY ERROR] {str(e)}")
        raise

def validate_scripts(base_dir):
    required_scripts = ["setup_output.sh", "setup_chroot.sh", "create_iso.sh"]
    for script in required_scripts:
        script_path = os.path.join(base_dir, script)
        if not os.path.exists(script_path):
            raise FileNotFoundError(f"Missing script: {script_path}")
        if not os.access(script_path, os.X_OK):
            raise PermissionError(f"Script not executable: {script_path}")

def main():
    try:
        print("[PROGRESS] 10% - Initializing Docker client")
        client = docker.from_env()
        client.ping()

        print("[PROGRESS] 15% - Checking existing containers")
        try:
            old_container = client.containers.get("my_unique_container")
            print("[PROGRESS] 16% - Removing old container")
            old_container.remove(force=True)
            time.sleep(2)
        except docker.errors.NotFound:
            pass

        print("[PROGRESS] 20% - Creating new container")
        container = client.containers.run(
            "ubuntu:latest",
            command="sleep infinity",
            name="my_unique_container",
            detach=True,
            privileged=True
        )
        container.reload()
        if container.status != "running":
            raise RuntimeError(f"Container failed to start. Status: {container.status}")

        print("[PROGRESS] 25% - Validating scripts")
        base_dir = r"I:\Files\Desktop\LinuxGUI"
        validate_scripts(base_dir)

        iso_path = r"I:\Files\Downloads\linuxmint-22-cinnamon-64bit.iso"
        copy_iso_to_container(container, iso_path, "/base.iso")

        print("[PROGRESS] 40% - Copying scripts to container")
        for script in ["setup_output.sh", "setup_chroot.sh", "create_iso.sh"]:
            host_path = os.path.join(base_dir, script)
            
            tarstream = io.BytesIO()
            with tarfile.open(fileobj=tarstream, mode="w") as tar:
                tar.add(host_path, arcname=script)
            tarstream.seek(0)
            container.put_archive("/", tarstream)

        print("[PROGRESS] 50% - Setting permissions")
        exit_code, output = container.exec_run("chmod +x /setup_output.sh /setup_chroot.sh /create_iso.sh")
        if exit_code != 0:
            raise RuntimeError(f"Permission setup failed: {output.decode()}")

        print("[PROGRESS] 60% - Running setup_output.sh")
        exit_code, output = container.exec_run("/setup_output.sh")
        if exit_code != 0:
            raise RuntimeError(f"setup_output.sh failed: {output.decode()}")

        print("[PROGRESS] 70% - Running setup_chroot.sh")
        exit_code, output_gen = container.exec_run("/setup_chroot.sh", stream=True)
        ready_signal = False
        for line in output_gen:
            line_str = line.decode().strip()
            print(f"[CHROOT] {line_str}")
            if "Ready" in line_str:
                ready_signal = True
                break
        if not ready_signal or exit_code != 0:
            raise RuntimeError("setup_chroot.sh did not complete successfully")

        print("[PROGRESS] 80% - Running create_iso.sh")
        exit_code, output = container.exec_run("/create_iso.sh", timeout=300)
        if exit_code != 0:
            raise RuntimeError(f"create_iso.sh failed: {output.decode()}")

        print("[PROGRESS] 100% - All operations completed successfully")

    except Exception as e:
        print(f"[FATAL ERROR] {str(e)}")
        traceback.print_exc()
        sys.exit(1)

if __name__ == "__main__":
    main()
)PYCODE";

    StartPythonWorkerThread(pythonCode);
#endif
}

void WindowsCmdPanel::ContinueInitialization() {
    wxLogMessage("ContinueInitialization() - Step %d", m_initStep);
    
    switch (m_initStep) {
        case 0: {
            wxLogMessage("Starting Python worker thread");
            RunEmbeddedPythonCode();
            break;
        }
        case 1: {
            wxLogMessage("Python completed, proceeding to CMD window creation");
            EmbedCmdWindow();
            CleanupTimer();
            break;
        }
    }
}

void WindowsCmdPanel::EmbedCmdWindow() {
#ifdef _WIN32
    wxLogMessage("EmbedCmdWindow() - Creating Docker process");
    
    std::wstring cmd = L"docker exec -it my_unique_container /bin/bash -c \"chroot /root/custom_iso/squashfs-root /bin/bash\"";
    
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = { 0 };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    if (!CreateProcessW(nullptr, cmd.data(),
        nullptr, nullptr, FALSE, 
        CREATE_NEW_CONSOLE | CREATE_NEW_PROCESS_GROUP,
        nullptr, nullptr, &si, &pi)) {
        wxLogError("CreateProcess failed (%lu)", GetLastError());
        return;
    }

    m_dwProcessId = pi.dwProcessId;
    wxLogMessage("Process created PID: %lu", m_dwProcessId);

    const int maxAttempts = 10;
    int attempts = 0;
    while (attempts++ < maxAttempts && !m_hwndCmd) {
        wxLogMessage("Waiting for window creation (attempt %d/%d)", attempts, maxAttempts);
        EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL CALLBACK {
            auto self = reinterpret_cast<WindowsCmdPanel*>(lParam);
            DWORD processId = 0;
            GetWindowThreadProcessId(hwnd, &processId);
            
            if (processId == self->m_dwProcessId) {
                wchar_t className[256];
                GetClassNameW(hwnd, className, 256);
                if (wcscmp(className, L"ConsoleWindowClass") == 0) {
                    self->m_hwndCmd = hwnd;
                    wxLogMessage("Found target window: %p", hwnd);
                    return FALSE;
                }
            }
            return TRUE;
        }, reinterpret_cast<LPARAM>(this));

        if (!m_hwndCmd) Sleep(500);
    }

    if (m_hwndCmd) {
        wxLogMessage("Embedding window %p", m_hwndCmd);
        LONG style = GetWindowLongW(m_hwndCmd, GWL_STYLE);
        style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
        SetWindowLongW(m_hwndCmd, GWL_STYLE, style);
        
        ::SetParent(m_hwndCmd, reinterpret_cast<HWND>(GetHWND()));
        ::ShowWindow(m_hwndCmd, SW_SHOW);
        ::SetWindowPos(m_hwndCmd, nullptr, 0, 0, 
                     GetClientSize().GetWidth(), GetClientSize().GetHeight(),
                     SWP_FRAMECHANGED | SWP_NOZORDER);
        
        wxLogMessage("Window embedded successfully");
    } else {
        wxLogError("Failed to find console window for PID %lu", m_dwProcessId);
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
#endif
}

void WindowsCmdPanel::OnPythonWorkComplete(wxCommandEvent& event) {
    wxLogMessage("OnPythonWorkComplete() - Status: %d", event.GetInt());
    
    if (event.GetInt() == 1) {
        wxLogMessage("All scripts executed successfully");
        m_initStep = 1;
        m_initTimer->StartOnce(100);
    } else {
        wxMessageBox("Critical error during script execution. Check logs.", 
                    "Initialization Failed", wxICON_ERROR);
        CleanupTimer();
    }
}

void WindowsCmdPanel::StartPythonWorkerThread(const char* code) {
#ifdef _WIN32
    if (m_workerThread) {
        m_workerThread->Delete();
    }
    m_workerThread = new PythonWorkerThread(this, code);
    m_workerThread->Run();
#endif
}

void WindowsCmdPanel::CleanupTimer() {
    if (m_initTimer) {
        m_initTimer->Stop();
        delete m_initTimer;
        m_initTimer = nullptr;
    }
}

InitTimer::InitTimer(WindowsCmdPanel* panel) : m_panel(panel) {}

void InitTimer::Notify() {
    m_panel->ContinueInitialization();
}

PythonWorkerThread::PythonWorkerThread(WindowsCmdPanel* panel, const char* code)
    : wxThread(wxTHREAD_DETACHED), m_panel(panel), m_code(code) {}

wxThread::ExitCode PythonWorkerThread::Entry() {
#ifdef _WIN32
    wxLogMessage("PythonWorkerThread::Entry() - Starting");
    bool success = false;
    
    try {
        if (!Py_IsInitialized()) {
            wxLogMessage("Initializing Python environment in thread");
            m_panel->InitializePythonEnvironment();
        }

        wxLogMessage("Acquiring GIL");
        PyGILState_STATE gstate = PyGILState_Ensure();
        
        wxLogMessage("Executing Python code");
        success = m_panel->ExecutePythonCode(m_code);
        
        wxLogMessage("Releasing GIL");
        PyGILState_Release(gstate);
        
        if (success) {
            wxLogMessage("Python execution completed successfully");
        } else {
            wxLogError("Python execution failed");
        }
    } 
    catch (...) {
        wxLogError("Exception in Python worker thread");
        success = false;
    }

    wxCommandEvent* event = new wxCommandEvent(wxEVT_COMMAND_TEXT_UPDATED, 
                                             WindowsCmdPanel::ID_PYTHON_WORK_COMPLETE);
    event->SetInt(success ? 1 : 0);
    wxQueueEvent(m_panel, event);
    
    wxLogMessage("PythonWorkerThread exiting");
    return (ExitCode)0;
#else
    return (ExitCode)1;
#endif
}