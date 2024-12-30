#include "WindowsCmdPanel.h"
#include "ContainerManager.h"

#include <wx/file.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/wx.h>
#include <wx/msgdlg.h>
#include <wx/log.h>

#include <sddl.h>
#include <aclapi.h>
#include <fstream>
#include <iostream>
#include <string>

#ifdef __WINDOWS__
    #include <windows.h>
#endif

wxBEGIN_EVENT_TABLE(WindowsCmdPanel, wxPanel)
    EVT_SIZE(WindowsCmdPanel::OnSize)
wxEND_EVENT_TABLE()

WindowsCmdPanel::WindowsCmdPanel(wxWindow* parent)
    : wxPanel(parent), m_hwndCmd(nullptr), m_initTimer(nullptr), m_initStep(0)
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
    wxLogMessage("WindowsCmdPanel destructor called");
    if (m_hwndCmd) {
        wxLogMessage("Closing CMD window");
        ::SendMessage(m_hwndCmd, WM_CLOSE, 0, 0);
    }

    wxString containerId = ContainerManager::Get().GetCurrentContainerId();
    if (!containerId.IsEmpty()) {
        wxLogMessage("Cleaning up container: %s", containerId);
        ContainerManager::Get().CleanupContainer(containerId);
    }
#endif

    CleanupTimer();
}

void WindowsCmdPanel::SetISOPath(const wxString& path)
{
    wxLogMessage("Setting ISO path: %s", path);
    m_isoPath = path;
    if (!m_isoPath.IsEmpty()) {
        CreateCmdWindow();
    }
}

