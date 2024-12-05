#include "WindowsCmdPanel.h"
#include <wx/file.h>
#include <wx/filename.h>

#ifdef __WINDOWS__
#include <wx/msw/private.h>
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

        // Copy setup.sh to container
        wxString copySetupCmd = wxString::Format("docker cp \"setup.sh\" %s:/setup.sh",
                                               containerId);
        if (wxExecute(copySetupCmd, wxEXEC_SYNC) != 0) {
            wxLogError("Failed to copy setup script to container");
            return;
        }

        // Make setup.sh executable
        wxString chmodCmd = "docker exec " + containerId + " chmod +x /setup.sh";
        if (wxExecute(chmodCmd, wxEXEC_SYNC) != 0) {
            wxLogError("Failed to make setup script executable");
            return;
        }

        // Run setup.sh in interactive mode
        wxString dockerExec = "docker exec -it " + containerId + " /setup.sh";
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
        else {
            wxLogError("Failed to create command window process");
        }
    }
    else {
        wxLogError("Failed to create Docker container");
    }
#endif
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

void WindowsCmdPanel::OnDockerProgress(int progress, const wxString& status)
{
    wxLogMessage(status);
}