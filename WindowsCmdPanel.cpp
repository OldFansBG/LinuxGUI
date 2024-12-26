#include "WindowsCmdPanel.h"
#include "ContainerManager.h"  // Add this include at the top
#include <wx/file.h>
#include <wx/filename.h>

#ifdef __WINDOWS__
    #include <windows.h>
#endif

wxBEGIN_EVENT_TABLE(WindowsCmdPanel, wxPanel)
    EVT_SIZE(WindowsCmdPanel::OnSize)
wxEND_EVENT_TABLE()

WindowsCmdPanel::WindowsCmdPanel(wxWindow* parent)
    : wxPanel(parent)
#ifdef __WINDOWS__
    , m_hwndCmd(nullptr)
#endif
{
}

WindowsCmdPanel::~WindowsCmdPanel()
{
#ifdef __WINDOWS__
    if (m_hwndCmd)
    {
        ::SendMessage(m_hwndCmd, WM_CLOSE, 0, 0);
    }
    
    wxString containerId = ContainerManager::Get().GetCurrentContainerId();
    if (!containerId.IsEmpty()) {
        ContainerManager::Get().CleanupContainer(containerId);
    }
#endif
}

void WindowsCmdPanel::SetISOPath(const wxString& path)
{
    m_isoPath = path;
    if (!m_isoPath.IsEmpty()) {
        CreateCmdWindow();
    }
}