void WindowsCmdPanel::OnSize(wxSizeEvent& event)
{
#ifdef __WINDOWS__
    if (m_hwndCmd) {
        wxSize size = GetClientSize();
        wxLogMessage("Resizing CMD window to %dx%d", size.GetWidth(), size.GetHeight());
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

void WindowsCmdPanel::ContinueInitialization()
{
#ifdef __WINDOWS__
    wxLogMessage("Continuing initialization step %d", m_initStep);
    switch (m_initStep) {
        case 0: { // Create container and embed immediately
            wxLogMessage("Step 0: Creating Docker container");
            std::wstring baseDir = L"I:\\Files\\Desktop\\LinuxGUI\\build";
            std::wstring cmdLine = L"cmd.exe /K \"cd /d " + baseDir +
                                   L" && docker run -it --privileged --name my_unique_container ubuntu:latest bash\"";
            wxLogMessage("Command to create container: %s", cmdLine);

            STARTUPINFOW si;
            PROCESS_INFORMATION pi;
            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            si.dwFlags = STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_HIDE;  // Hide the window initially
            ZeroMemory(&pi, sizeof(pi));

            if (CreateProcessW(NULL, const_cast<wchar_t*>(cmdLine.c_str()),
                              NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi)) {
                wxLogMessage("Container process created successfully");
                wxLogMessage("Process ID: %d", pi.dwProcessId);
                wxLogMessage("Thread ID: %d", pi.dwThreadId);
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);

                // Wait briefly for the window to be created
                Sleep(500);

                // Find and embed the CMD window immediately
                m_hwndCmd = FindWindowExW(NULL, NULL, L"ConsoleWindowClass", NULL);
                if (m_hwndCmd) {
                    wxLogMessage("Found CMD window, embedding immediately");
                    LONG style = GetWindowLong(m_hwndCmd, GWL_STYLE);
                    style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX |
                               WS_MAXIMIZEBOX | WS_SYSMENU);
                    SetWindowLong(m_hwndCmd, GWL_STYLE, style);

                    LONG exStyle = GetWindowLong(m_hwndCmd, GWL_EXSTYLE);
                    exStyle &= ~(WS_EX_CLIENTEDGE | WS_EX_WINDOWEDGE);
                    SetWindowLong(m_hwndCmd, GWL_EXSTYLE, exStyle);

                    ::SetParent(m_hwndCmd, (HWND)GetHWND());
                    ::ShowWindow(m_hwndCmd, SW_SHOW);  // Show the window after embedding
                    ::SetWindowPos(m_hwndCmd, NULL, 0, 0,
                                   GetClientSize().GetWidth(), GetClientSize().GetHeight(),
                                   SWP_FRAMECHANGED);
                    ::SetForegroundWindow(m_hwndCmd);
                    ::SetFocus(m_hwndCmd);
                    wxLogMessage("CMD window embedded successfully");
                } else {
                    wxLogError("Failed to find CMD window for immediate embedding");
                }

                m_initTimer->StartOnce(3000); // Wait 3 seconds before next step
                m_initStep++;
            } else {
                wxLogError("Failed to create container process. Error code: %d", GetLastError());
                CleanupTimer();
            }
            break;
        }

        case 1: { // Get container ID
            wxLogMessage("Step 1: Getting container ID");
            std::wstring baseDir = L"I:\\Files\\Desktop\\LinuxGUI\\build";
            std::wstring captureCmd = L"docker ps -aqf name=my_unique_container > \"" +
                                      baseDir + L"\\container_id.txt\"";
            wxLogMessage("Running command to capture container ID: %s", captureCmd);

            STARTUPINFOW siHidden = { sizeof(STARTUPINFOW) };
            PROCESS_INFORMATION piHidden;
            siHidden.dwFlags = STARTF_USESHOWWINDOW;
            siHidden.wShowWindow = SW_HIDE; // Hide the window for this process

            std::wstring fullCmd = L"cmd.exe /C " + captureCmd;
            if (CreateProcessW(NULL, const_cast<LPWSTR>(fullCmd.c_str()),
                              NULL, NULL, FALSE, CREATE_NO_WINDOW,
                              NULL, NULL, &siHidden, &piHidden)) {
                wxLogMessage("Process created successfully for capturing container ID");
                wxLogMessage("Process ID: %d", piHidden.dwProcessId);
                wxLogMessage("Thread ID: %d", piHidden.dwThreadId);

                // Wait for the command to complete
                WaitForSingleObject(piHidden.hProcess, INFINITE);
                CloseHandle(piHidden.hProcess);
                CloseHandle(piHidden.hThread);

                std::string containerId;
                std::ifstream file((baseDir + L"\\container_id.txt").c_str());
                if (file.is_open()) {
                    std::getline(file, containerId);
                    file.close();
                    containerId.erase(0, containerId.find_first_not_of(" \n\r\t"));
                    containerId.erase(containerId.find_last_not_of(" \n\r\t") + 1);

                    if (!containerId.empty()) {
                        m_containerId = containerId;
                        wxLogMessage("Container ID captured: %s", containerId);
                        m_initTimer->StartOnce(100);
                        m_initStep++;
                    } else {
                        wxLogError("Container ID is empty");
                        CleanupTimer();
                    }
                } else {
                    wxLogError("Failed to read container ID file");
                    CleanupTimer();
                }
            } else {
                wxLogError("Failed to create process for capturing container ID. Error code: %d", GetLastError());
                CleanupTimer();
            }
            break;
        }

        case 2: { // Copy scripts and set permissions
            wxLogMessage("Step 2: Copying scripts and setting permissions");
            for (const auto& script : {"setup_output.sh", "setup_chroot.sh", "create_iso.sh"}) {
                wxString cmd = wxString::Format("docker cp \"I:\\Files\\Desktop\\LinuxGUI\\build\\%s\" %s:/%s",
                                                script, m_containerId, script);
                wxLogMessage("Copying script: %s", cmd);
                wxArrayString output, errors;
                if (wxExecute(cmd, output, errors, wxEXEC_SYNC | wxEXEC_HIDE_CONSOLE) == 0) {
                    wxLogMessage("Script copied successfully: %s", script);
                    wxLogMessage("Command output:");
                    for (const auto& line : output) {
                        wxLogMessage(" - %s", line);
                    }
                } else {
                    wxLogError("Failed to copy script: %s", script);
                    wxLogError("Command errors:");
                    for (const auto& error : errors) {
                        wxLogError(" - %s", error);
                    }
                }
            }

            wxString chmodCmd = wxString::Format(
                "docker exec %s chmod +x /setup_output.sh /setup_chroot.sh /create_iso.sh",
                m_containerId);
            wxLogMessage("Setting permissions: %s", chmodCmd);
            wxArrayString chmodOutput, chmodErrors;
            if (wxExecute(chmodCmd, chmodOutput, chmodErrors, wxEXEC_SYNC | wxEXEC_HIDE_CONSOLE) == 0) {
                wxLogMessage("Permissions set successfully");
                wxLogMessage("Command output:");
                for (const auto& line : chmodOutput) {
                    wxLogMessage(" - %s", line);
                }
            } else {
                wxLogError("Failed to set permissions");
                wxLogError("Command errors:");
                for (const auto& error : chmodErrors) {
                    wxLogError(" - %s", error);
                }
            }

            m_initTimer->StartOnce(100);
            m_initStep++;
            break;
        }

        case 3: { // Copy ISO and run setup scripts if path is set
            wxLogMessage("Step 3: ISO setup and script execution");
            if (!m_isoPath.IsEmpty()) {
                wxLogMessage("ISO path: %s", m_isoPath);

                // Create the ISO directory in the container
                wxString mkdirCmd = wxString::Format("docker exec %s mkdir -p /mnt/iso", m_containerId);
                wxLogMessage("Creating ISO directory in container with command: %s", mkdirCmd);

                wxArrayString mkdirOutput, mkdirErrors;
                if (wxExecute(mkdirCmd, mkdirOutput, mkdirErrors, wxEXEC_SYNC | wxEXEC_HIDE_CONSOLE) == 0) {
                    wxLogMessage("ISO directory created successfully in container.");
                    wxLogMessage("Command output:");
                    for (const auto& line : mkdirOutput) {
                        wxLogMessage(" - %s", line);
                    }
                } else {
                    wxLogError("Failed to create ISO directory in container.");
                    wxLogError("Command errors:");
                    for (const auto& error : mkdirErrors) {
                        wxLogError(" - %s", error);
                    }
                    CleanupTimer();
                    return;
                }

                // Copy the ISO file into the container
                wxString copyIsoCmd = wxString::Format("docker cp \"%s\" %s:/mnt/iso/base.iso",
                                                       m_isoPath, m_containerId);
                wxLogMessage("Copying ISO file into container with command: %s", copyIsoCmd);

                wxArrayString output, errors;
                if (wxExecute(copyIsoCmd, output, errors, wxEXEC_SYNC | wxEXEC_HIDE_CONSOLE) == 0) {
                    wxLogMessage("ISO file copied successfully into container.");
                    wxLogMessage("Command output:");
                    for (const auto& line : output) {
                        wxLogMessage(" - %s", line);
                    }

                    // Run setup scripts after ISO copy
                    wxString setupOutputCmd = wxString::Format("docker exec %s /setup_output.sh", m_containerId);
                    wxString setupChrootCmd = wxString::Format("docker exec %s /setup_chroot.sh", m_containerId);

                    wxLogMessage("Running setup_output.sh script...");
                    wxArrayString setupOutputOutput, setupOutputErrors;
                    if (wxExecute(setupOutputCmd, setupOutputOutput, setupOutputErrors, wxEXEC_SYNC | wxEXEC_HIDE_CONSOLE) == 0) {
                        wxLogMessage("setup_output.sh executed successfully.");
                        wxLogMessage("Command output:");
                        for (const auto& line : setupOutputOutput) {
                            wxLogMessage(" - %s", line);
                        }
                    } else {
                        wxLogError("Failed to execute setup_output.sh.");
                        wxLogError("Command errors:");
                        for (const auto& error : setupOutputErrors) {
                            wxLogError(" - %s", error);
                        }
                    }

                    wxLogMessage("Running setup_chroot.sh script...");
                    wxArrayString setupChrootOutput, setupChrootErrors;
                    if (wxExecute(setupChrootCmd, setupChrootOutput, setupChrootErrors, wxEXEC_SYNC | wxEXEC_HIDE_CONSOLE) == 0) {
                        wxLogMessage("setup_chroot.sh executed successfully.");
                        wxLogMessage("Command output:");
                        for (const auto& line : setupChrootOutput) {
                            wxLogMessage(" - %s", line);
                        }
                    } else {
                        wxLogError("Failed to execute setup_chroot.sh.");
                        wxLogError("Command errors:");
                        for (const auto& error : setupChrootErrors) {
                            wxLogError(" - %s", error);
                        }
                    }
                } else {
                    wxLogError("Failed to copy ISO file into container.");
                    wxLogError("Command errors:");
                    for (const auto& error : errors) {
                        wxLogError(" - %s", error);
                    }
                    CleanupTimer();
                    return;
                }
            } else {
                wxLogMessage("No ISO path set, skipping ISO setup.");
            }

            m_initTimer->StartOnce(200);
            m_initStep++;
            break;
        }

        case 4: { // Final cleanup
            wxLogMessage("Step 4: Initialization complete");
            CleanupTimer();
            break;
        }
    }
#endif
}

