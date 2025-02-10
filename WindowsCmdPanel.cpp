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
    : wxPanel(parent), 
      m_initTimer(nullptr), 
      m_initStep(0),
      m_initializationComplete(false) {
    // Initialize logging
    static bool logInitialized = false;
    if (!logInitialized) {
        wxLog::SetActiveTarget(new wxLogStderr());
        logInitialized = true;
        wxLogMessage("Logging initialized in WindowsCmdPanel");
    }

    // Start the initialization sequence
    CreateCmdWindow();
}

// Destructor
WindowsCmdPanel::~WindowsCmdPanel() {
    wxString containerId = ContainerManager::Get().GetCurrentContainerId();
    if (!containerId.IsEmpty()) {
        wxLogMessage("Cleaning up container: %s", containerId);
        ContainerManager::Get().CleanupContainer(containerId);
    }

    CleanupTimer();
}

// Handle window resize
void WindowsCmdPanel::OnSize(wxSizeEvent& event) {
    event.Skip();
}

// Create CMD window (now just starts the initialization sequence)
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

        // Handle progress updates
        if (strstr(errorMsg, "COPY_PROGRESS:") != nullptr) {
            int progress = atoi(errorMsg + 14);
            wxLogMessage("ISO Copy Progress: %d%%", progress);
            Py_XDECREF(str);
            Py_XDECREF(type);
            Py_XDECREF(value);
            Py_XDECREF(traceback);
            return;
        }

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

        const char* pythonCode = R"PYCODE(
import docker
import os
import time
import traceback
import sys
import tarfile
import io

def copy_iso_to_container(container, host_path, container_dest_path):
    """Copy ISO file into container using a tar archive."""
    try:
        print(f"[PYTHON] Copying ISO: {os.path.basename(host_path)}")
        
        # Verify file exists before copying
        if not os.path.exists(host_path):
            raise FileNotFoundError(f"ISO file not found: {host_path}")
        
        # Create an in-memory tar archive containing the ISO file
        tarstream = io.BytesIO()
        with tarfile.open(fileobj=tarstream, mode="w") as tar:
            # Place the file in the tar archive with the desired destination filename
            tar.add(host_path, arcname=os.path.basename(container_dest_path))
        tarstream.seek(0)
        
        # Put the archive into the container.
        # The destination must be an existing directory. Here, we extract into the container's root ("/")
        dest_dir = os.path.dirname(container_dest_path) or "/"
        container.put_archive(dest_dir, tarstream)
        print("[PYTHON] ISO copy completed")
    except Exception as e:
        print(f"[COPY ERROR] {str(e)}")
        raise

def main():
    try:
        print("[PYTHON] Attempting to import docker module")
        from docker import APIClient
        
        # 1. Initialize Docker client
        print("[PYTHON] Initializing Docker client")
        try:
            client = docker.from_env()
            client.ping()
        except docker.errors.DockerException as de:
            print(f"[DOCKER ERROR] Docker connection failed: {str(de)}")
            print("Verify Docker Desktop is running and accessible")
            raise
        
        print("[PYTHON] Connected to Docker daemon")

        # 2. Create container with conflict handling
        print("[PYTHON] Creating container...")
        try:
            container = client.containers.run(
                "ubuntu:latest",
                command="sleep infinity",
                name="my_unique_container",
                detach=True,
                privileged=True,
                remove=True
            )
        except docker.errors.APIError as ae:
            if "Conflict" in str(ae):
                print("[WARNING] Container already exists, removing...")
                old_container = client.containers.get("my_unique_container")
                old_container.remove(force=True)
                container = client.containers.run(
                    "ubuntu:latest",
                    command="sleep infinity",
                    name="my_unique_container",
                    detach=True,
                    privileged=True
                )
            else:
                raise

        print(f"[PYTHON] Container created: {container.id}")

        # 3. Copy ISO file (UPDATE THIS PATH)
        iso_path = r"I:\Files\Downloads\linuxmint-22-cinnamon-64bit.iso"  # updated iso path
        copy_iso_to_container(container, iso_path, "/base.iso")

        # 4. Verify ISO copy
        exit_code, output = container.exec_run("stat -c%s /base.iso")
        host_size = os.path.getsize(iso_path)
        container_size = int(output.decode().strip())
        
        if host_size != container_size:
            raise RuntimeError(f"ISO copy mismatch! Host: {host_size} Container: {container_size}")
        print("[PYTHON] ISO verification passed")

        # 5. Copy scripts (UPDATE THIS PATH)
        scripts = ["setup_output.sh", "setup_chroot.sh", "create_iso.sh"]
        base_dir = r"I:\Files\Desktop\LinuxGUI"  # updated directory containing the scripts
        
        for script in scripts:
            host_path = os.path.join(base_dir, script)
            if not os.path.exists(host_path):
                raise FileNotFoundError(f"Script file not found: {host_path}")

            # Create an in-memory tar archive for the script
            tarstream = io.BytesIO()
            with tarfile.open(fileobj=tarstream, mode="w") as tar:
                tar.add(host_path, arcname=script)
            tarstream.seek(0)
            container.put_archive("/", tarstream)
            print(f"[PYTHON] Copied {script} to container")

        # 6. Set permissions
        exit_code, output = container.exec_run("chmod +x /setup_output.sh /setup_chroot.sh /create_iso.sh")
        if exit_code != 0:
            raise RuntimeError(f"Failed to set permissions: {output.decode()}")

        # 7. Create directories
        for path in ["/mnt/iso", "/output"]:
            exit_code, output = container.exec_run(f"mkdir -p {path}")
            if exit_code != 0:
                raise RuntimeError(f"Failed to create {path}: {output.decode()}")

        # 8. Execute setup_output.sh
        exit_code, output = container.exec_run("/setup_output.sh")
        if exit_code != 0:
            raise RuntimeError(f"setup_output.sh failed: {output.decode()}")

        # 9. Execute setup_chroot.sh
        print("[PYTHON] Entering chroot environment...")
        exit_code, output = container.exec_run("/setup_chroot.sh", stream=True)
        for line in output:
            print(line.decode().strip())
            if "Ready" in line.decode():
                break

    except Exception as e:
        print(f"[FATAL ERROR] {str(e)}")
        traceback.print_exc()
        sys.exit(1)

