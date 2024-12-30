#include "WindowsCmdPanel.h"
#include "ContainerManager.h"
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>  // Add this header for wxStandardPaths
#include <sddl.h>
#include <aclapi.h>
#include <fstream>
#include <iostream>
#include <string>
#include <string>
#include <fstream>
#include <wx/wx.h>
#include <wx/msgdlg.h>
#include <wx/log.h>

#ifdef __WINDOWS__
    #include <windows.h>
#endif

wxBEGIN_EVENT_TABLE(WindowsCmdPanel, wxPanel)
    EVT_SIZE(WindowsCmdPanel::OnSize)
wxEND_EVENT_TABLE()

WindowsCmdPanel::WindowsCmdPanel(wxWindow* parent)
    : wxPanel(parent)
{
    // Initialize logging
    static bool logInitialized = false;
    if (!logInitialized) {
        wxLog::SetActiveTarget(new wxLogStderr());
        logInitialized = true;
        wxLogMessage("Logging initialized in WindowsCmdPanel");
    }
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

void WindowsCmdPanel::CreateCmdWindow() 
{
#ifdef __WINDOWS__
    wxLogMessage("Starting CreateCmdWindow");

    // Admin rights check
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
        wxLogError("Admin rights check failed");
        wxMessageBox("This application requires administrator privileges.\n"
                     "Please run as administrator.", "Admin Rights Required",
                     wxICON_ERROR | wxOK);
        return;
    }
    wxLogMessage("Admin rights check passed");

    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_SHOW;

    ZeroMemory(&pi, sizeof(pi));

    std::wstring baseDir = L"I:\\Files\\Desktop\\LinuxGUI\\build";
    wxLogMessage("Using base directory: %s", baseDir);
    
    std::wstring cmdLine = L"cmd.exe /K \"cd /d " + baseDir +
                           L" && docker run -it --privileged --name my_unique_container ubuntu:latest bash\"";
    wxLogMessage("Docker run command: %s", cmdLine);

    if (CreateProcessW(NULL, const_cast<wchar_t*>(cmdLine.c_str()), NULL, NULL, FALSE,
            CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi)) 
    {
        wxLogMessage("Created process successfully, waiting for container startup...");
        Sleep(3000);  // Allow container to start

        std::wstring captureCmd = L"cmd.exe /C docker ps -aqf name=my_unique_container > \"" + 
                                 baseDir + L"\\container_id.txt\"";
        wxLogMessage("Capturing container ID with command: %s", captureCmd);
        
        int captureResult = _wsystem(captureCmd.c_str());
        wxLogMessage("Container ID capture command result: %d", captureResult);

        Sleep(1000);

        std::string containerId;
        {
            std::ifstream file((baseDir + L"\\container_id.txt").c_str());
            if (file.is_open()) {
                wxLogMessage("Container ID file opened successfully");
                std::getline(file, containerId);
                file.close();
                
                containerId.erase(0, containerId.find_first_not_of(" \n\r\t"));
                containerId.erase(containerId.find_last_not_of(" \n\r\t") + 1);
                wxLogMessage("Container ID captured: '%s'", containerId);
            } else {
                wxLogError("Failed to open container ID file");
                return;
            }
        }

        if (containerId.empty()) {
            wxLogError("Container ID is empty");
            return;
        }

        // Copy scripts
        wxString cmd1 = wxString::Format("docker cp \"%s\\setup_output.sh\" %s:/setup_output.sh", 
                                        baseDir, containerId);
        wxString cmd2 = wxString::Format("docker cp \"%s\\setup_chroot.sh\" %s:/setup_chroot.sh", 
                                        baseDir, containerId);
        wxString cmd3 = wxString::Format("docker cp \"%s\\create_iso.sh\" %s:/create_iso.sh", 
                                        baseDir, containerId);

        wxLogMessage("Executing script copy commands:");
        wxLogMessage("1: %s", cmd1);
        wxLogMessage("2: %s", cmd2);
        wxLogMessage("3: %s", cmd3);

        wxArrayString output, errors;
        int result1 = wxExecute(cmd1, output, errors, wxEXEC_SYNC);
        int result2 = wxExecute(cmd2, output, errors, wxEXEC_SYNC);
        int result3 = wxExecute(cmd3, output, errors, wxEXEC_SYNC);

        if (result1 != 0 || result2 != 0 || result3 != 0) {
            wxLogError("Failed to copy scripts to container");
            return;
        }
        wxLogMessage("Scripts copied successfully");

        // Set execute permissions
        wxString chmodCmd = wxString::Format("docker exec %s chmod +x /setup_output.sh /setup_chroot.sh /create_iso.sh", 
                                           containerId);
        wxExecute(chmodCmd, wxEXEC_SYNC);
        wxLogMessage("Set execute permissions on scripts");

        // Copy ISO if path is set
        if (!m_isoPath.IsEmpty()) {
            wxLogMessage("Copying ISO from: %s", m_isoPath);
            
            // Create mnt/iso directory
            wxString mkdirCmd = wxString::Format("docker exec %s mkdir -p /mnt/iso", containerId);
            wxExecute(mkdirCmd, wxEXEC_SYNC);
            
            // Copy ISO file
            wxString copyIsoCmd = wxString::Format("docker cp \"%s\" %s:/mnt/iso/base.iso",
                                                 m_isoPath, containerId);
            wxLogMessage("Copying ISO with command: %s", copyIsoCmd);

            wxArrayString isoOutput, isoErrors;
            if (wxExecute(copyIsoCmd, isoOutput, isoErrors, wxEXEC_SYNC) != 0) {
                wxLogError("Failed to copy ISO to container");
                for(const auto& error : isoErrors) {
                    wxLogError("Error: %s", error);
                }
                return;
            }
            wxLogMessage("ISO copied successfully");

            // Set permissions on ISO directory
            wxString chmodIsoCmd = wxString::Format("docker exec %s chmod -R 777 /mnt/iso", containerId);
            wxExecute(chmodIsoCmd, wxEXEC_SYNC);
            wxLogMessage("Set permissions on ISO directory");

            // Run setup_output.sh
            wxString setupOutputCmd = wxString::Format("docker exec %s /setup_output.sh", containerId);
            wxLogMessage("Running setup_output.sh");
            
            if (wxExecute(setupOutputCmd, output, errors, wxEXEC_SYNC) != 0) {
                wxLogError("Failed to run setup_output.sh");
                for(const auto& error : errors) {
                    wxLogError("Error: %s", error);
                }
                return;
            }
            wxLogMessage("setup_output.sh completed successfully");

            // Run setup_chroot.sh
            wxString setupChrootCmd = wxString::Format("docker exec %s /setup_chroot.sh", containerId);
            wxLogMessage("Running setup_chroot.sh");
            
            if (wxExecute(setupChrootCmd, output, errors, wxEXEC_SYNC) != 0) {
                wxLogError("Failed to run setup_chroot.sh");
                for(const auto& error : errors) {
                    wxLogError("Error: %s", error);
                }
                return;
            }
            wxLogMessage("setup_chroot.sh completed successfully");
        } else {
            wxLogMessage("No ISO path set, skipping ISO copy and script execution");
        }

        m_containerId = wxString(containerId);

        // Embed the CMD window
        m_hwndCmd = FindWindowExW(NULL, NULL, L"ConsoleWindowClass", NULL);
        
        if (m_hwndCmd) 
        {
            wxLogMessage("Found CMD window, setting up window properties");
            LONG style = GetWindowLong(m_hwndCmd, GWL_STYLE);
            style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
            SetWindowLong(m_hwndCmd, GWL_STYLE, style);
            
            LONG exStyle = GetWindowLong(m_hwndCmd, GWL_EXSTYLE);
            exStyle &= ~(WS_EX_CLIENTEDGE | WS_EX_WINDOWEDGE);
            SetWindowLong(m_hwndCmd, GWL_EXSTYLE, exStyle);

            ::SetParent(m_hwndCmd, (HWND)GetHWND());
            ::ShowWindow(m_hwndCmd, SW_SHOW);
            ::SetWindowPos(
                m_hwndCmd, NULL, 0, 0, GetClientSize().GetWidth(), GetClientSize().GetHeight(), 
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
            ::SetForegroundWindow(m_hwndCmd);
            ::SetFocus(m_hwndCmd);
            wxLogMessage("CMD window embedded successfully");
        }
        else {
            wxLogError("Failed to find CMD window");
        }

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        wxLogMessage("Process handles clesaned up");
    } 
    else 
    {
        wxLogError("Failed to create CMD process");
    }
#endif
}