void WindowsCmdPanel::CleanupTimer()
{
    if (m_initTimer) {
        wxLogMessage("Cleaning up initialization timer");
        m_initTimer->Stop();
        delete m_initTimer;
        m_initTimer = nullptr;
    }
}

bool WindowsCmdPanel::ExecuteHiddenCommand(const wxString& cmd, wxArrayString* output)
{
#ifdef __WINDOWS__
    STARTUPINFOW si = { sizeof(STARTUPINFOW) };
    PROCESS_INFORMATION pi;

    // Create pipes for standard handles
    HANDLE hStdInRead, hStdInWrite;
    HANDLE hStdOutRead, hStdOutWrite;
    HANDLE hStdErrRead, hStdErrWrite;

    if (!CreatePipe(&hStdInRead, &hStdInWrite, NULL, 0)) return false;
    if (!CreatePipe(&hStdOutRead, &hStdOutWrite, NULL, 0)) return false;
    if (!CreatePipe(&hStdErrRead, &hStdErrWrite, NULL, 0)) return false;

    // Set up STARTUPINFOW
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdInput = hStdInRead;
    si.hStdOutput = hStdOutWrite;
    si.hStdError = hStdErrWrite;
    si.wShowWindow = SW_HIDE;

    // Prepare the command line
    std::wstring fullCmd = L"cmd.exe /C " + std::wstring(cmd.wc_str());

    BOOL result = CreateProcessW(NULL, const_cast<wchar_t*>(fullCmd.c_str()),
                                 NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);

    // Close the handles to the pipes
    CloseHandle(hStdInRead);
    CloseHandle(hStdInWrite);
    CloseHandle(hStdOutRead);
    CloseHandle(hStdOutWrite);
    CloseHandle(hStdErrRead);
    CloseHandle(hStdErrWrite);

    if (!result) {
        wxLogError("Failed to execute command: %s", cmd);
        return false;
    }

    // Wait for the process to complete
    WaitForSingleObject(pi.hProcess, INFINITE);

    // Get exit code
    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    // Cleanup
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return (exitCode == 0);
#else
    return false;
#endif
}

bool WindowsCmdPanel::ExecuteDockerCommand(const wxString& cmd, wxArrayString* output)
{
    wxString dockerCmd = wxString::Format("docker %s", cmd);
    return ExecuteHiddenCommand(dockerCmd, output);
}