if __name__ == "__main__":
    main()
)PYCODE";

    if (!ExecutePythonCode(pythonCode)) {
        wxLogError("Failed to execute Python code");
        wxMessageBox("Python operations failed. Check logs for details.", 
                    "Critical Error", wxICON_ERROR);
    }
#endif
}

// Continue initialization
void WindowsCmdPanel::ContinueInitialization() {
    wxLogMessage("Continuing initialization step %d", m_initStep);

    switch (m_initStep) {
        case 0: { // Run Python operations
            wxLogMessage("Step 0: Running embedded Python code");
            RunEmbeddedPythonCode();
            m_initStep++;
            m_initTimer->StartOnce(3000); // Wait 3s before next step
            break;
        }
        case 1: { // Create CMD window after Python completes
            wxLogMessage("Step 1: Creating chroot terminal");
            
            #ifdef _WIN32
            // Command to attach to container's chroot environment
            std::wstring cmd = L"docker exec -it my_unique_container /bin/bash -c \"chroot /root/custom_iso/squashfs-root /bin/bash\"";
            
            STARTUPINFOW si = { sizeof(si) };
            PROCESS_INFORMATION pi;
            si.dwFlags = STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_HIDE;

            if (CreateProcessW(NULL, const_cast<wchar_t*>(cmd.c_str()),
                NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi)) {
                
                // Wait for console window to be created
                Sleep(2000);
                
                // Find and embed the new CMD window
                HWND hwndCmd = FindWindowExW(NULL, NULL, L"ConsoleWindowClass", NULL);
                if (hwndCmd) {
                    // Window styling
                    LONG style = GetWindowLong(hwndCmd, GWL_STYLE);
                    style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
                    SetWindowLong(hwndCmd, GWL_STYLE, style);

                    // Embedding logic
                    ::SetParent(hwndCmd, (HWND)GetHWND());
                    ::ShowWindow(hwndCmd, SW_SHOW);
                    ::SetWindowPos(hwndCmd, NULL, 0, 0, 
                                 GetClientSize().GetWidth(), GetClientSize().GetHeight(),
                                 SWP_FRAMECHANGED);
                    wxLogMessage("CMD window embedded successfully");
                }
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }
            #endif

            CleanupTimer();
            break;
        }
    }
}

// Get file size as a string
wxString WindowsCmdPanel::GetFileSizeString(const wxString& filePath) {
    wxULongLong size = wxFileName::GetSize(filePath);

    if (size == wxInvalidSize) {
        return "Unknown size";
    }

    const double KB = 1024;
    const double MB = KB * 1024;
    const double GB = MB * 1024;

    if (size.ToDouble() >= GB) {
        return wxString::Format("%.2f GB", size.ToDouble() / GB);
    } else if (size.ToDouble() >= MB) {
        return wxString::Format("%.2f MB", size.ToDouble() / MB);
    } else if (size.ToDouble() >= KB) {
        return wxString::Format("%.2f KB", size.ToDouble() / KB);
    } else {
        return wxString::Format("%llu bytes", size.GetValue());
    }
}

// Show completion dialog
void WindowsCmdPanel::ShowCompletionDialog(const wxString& isoPath) {
    wxString msg = wxString::Format(
        "Custom ISO created at:\n%s\n\n"
        "Size: %s\n\n"
        "Would you like to open the containing folder?",
        isoPath, GetFileSizeString(isoPath)
    );

    if (wxMessageBox(msg, "ISO Creation Complete", 
                    wxYES_NO | wxICON_INFORMATION) == wxYES) {
        wxString explorerCmd = wxString::Format("explorer.exe /select,\"%s\"", isoPath);
        wxExecute(explorerCmd);
    }
}

// Cleanup timer
void WindowsCmdPanel::CleanupTimer() {
    if (m_initTimer) {
        wxLogMessage("Stopping initialization timer");
        m_initTimer->Stop();
        delete m_initTimer;
        m_initTimer = nullptr;
    }
}

// InitTimer implementation
InitTimer::InitTimer(WindowsCmdPanel* panel) : m_panel(panel) {}

void InitTimer::Notify() {
    m_panel->ContinueInitialization();
}