void WindowsCmdPanel::CreateCmdWindow()
{
#ifdef __WINDOWS__
    if (m_isoPath.IsEmpty()) {
        wxLogError("ISO path is empty. Cannot create command window.");
        return;
    }

    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    ZeroMemory(&pi, sizeof(pi));

    // Create Docker container
    wxArrayString output;
    wxExecute("docker run -d --privileged ubuntu:latest tail -f /dev/null", output, wxEXEC_SYNC);
    
    if (!output.IsEmpty()) {
        wxString containerId = output[0].Trim();
        
        // Save container ID using ContainerManager
        if (!ContainerManager::Get().SaveContainerId(containerId, m_isoPath)) {
            wxLogError("Failed to save container ID");
            return;
        }

        // Install requirements first
        wxString setupCmd = "docker exec " + containerId + " /bin/bash -c \"apt-get update && "
            "DEBIAN_FRONTEND=noninteractive apt-get install -y "
            "squashfs-tools xorriso grub-pc-bin grub-efi-amd64-bin mtools "
            "binutils\"";
            
        wxArrayString setupOutput, setupError;
        if (wxExecute(setupCmd, setupOutput, setupError, wxEXEC_SYNC) != 0) {
            wxLogError("Failed to install required packages: " + wxJoin(setupError, '\n'));
            return;
        }

        // Copy ISO to container
        wxString copyISOCmd = wxString::Format("docker cp \"%s\" %s:/iso.iso", 
                                             m_isoPath, containerId);
        if (wxExecute(copyISOCmd, wxEXEC_SYNC) != 0) {
            wxLogError("Failed to copy ISO to container");
            return;
        }

        // Copy scripts to container
        if (!CopyScriptsToContainer(containerId)) {
            wxLogError("Failed to copy and set up scripts in container");
            return;
        }

        // Run setup_chroot.sh in interactive mode
        wxString dockerExec = "docker exec -it " + containerId + " /setup_chroot.sh";
        wchar_t cmdLine[4096];
        swprintf(cmdLine, 4096, L"cmd.exe /K %s", dockerExec.wc_str());
        
        if (CreateProcessW(NULL, cmdLine, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi))
        {
            Sleep(200);  // Give the window time to appear
            m_hwndCmd = FindWindowExW(NULL, NULL, L"ConsoleWindowClass", NULL);
            if (m_hwndCmd)
            {
                // Remove window decorations
                LONG style = GetWindowLong(m_hwndCmd, GWL_STYLE);
                style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
                SetWindowLong(m_hwndCmd, GWL_STYLE, style);
                
                LONG exStyle = GetWindowLong(m_hwndCmd, GWL_EXSTYLE);
                exStyle &= ~(WS_EX_CLIENTEDGE | WS_EX_WINDOWEDGE);
                SetWindowLong(m_hwndCmd, GWL_EXSTYLE, exStyle);

                // Embed the CMD window
                ::SetParent(m_hwndCmd, (HWND)GetHWND());
                ::ShowWindow(m_hwndCmd, SW_SHOW);
                ::SetWindowPos(m_hwndCmd, NULL, 0, 0, 0, 0, 
                             SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
                ::SetForegroundWindow(m_hwndCmd);
                ::SetFocus(m_hwndCmd);
            }
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
    }
    else {
        wxLogError("Failed to create Docker container");
    }
#endif
}

bool WindowsCmdPanel::CopyScriptsToContainer(const wxString& containerId) {
    const std::vector<wxString> scripts = {
        "setup_chroot.sh",
        "create_iso.sh",
    };

    for (const auto& script : scripts) {
        wxString copyCmd = wxString::Format(
            "docker cp \"%s\" %s:/%s",
            script, containerId, script);
        if (wxExecute(copyCmd, wxEXEC_SYNC) != 0) {
            return false;
        }

        wxString chmodCmd = wxString::Format(
            "docker exec %s chmod +x /%s",
            containerId, script);
        if (wxExecute(chmodCmd, wxEXEC_SYNC) != 0) {
            return false;
        }
    }

    return true;
}

bool WindowsCmdPanel::CreateOutputDirectory(const wxString& containerId) {
    // First check if directory already exists
    wxString checkCmd = wxString::Format(
        "docker exec %s test -d /output", 
        containerId);
    if (wxExecute(checkCmd, wxEXEC_SYNC) == 0) {
        // Directory exists, ensure permissions
        wxString chmodCmd = wxString::Format(
            "docker exec %s chmod -R 777 /output",
            containerId);
        return wxExecute(chmodCmd, wxEXEC_SYNC) == 0;
    }

    // Create directory with full permissions
    wxArrayString output, errors;
    wxString mkdirCmd = wxString::Format(
        "docker exec %s /bin/bash -c '"
        "mkdir -p /output && "
        "chmod -R 777 /output && "
        "ls -ld /output'",
        containerId);

    int result = wxExecute(mkdirCmd, output, errors, wxEXEC_SYNC);
    if (result != 0) {
        wxString errorMsg = "Failed to create output directory:\n";
        for (const auto& error : errors) {
            errorMsg += error + "\n";
        }
        wxLogError(errorMsg);
        return false;
    }

    return true;
}

bool WindowsCmdPanel::VerifyContainerSetup(const wxString& containerId) {
    wxArrayString output, errors;
    
    // Verify container is running
    wxString checkContainerCmd = wxString::Format(
        "docker ps -q -f id=%s",
        containerId);
    wxExecute(checkContainerCmd, output, errors, wxEXEC_SYNC);
    if (output.IsEmpty()) {
        wxLogError("Container is not running");
        return false;
    }

    // Verify output directory exists and is writable
    wxString checkDirCmd = wxString::Format(
        "docker exec %s /bin/bash -c '"
        "test -d /output && "
        "test -w /output'",
        containerId);
    if (wxExecute(checkDirCmd, wxEXEC_SYNC) != 0) {
        wxLogError("Output directory is not accessible or writable");
        return false;
    }

    return true;
}
void WindowsCmdPanel::OnSize(wxSizeEvent& event)
{
#ifdef __WINDOWS__
    if (m_hwndCmd)
    {
        wxSize size = GetClientSize();
        ::MoveWindow(m_hwndCmd, 0, 0, size.GetWidth(), size.GetHeight(), TRUE);
    }
#endif
    event.Skip();
}