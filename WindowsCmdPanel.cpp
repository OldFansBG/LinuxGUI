#include "WindowsCmdPanel.h"
#include "ContainerManager.h"

#include <wx/file.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/wx.h>
#include <wx/msgdlg.h>
#include <wx/log.h>
#include <memory>
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

bool WindowsCmdPanel::CheckISOExistsInContainer(const wxString& containerId) {
    wxString checkCmd = wxString::Format("docker exec %s [ -f /base.iso ] && echo 'exists'", containerId);
    wxArrayString output, errors;
    if (wxExecute(checkCmd, output, errors, wxEXEC_SYNC | wxEXEC_HIDE_CONSOLE) == 0) {
        for (const auto& line : output) {
            if (line.Contains("exists")) {
                wxLogMessage("base.iso found in container.");
                return true;
            }
        }
    }
    wxLogError("base.iso not found in container.");
    return false;
}

void WindowsCmdPanel::ContinueInitialization()
{
#ifdef __WINDOWS__
    wxLogMessage("Continuing initialization step %d", m_initStep);
    switch (m_initStep) {
        case 0: { // Step 0: Create Docker container with sleep infinity
            wxLogMessage("Step 0: Creating Docker container");

            // Command to start the container with sleep infinity
            std::wstring cmdLine = L"docker run -it --privileged --name my_unique_container ubuntu:latest sleep infinity";

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

                // Wait briefly for the container to start
                Sleep(2000);

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

        case 1: { // Step 1: Get container ID
            wxLogMessage("Step 1: Getting container ID");
            std::wstring baseDir = L"I:\\Files\\Desktop\\LinuxGUI\\build";
            std::wstring captureCmd = L"docker ps -aqf name=my_unique_container > \"" + baseDir + L"\\container_id.txt\"";
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

        case 2: { // Step 2: Copy scripts and set permissions
            wxLogMessage("Step 2: Copying scripts and setting permissions");

            // List of scripts to copy
            const wxString scripts[] = {"setup_output.sh", "setup_chroot.sh", "create_iso.sh"};

            for (const auto& script : scripts) {
                wxString scriptPath = wxString::Format("I:\\Files\\Desktop\\LinuxGUI\\build\\%s", script);
                wxString copyCmd = wxString::Format("docker cp \"%s\" %s:/%s", scriptPath, m_containerId, script);

                wxLogMessage("Copying script: %s", copyCmd);

                wxArrayString output, errors;
                if (wxExecute(copyCmd, output, errors, wxEXEC_SYNC | wxEXEC_HIDE_CONSOLE) != 0) {
                    wxLogError("Failed to copy script: %s", script);
                    wxLogError("Command errors:");
                    for (const auto& error : errors) {
                        wxLogError(" - %s", error);
                    }
                    wxMessageBox(wxString::Format("Failed to copy script: %s", script), "Error", wxOK | wxICON_ERROR);
                    CleanupTimer();
                    return;
                }

                wxLogMessage("Script copied successfully: %s", script);
            }

            // Set execute permissions on the scripts
            wxString chmodCmd = wxString::Format("docker exec %s chmod +x /setup_output.sh /setup_chroot.sh /create_iso.sh", m_containerId);
            wxArrayString chmodOutput, chmodErrors;
            if (wxExecute(chmodCmd, chmodOutput, chmodErrors, wxEXEC_SYNC | wxEXEC_HIDE_CONSOLE) != 0) {
                wxLogError("Failed to set permissions on scripts.");
                wxLogError("Command errors:");
                for (const auto& error : chmodErrors) {
                    wxLogError(" - %s", error);
                }
                wxMessageBox("Failed to set permissions on scripts.", "Error", wxOK | wxICON_ERROR);
                CleanupTimer();
                return;
            }

            wxLogMessage("Permissions set successfully on all scripts.");

            // Proceed to the next step
            m_initTimer->StartOnce(200);
            m_initStep++;
            break;
        }

        case 3: { // Step 3: Copy ISO and verify
            wxLogMessage("Step 3: Copying ISO and verifying");
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
                wxString copyIsoCmd = wxString::Format("docker cp \"%s\" %s:/base.iso", m_isoPath, m_containerId);
                wxLogMessage("Copying ISO file into container with command: %s", copyIsoCmd);

                wxArrayString output, errors;
                if (wxExecute(copyIsoCmd, output, errors, wxEXEC_SYNC | wxEXEC_HIDE_CONSOLE) == 0) {
                    wxLogMessage("ISO file copied successfully into container.");
                    wxLogMessage("Command output:");
                    for (const auto& line : output) {
                        wxLogMessage(" - %s", line);
                    }

                    // Verify the ISO file is fully copied
                    wxString checkCmd = wxString::Format("docker exec %s stat -c %%s /base.iso", m_containerId);
                    wxArrayString checkOutput, checkErrors;
                    if (wxExecute(checkCmd, checkOutput, checkErrors, wxEXEC_SYNC | wxEXEC_HIDE_CONSOLE) == 0) {
                        wxString isoSize = checkOutput[0];
                        wxLogMessage("ISO file size: %s", isoSize);
                    } else {
                        wxLogError("Failed to verify ISO file size.");
                        wxLogError("Command errors:");
                        for (const auto& error : checkErrors) {
                            wxLogError(" - %s", error);
                        }
                        CleanupTimer();
                        return;
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

            // Create the /output directory in the container
            wxString mkdirOutputCmd = wxString::Format("docker exec %s mkdir -p /output", m_containerId);
            wxLogMessage("Creating /output directory in container with command: %s", mkdirOutputCmd);

            wxArrayString mkdirOutputResult, mkdirOutputErrors;
            if (wxExecute(mkdirOutputCmd, mkdirOutputResult, mkdirOutputErrors, wxEXEC_SYNC | wxEXEC_HIDE_CONSOLE) == 0) {
                wxLogMessage("/output directory created successfully in container.");
                wxLogMessage("Command output:");
                for (const auto& line : mkdirOutputResult) {
                    wxLogMessage(" - %s", line);
                }
            } else {
                wxLogError("Failed to create /output directory in container.");
                wxLogError("Command errors:");
                for (const auto& error : mkdirOutputErrors) {
                    wxLogError(" - %s", error);
                }
                CleanupTimer();
                return;
            }

            m_initTimer->StartOnce(200);
            m_initStep++;
            break;
        }

        case 4: { // Step 4: Execute setup_output.sh
            wxLogMessage("Step 4: Executing setup_output.sh");
            wxString setupOutputCmd = wxString::Format("docker exec %s /setup_output.sh", m_containerId);
            wxArrayString output, errors;
            if (wxExecute(setupOutputCmd, output, errors, wxEXEC_SYNC | wxEXEC_HIDE_CONSOLE) == 0) {
                wxLogMessage("setup_output.sh executed successfully.");
                wxLogMessage("Command output:");
                for (const auto& line : output) {
                    wxLogMessage(" - %s", line);
                }
            } else {
                wxLogError("Failed to execute setup_output.sh.");
                wxLogError("Command errors:");
                for (const auto& error : errors) {
                    wxLogError(" - %s", error);
                }
                CleanupTimer();
                return;
            }

            m_initTimer->StartOnce(200);
            m_initStep++; // Proceed to Step 6
            break;
        }

        case 5: { // Step 5: (Removed)
            // Skip this step and proceed to Step 6
            m_initTimer->StartOnce(200);
            m_initStep++;
            break;
        }

        case 6: { // Step 6: Execute setup_chroot.sh and enter chroot
            wxLogMessage("Step 6: Executing setup_chroot.sh and entering chroot");

            // Detach the existing CMD window (if any)
            if (m_hwndCmd) {
                ::SendMessage(m_hwndCmd, WM_CLOSE, 0, 0); // Close the existing CMD window
                m_hwndCmd = nullptr; // Reset the handle
            }

            // Command to run docker exec -it, execute setup_chroot.sh, and enter chroot
            std::wstring attachCmd = wxString::Format(
                L"cmd.exe /K docker exec -it %s /bin/bash -c \"/setup_chroot.sh 'echo $$'\"",
                m_containerId
            ).ToStdWstring();

            STARTUPINFOW si;
            PROCESS_INFORMATION pi;
            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            si.dwFlags = STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_HIDE; // Hide the window initially
            ZeroMemory(&pi, sizeof(pi));

            if (CreateProcessW(NULL, const_cast<wchar_t*>(attachCmd.c_str()),
                            NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi)) {
                wxLogMessage("New CMD window created successfully for docker exec -it");
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);

                // Wait for the new CMD window to be created
                Sleep(2000);

                // Find and embed the new CMD window
                m_hwndCmd = FindWindowExW(NULL, NULL, L"ConsoleWindowClass", NULL);
                if (m_hwndCmd) {
                    wxLogMessage("Found new CMD window, embedding immediately");

                    // Remove window decorations
                    LONG style = GetWindowLong(m_hwndCmd, GWL_STYLE);
                    style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX |
                            WS_MAXIMIZEBOX | WS_SYSMENU);
                    SetWindowLong(m_hwndCmd, GWL_STYLE, style);

                    LONG exStyle = GetWindowLong(m_hwndCmd, GWL_EXSTYLE);
                    exStyle &= ~(WS_EX_CLIENTEDGE | WS_EX_WINDOWEDGE);
                    SetWindowLong(m_hwndCmd, GWL_EXSTYLE, exStyle);

                    // Embed the window into the application
                    ::SetParent(m_hwndCmd, (HWND)GetHWND());
                    ::ShowWindow(m_hwndCmd, SW_SHOW);  // Show the window after embedding
                    ::SetWindowPos(m_hwndCmd, NULL, 0, 0,
                                GetClientSize().GetWidth(), GetClientSize().GetHeight(),
                                SWP_FRAMECHANGED);
                    ::SetForegroundWindow(m_hwndCmd);
                    ::SetFocus(m_hwndCmd);
                    wxLogMessage("New CMD window embedded successfully");
                } else {
                    wxLogError("Failed to find new CMD window for embedding");
                }
            } else {
                wxLogError("Failed to create new CMD window for docker exec -it. Error code: %d", GetLastError());
            }

            // Clean up the timer and increment the step
            CleanupTimer();
            m_initStep++; // Move to the next step
            break;
        }

       case 7: { // Step 7: Execute create_iso.sh
            wxLogMessage("Step 7: Executing create_iso.sh");

            // Detach the existing CMD window (if any)
            if (m_hwndCmd) {
                ::SendMessage(m_hwndCmd, WM_CLOSE, 0, 0); // Close the existing CMD window
                m_hwndCmd = nullptr; // Reset the handle
            }

            // Command to run docker exec and execute create_iso.sh
            std::wstring createIsoCmd = wxString::Format(
                L"docker exec %s /bin/bash -c \"/create_iso.sh\"",
                m_containerId
            ).ToStdWstring();

            // Execute the command and check the exit code
            int exitCode = wxExecute(createIsoCmd, wxEXEC_SYNC | wxEXEC_HIDE_CONSOLE);
            if (exitCode == 0) {
                wxLogMessage("create_iso.sh has completed successfully.");
                m_initStep++; // Proceed to Step 8
                ContinueInitialization(); // Directly call Step 8
            } else {
                wxLogError("create_iso.sh failed to complete. Exit code: %d", exitCode);
                CleanupTimer();
                return;
            }
            break;
        }

        case 8: { // Step 8: Copy the ISO file from the container to the host
            wxLogMessage("Step 8: Copying ISO file from container to host");

            // Step 8.1: Verify the container is still running
            wxString checkContainerCmd = wxString::Format("docker ps -q --filter \"id=%s\"", m_containerId);
            wxArrayString containerOutput, containerErrors;
            wxLogMessage("Checking if container is running: %s", checkContainerCmd);

            if (wxExecute(checkContainerCmd, containerOutput, containerErrors, wxEXEC_SYNC | wxEXEC_HIDE_CONSOLE) != 0 || containerOutput.IsEmpty()) {
                wxLogError("Container is not running.");
                wxMessageBox("Container is not running.", "Error", wxOK | wxICON_ERROR);
                CleanupTimer();
                return;
            }
            wxLogMessage("Container is running.");

            // Step 8.2: Verify the ISO file exists in the container
            wxString checkIsoCmd = wxString::Format("docker exec %s ls -l /root/custom_iso/custom_linuxmint.iso", m_containerId);
            wxArrayString checkIsoOutput, checkIsoErrors;
            wxLogMessage("Checking if ISO exists in container: %s", checkIsoCmd);

            if (wxExecute(checkIsoCmd, checkIsoOutput, checkIsoErrors, wxEXEC_SYNC | wxEXEC_HIDE_CONSOLE) != 0) {
                wxLogError("ISO file not found in container.");
                wxMessageBox("ISO file not found in container.", "Error", wxOK | wxICON_ERROR);
                CleanupTimer();
                return;
            }
            wxLogMessage("ISO file found in container.");

            // Step 8.3: Define the output directory on the host
            wxString isoDir = "I:\\Files\\Desktop\\LinuxGUI\\iso";
            wxLogMessage("Checking if output directory exists: %s", isoDir);

            // Create the output directory if it doesn't exist
            if (!wxDirExists(isoDir)) {
                wxLogMessage("Output directory does not exist. Creating it.");
                if (!wxFileName::Mkdir(isoDir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL)) {
                    wxLogError("Failed to create output directory.");
                    wxMessageBox("Failed to create local output directory. Please check permissions.", "Error", wxOK | wxICON_ERROR);
                    CleanupTimer();
                    return;
                }
                wxLogMessage("Output directory created successfully.");
            }

            // Step 8.4: Define the final ISO path on the host
            wxString finalIsoPath = isoDir + "\\custom_linuxmint.iso";
            wxLogMessage("Copying ISO from container to: %s", finalIsoPath);

            // Step 8.5: Command to copy the ISO from the container to the host
            wxString copyIsoCmd = wxString::Format("docker cp %s:/root/custom_iso/custom_linuxmint.iso \"%s\"", m_containerId, finalIsoPath);
            wxArrayString copyOutput, copyErrors;
            wxLogMessage("Executing command: %s", copyIsoCmd);

            // Execute the copy command
            if (wxExecute(copyIsoCmd, copyOutput, copyErrors, wxEXEC_SYNC | wxEXEC_HIDE_CONSOLE) != 0) {
                wxLogError("Failed to copy ISO. Errors:");
                for (const auto& error : copyErrors) {
                    wxLogError(" - %s", error);
                }
                wxMessageBox("Failed to copy ISO.", "Error", wxOK | wxICON_ERROR);
                CleanupTimer();
                return;
            }

            wxLogMessage("Command output:");
            for (const auto& line : copyOutput) {
                wxLogMessage(" - %s", line);
            }

            // Step 8.6: Verify the ISO file exists on the host
            wxLogMessage("Verifying ISO file: %s", finalIsoPath);
            if (!wxFileExists(finalIsoPath)) {
                wxLogError("ISO file does not exist.");
                wxMessageBox("Failed to verify ISO file. Please check the logs.", "Error", wxOK | wxICON_ERROR);
                CleanupTimer();
                return;
            }

            // Step 8.7: Verify the ISO file is not empty
            wxULongLong isoSize = wxFileName::GetSize(finalIsoPath);
            if (isoSize == 0) {
                wxLogError("ISO file is empty.");
                wxMessageBox("ISO file is empty.", "Error", wxOK | wxICON_ERROR);
                CleanupTimer();
                return;
            }

            wxLogMessage("ISO file verified: %s (Size: %s)", finalIsoPath, GetFileSizeString(finalIsoPath));

            // Step 8.8: Show completion dialog
            wxLogMessage("ISO creation process completed successfully.");
            ShowCompletionDialog(finalIsoPath); // Call the completion dialog here

            CleanupTimer();
            break;
        }
    }
#endif
}

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
bool WindowsCmdPanel::WaitForISOToBeCopied(const wxString& containerId) {
    wxString checkCmd = wxString::Format("docker exec %s ls -l /base.iso", containerId);
    wxArrayString output, errors;
    int attempts = 0;
    const int maxAttempts = 30; // Wait for up to 30 seconds
    const int delay = 1000; // 1 second delay between attempts

    while (attempts < maxAttempts) {
        if (wxExecute(checkCmd, output, errors, wxEXEC_SYNC | wxEXEC_HIDE_CONSOLE) == 0) {
            for (const auto& line : output) {
                if (line.Contains("base.iso")) {
                    wxLogMessage("base.iso found in container: %s", line);
                    return true;
                }
            }
        }
        wxLogMessage("Waiting for base.iso to be copied... Attempt %d/%d", attempts + 1, maxAttempts);
        wxMilliSleep(delay);
        attempts++;
    }

    wxLogError("base.iso not found in container after %d attempts.", maxAttempts);
    wxLogError("Command output:");
    for (const auto& line : output) {
        wxLogError(" - %s", line);
    }
    wxLogError("Command errors:");
    for (const auto& error : errors) {
        wxLogError(" - %s", error);
    }
    return false;
}

void  WindowsCmdPanel :: CleanupTimer ( ) 
{ 
    if  ( m_initTimer )  { 
        wxLogMessage ( "Cleaning up initialization timer" ) ; 
        m_initTimer -> Stop ( ) ; 
        delete  m_initTimer ; 
        m_initTimer  =  nullptr ; 
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