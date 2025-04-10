#include "SecondWindow.h"
#include "DesktopTab.h"
#include "MongoDBPanel.h" // <<< Include the separated header
#include "WindowIDs.h"    // <<< Ensure this includes ID_MONGODB_PANEL_CLOSE
#include "CustomEvents.h" // <<< Include for FILE_COPY_COMPLETE_EVENT etc.
#include <wx/utils.h>
#include <wx/statline.h>
#include <wx/process.h>
#include <wx/dcbuffer.h>
#include <wx/filename.h>
#include <wx/file.h>
#include <wx/arrstr.h>
#include <wx/log.h>
#include <cmath>         // For M_PI, fmod, cos, sin in OverlayFrame
#include <chrono>        // For std::chrono (used indirectly)
#include <iomanip>       // For std::put_time (used indirectly)
#include <fstream>       // For std::ifstream (used indirectly)
#include <sstream>       // For std::stringstream (used indirectly)
#include <string>        // For std::string
#include <wx/graphics.h> // For wxGraphicsContext in OverlayFrame

// MongoDB includes needed by SecondWindow (minimal if panel handles logic)
#include <mongocxx/instance.hpp> // Still needed for static instance

#ifdef __WXMSW__
#include <windows.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
// Define if not present in your SDK version
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#endif

// Initialize static member
mongocxx::instance SecondWindow::m_mongoInstance{};

//---------------------------------------------------------------------
// PythonProcess class
class PythonProcess : public wxProcess
{
public:
    PythonProcess(SecondWindow *parent) : wxProcess(parent), m_parent(parent) {}

    virtual void OnTerminate(int pid, int status) override
    {
        if (m_parent)
        {
            m_parent->CloseOverlay();
            wxString containerIdPath = wxFileName(m_parent->GetProjectDir(), "container_id.txt").GetFullPath();
            wxFile file;
            if (file.Open(containerIdPath))
            {
                wxString containerId;
                file.ReadAll(&containerId);
                containerId.Trim();

                // Update FlatpakStore and SQLTab if they exist
                if (m_parent->GetFlatpakStore())
                {
                    m_parent->GetFlatpakStore()->SetContainerId(containerId);
                    wxLogDebug("Updated FlatpakStore container ID in SecondWindow: %s", containerId);
                }
                if (m_parent->GetSQLTab())
                {
                    m_parent->GetSQLTab()->SetContainerId(containerId);
                    wxLogDebug("Updated SQLTab container ID: %s", containerId);
                }

                // Detect GUI Environment
                wxString guiDetectCommand = wxString::Format("docker exec %s cat /root/custom_iso/detected_gui.txt", containerId);
                wxArrayString output, errors;
                int exitCode = wxExecute(guiDetectCommand, output, errors, wxEXEC_SYNC | wxEXEC_HIDE_CONSOLE);

                if (exitCode == 0 && !output.IsEmpty())
                {
                    wxString guiName = output[0];
                    guiName.Trim(true).Trim(false);
                    guiName.Replace("\n", ""); // Clean up potential newline

                    wxLogDebug("Detected GUI environment before event: %s", guiName);

                    // Save GUI name locally
                    wxString localGuiPath = wxFileName(m_parent->GetProjectDir(), "detected_gui.txt").GetFullPath();
                    wxFile guiFile;
                    if (guiFile.Create(localGuiPath, true) && guiFile.IsOpened())
                    {
                        guiFile.Write(guiName);
                        guiFile.Close();
                        wxLogDebug("GUI name saved to file: %s", localGuiPath);
                    }

                    // Notify DesktopTab
                    if (m_parent->GetDesktopTab())
                    {
                        wxCommandEvent *guiEvent = new wxCommandEvent(FILE_COPY_COMPLETE_EVENT);
                        guiEvent->SetString(guiName);
                        wxLogDebug("Posting event with GUI name: %s", guiName);
                        wxPostEvent(m_parent->GetDesktopTab(), *guiEvent);
                        delete guiEvent; // Clean up the event object
                    }
                }
                else
                {
                    wxLogError("Failed to detect GUI environment or no output. ExitCode: %d", exitCode);
                }
                // Execute initial command in terminal if applicable
                m_parent->ExecuteDockerCommand(containerId);
            }
            else
            {
                wxLogError("Could not open container_id.txt file: %s", containerIdPath);
            }
        }
    }

private:
    SecondWindow *m_parent;
};
//---------------------------------------------------------------------

//---------------------------------------------------------------------
// DockerExecProcess class
class DockerExecProcess : public wxProcess
{
public:
    DockerExecProcess(SecondWindow *parent) : wxProcess(parent), m_parent(parent) {}

    virtual void OnTerminate(int pid, int status) override
    {
        if (m_parent)
        {
            m_parent->CloseOverlay();
            if (status == 0)
            {
                wxString containerIdPath = wxFileName(m_parent->GetProjectDir(), "container_id.txt").GetFullPath();
                wxFile file;
                if (file.Open(containerIdPath))
                {
                    wxString containerId;
                    file.ReadAll(&containerId);
                    containerId.Trim();

                    wxString sourcePath = wxString::Format("%s:/root/custom_iso/custom_linuxmint.iso", containerId);
                    wxString destPath = m_parent->GetProjectDir();
                    wxString copyCommand = wxString::Format("docker cp \"%s\" \"%s\"", sourcePath, destPath);
                    int copyStatus = wxExecute(copyCommand, wxEXEC_SYNC | wxEXEC_HIDE_CONSOLE);

                    if (copyStatus == 0)
                    {
                        wxMessageBox("ISO created and copied to project directory successfully!", "Success", wxOK | wxICON_INFORMATION);
                    }
                    else
                    {
                        wxMessageBox("ISO creation succeeded but failed to copy to host.", "Error", wxICON_ERROR);
                    }
                }
                else
                {
                    wxLogError("Could not open container_id.txt during ISO copy phase.");
                    wxMessageBox("ISO creation succeeded but failed read container ID to copy.", "Error", wxICON_ERROR);
                }
            }
            else
            {
                wxLogError("ISO creation process failed with status: %d", status);
                wxMessageBox("ISO creation process failed!", "Error", wxICON_ERROR);
            }
            // Re-enable the 'Next' button regardless of success/failure
            wxButton *nextButton = wxDynamicCast(m_parent->FindWindow(ID_NEXT_BUTTON), wxButton);
            if (nextButton)
                nextButton->Enable();
        }
    }

private:
    SecondWindow *m_parent;
};
//---------------------------------------------------------------------

//---------------------------------------------------------------------
// OverlayFrame class
//---------------------------------------------------------------------
// OverlayFrame class (Copied from the OLD working version)
class OverlayFrame : public wxDialog
{
public:
    OverlayFrame(wxWindow *parent) : wxDialog(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxNO_BORDER | wxFRAME_FLOAT_ON_PARENT)
    {
        SetBackgroundColour(wxColour(0, 0, 0));
        SetTransparent(200);
        SetBackgroundStyle(wxBG_STYLE_PAINT);

        m_parentWindow = parent;
        UpdatePositionAndSize(); // Set initial position/size

        m_animationAngle = 0.0;

        Bind(wxEVT_PAINT, &OverlayFrame::OnPaint, this);
        Bind(wxEVT_TIMER, &OverlayFrame::OnTimer, this);

        // Bind parent move/size events to keep overlay aligned
        if (parent)
        {
            parent->Bind(wxEVT_MOVE, &OverlayFrame::OnParentMoveOrResize, this);
            parent->Bind(wxEVT_SIZE, &OverlayFrame::OnParentMoveOrResize, this);
        }

        m_timer.SetOwner(this); // Associate timer with this window
        m_timer.Start(30);      // Animation frame rate
        Show(true);
    }

    ~OverlayFrame()
    {
        // Unbind events when the overlay is destroyed
        if (m_parentWindow)
        {
            m_parentWindow->Unbind(wxEVT_MOVE, &OverlayFrame::OnParentMoveOrResize, this);
            m_parentWindow->Unbind(wxEVT_SIZE, &OverlayFrame::OnParentMoveOrResize, this);
        }
        m_timer.Stop(); // Stop the animation timer
    }

private:
    void UpdatePositionAndSize()
    {
        if (m_parentWindow)
        {
            wxRect clientRect = m_parentWindow->GetClientRect();
            // Ensure client rect is valid before proceeding (minor improvement added here)
            if (clientRect.GetWidth() > 0 && clientRect.GetHeight() > 0)
            {
                SetPosition(m_parentWindow->ClientToScreen(clientRect.GetTopLeft()));
                SetSize(clientRect.GetSize());
            }
            else
            {
                // Optionally hide or handle cases where parent size is zero/invalid
                Hide();
            }
        }
    }

    void OnParentMoveOrResize(wxEvent &event)
    {
        UpdatePositionAndSize();
        event.Skip(); // Allow other handlers to process (added from new version - good practice)
    }

    void OnPaint(wxPaintEvent &event)
    {
        wxAutoBufferedPaintDC dc(this); // Use buffered DC for flicker-free drawing
        dc.Clear();                     // Clear background

        wxSize size = GetClientSize();
        // Draw semi-transparent background
        dc.SetBrush(wxBrush(wxColour(0, 0, 0, 230))); // Adjust alpha for desired transparency
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawRectangle(0, 0, size.GetWidth(), size.GetHeight());

        // Draw the loading animation centered
        DrawLoadingAnimation(dc, size);
    }

    // *** Uses the OLD DrawLoadingAnimation logic ***
    void DrawLoadingAnimation(wxDC &dc, const wxSize &size)
    {
        wxPoint center(size.GetWidth() / 2, size.GetHeight() / 2);
        int radius = std::min(size.GetWidth(), size.GetHeight()) / 6; // Original radius calculation
        int numPoints = 8;
        int pointRadius = radius / 4; // Original point radius calculation

        // Use non-transparent pen and brush for solid dots
        dc.SetBrush(*wxWHITE_BRUSH);
        dc.SetPen(*wxWHITE_PEN); // Original pen

        for (int i = 0; i < numPoints; ++i)
        {
            double angle = m_animationAngle + (2.0 * M_PI * i / numPoints);
            int x = center.x + static_cast<int>(radius * cos(angle));
            int y = center.y + static_cast<int>(radius * sin(angle));
            dc.DrawCircle(x, y, pointRadius); // Original drawing call
        }
    }

    // *** Uses the OLD OnTimer logic ***
    void OnTimer(wxTimerEvent &event)
    {
        m_animationAngle += 0.1; // Original speed
        if (m_animationAngle > 2.0 * M_PI)
            m_animationAngle -= 2.0 * M_PI;
        Refresh(); // Original Refresh call
    }

    double m_animationAngle;
    wxTimer m_timer;
    wxWindow *m_parentWindow; // Pointer to the window this overlay covers
};
//---------------------------------------------------------------------

// Event table for SecondWindow
wxBEGIN_EVENT_TABLE(SecondWindow, wxFrame)
    EVT_CLOSE(SecondWindow::OnClose)
        EVT_TIMER(wxID_ANY, SecondWindow::OnCloseTimer) // Catch any timer event for the close timer
    EVT_BUTTON(ID_TERMINAL_TAB, SecondWindow::OnTabChanged)
        EVT_BUTTON(ID_SQL_TAB, SecondWindow::OnTabChanged)
            EVT_BUTTON(ID_MONGODB_BUTTON, SecondWindow::OnMongoDBButton)  // Handles click on main MongoDB button
    EVT_BUTTON(ID_MONGODB_PANEL_CLOSE, SecondWindow::OnMongoDBPanelClose) // Handles close request *from* the panel
    EVT_BUTTON(ID_NEXT_BUTTON, SecondWindow::OnNext)
        wxEND_EVENT_TABLE()

    //---------------------------------------------------------------------
    // ContainerCleanupThread class
    class ContainerCleanupThread : public wxThread
{
public:
    ContainerCleanupThread(const wxString &containerId)
        : wxThread(wxTHREAD_DETACHED), m_containerId(containerId) {}

protected:
    virtual ExitCode Entry() override
    {
        wxLogDebug("ContainerCleanupThread::Entry - Starting cleanup thread for container: %s", m_containerId);
        if (m_containerId.IsEmpty())
        {
            wxLogDebug("ContainerCleanupThread::Entry - Container ID is empty, exiting thread.");
            return (ExitCode)0;
        }
        // Use ContainerManager to perform cleanup
        ContainerManager::Get().CleanupContainer(m_containerId);
        wxLogDebug("ContainerCleanupThread::Entry - Cleanup thread finished for container: %s", m_containerId);
        return (ExitCode)0;
    }

private:
    wxString m_containerId;
};
//---------------------------------------------------------------------

// --- SecondWindow Implementation ---

SecondWindow::SecondWindow(wxWindow *parent,
                           const wxString &title,
                           const wxString &isoPath,
                           const wxString &projectDir,
                           DesktopTab *desktopTab) // Keep DesktopTab parameter
    : wxFrame(parent, wxID_ANY, title,
              wxDefaultPosition, wxSize(800, 650), // Adjusted default size
              wxDEFAULT_FRAME_STYLE | wxSYSTEM_MENU | wxCAPTION |
                  wxCLOSE_BOX | wxCLIP_CHILDREN | wxMINIMIZE_BOX |
                  wxMAXIMIZE_BOX | wxRESIZE_BORDER),
      m_isoPath(isoPath),
      m_projectDir(projectDir),
      m_threadRunning(false),
      m_overlay(nullptr),
      m_desktopTab(desktopTab), // Store the passed DesktopTab pointer
      m_terminalTab(nullptr),   // Initialize pointers
      m_sqlTab(nullptr),
      m_terminalPanel(nullptr),
      m_flatpakStore(nullptr), // Initialize FlatpakStore pointer
      m_mongoPanel(nullptr),
      m_cleanupThread(nullptr),
      m_isClosing(false),
      m_closeTimer(nullptr),
      m_lastTab(nullptr)
#ifdef __WXMSW__
      ,
      m_winTerminalManager(nullptr) // Initialize WinTerminalManager pointer
#endif
{
    wxLogDebug("SecondWindow constructor: desktopTab pointer = %p", m_desktopTab);

#ifdef __WXMSW__
    // Apply dark mode hints for Windows title bar/borders if applicable
    BOOL value = TRUE;
    ::DwmSetWindowAttribute(GetHandle(), DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
#endif

    // Attempt to get container ID immediately if available (might be empty initially)
    m_containerId = ContainerManager::Get().GetCurrentContainerId();
    wxLogDebug("SecondWindow constructor initial container ID: %s", m_containerId);

    CreateControls(); // Create all UI elements
    Centre();
    SetBackgroundColour(wxColour(40, 44, 52)); // Set a base background color
    m_lastTab = m_terminalTab;                 // Set initial last visible tab
    Show(true);

    // Show overlay while Python script runs
    m_overlay = new OverlayFrame(this);

    // Start the backend Python process
    StartPythonExecutable();

    // Initialize Windows Terminal if on Windows
#ifdef __WXMSW__
    if (m_winTerminalManager)
    {
        // Post-initialization command if needed after Python script provides container ID
        // This might need to be deferred until OnTerminate of PythonProcess
        // m_winTerminalManager->SendCommand(L"echo Terminal Ready\r\n");
    }
#endif
}

SecondWindow::~SecondWindow()
{
    wxLogDebug("SecondWindow::~SecondWindow - START");

    // Stop and delete the close timer if it exists
    if (m_closeTimer)
    {
        if (m_closeTimer->IsRunning())
        {
            m_closeTimer->Stop();
        }
        delete m_closeTimer;
        m_closeTimer = nullptr;
        wxLogDebug("SecondWindow::~SecondWindow - Closed timer deleted.");
    }

    // Ensure the Python backend thread/process is signaled to stop if applicable
    CleanupThread(); // (This function might need implementation depending on Python process management)

    // Wait for the cleanup thread if it's running
    if (m_cleanupThread)
    {
        wxLogDebug("SecondWindow::~SecondWindow - Waiting for container cleanup thread to finish.");
        if (m_cleanupThread->IsRunning())
        {
            m_cleanupThread->Wait(); // Block until the thread completes
        }
        delete m_cleanupThread; // Delete the thread object
        m_cleanupThread = nullptr;
        wxLogDebug("SecondWindow::~SecondWindow - Container cleanup thread finished and deleted.");
    }

    // Optional: Explicitly clean up the container if the thread didn't run/finish
    // This is a fallback, the thread is the preferred way.
    if (!m_containerId.IsEmpty())
    {
        // NOTE: Removing synchronous cleanup here as the thread *should* handle it.
        // If issues arise where containers aren't cleaned up on close, this might be re-added
        // with careful checks (e.g., check if thread existed and ran).
        wxLogDebug("SecondWindow::~SecondWindow - Skipping synchronous container cleanup (handled by thread).");
    }
    else
    {
        wxLogDebug("SecondWindow::~SecondWindow - m_containerId is empty, skipping synchronous cleanup.");
    }

#ifdef __WXMSW__
    // Clean up Windows Terminal Manager
    delete m_winTerminalManager;
    m_winTerminalManager = nullptr;
    wxLogDebug("SecondWindow::~SecondWindow - m_winTerminalManager deleted");
#endif

    wxLogDebug("SecondWindow::~SecondWindow - END");
}

void SecondWindow::CloseOverlay()
{
    if (m_overlay)
    {
        m_overlay->Hide();    // Hide it first
        m_overlay->Destroy(); // Then destroy it
        m_overlay = nullptr;
        wxLogDebug("Overlay closed and destroyed.");
    }
}

void SecondWindow::StartPythonExecutable()
{
    wxString pythonExePath = "script.exe"; // Ensure this is in PATH or provide full path

    // Check if the executable exists
    if (!wxFileName::FileExists(pythonExePath))
    {
        pythonExePath = wxFileName::GetCwd() + wxFILE_SEP_PATH + "script.exe"; // Try current dir
        if (!wxFileName::FileExists(pythonExePath))
        {
            wxMessageBox("Error: script.exe not found!\nPlease ensure it is in the application directory or system PATH.",
                         "Executable Not Found", wxOK | wxICON_ERROR);
            CloseOverlay(); // Close overlay if we can't start
            // Optionally close the window or disable functionality
            // GetParent()->Close(); // Example: Close the parent (MainFrame)
            return;
        }
    }

    // Ensure project and ISO paths are valid before proceeding
    if (m_projectDir.IsEmpty() || !wxDirExists(m_projectDir))
    {
        wxMessageBox("Error: Project directory is invalid or does not exist.\nPath: " + m_projectDir,
                     "Invalid Project Directory", wxOK | wxICON_ERROR);
        CloseOverlay();
        return;
    }
    if (m_isoPath.IsEmpty() || !wxFileExists(m_isoPath))
    {
        wxMessageBox("Error: ISO file path is invalid or the file does not exist.\nPath: " + m_isoPath,
                     "Invalid ISO Path", wxOK | wxICON_ERROR);
        CloseOverlay();
        return;
    }

    wxString command = wxString::Format("\"%s\" --project-dir \"%s\" --iso-path \"%s\"",
                                        pythonExePath, m_projectDir, m_isoPath);
    wxLogDebug("Executing Python command: %s", command);

    PythonProcess *proc = new PythonProcess(this);                           // Use our custom process class
    long pid = wxExecute(command, wxEXEC_ASYNC | wxEXEC_HIDE_CONSOLE, proc); // Run hidden

    if (pid == 0)
    {
        wxLogError("Failed to start Python executable: %s", command);
        wxMessageBox("Failed to launch the backend process (script.exe)!", "Execution Error", wxICON_ERROR);
        delete proc;             // Clean up the process object
        CloseOverlay();          // Close the loading overlay
        m_threadRunning = false; // Ensure flag is reset
        // Consider disabling further actions or closing the window here
    }
    else
    {
        wxLogDebug("Python executable started successfully, PID: %ld", pid);
        m_threadRunning = true; // Indicate the process is running
    }
}

// This function might need more implementation if the Python process needs explicit stopping
void SecondWindow::CleanupThread()
{
    wxLogDebug("SecondWindow::CleanupThread - START");
    if (m_threadRunning)
    {
        wxLogDebug("SecondWindow::CleanupThread - Backend process was running, setting flag to false.");
        // If PythonProcess has a PID, you might send a signal here if graceful termination is needed
        // e.g., if (python_pid > 0) wxKill(python_pid, wxSIGTERM);
        m_threadRunning = false;
    }
    else
    {
        wxLogDebug("SecondWindow::CleanupThread - Backend process flag was already false.");
    }
    wxLogDebug("SecondWindow::CleanupThread - END");
}

void SecondWindow::ExecuteDockerCommand(const wxString &containerId)
{
    if (containerId.IsEmpty())
    {
        wxLogWarning("ExecuteDockerCommand called with empty container ID.");
        return;
    }

#ifdef __WXMSW__
    // For Windows Terminal Manager
    if (m_winTerminalManager)
    {
        // Clear screen, execute docker command, then enter chroot
        // Sending commands separated by '\r\n' simulates pressing Enter after each
        wxString command = wxString::Format(
            // L"cls\r\n" // Clear screen - might clear too early
            L"docker exec -it %s /bin/bash\r\n" // Enter the container
            // L"chroot /root/custom_iso/squashfs-root /bin/bash\r\n" // Enter chroot *after* entering container - User might need to type this
            ,
            containerId);
        wxLogDebug("Sending command to Windows Terminal: %s", command);
        m_winTerminalManager->SendCommand(command.ToStdWstring());
    }
#else // Assuming Linux Terminal Panel for non-Windows
    if (m_terminalPanel)
    {
        // Linux terminal might handle this differently, possibly via the initial xterm command
        // or by sending input directly if the terminal widget supports it.
        // For a basic embedded xterm, the initial command is usually sufficient.
        wxLogDebug("Linux terminal detected, initial command should handle container entry.");
        // If you needed to send *more* commands later, you'd need a mechanism
        // to write to the xterm's PTY (Pseudo-Terminal), which is more complex.
    }
#endif
}

void SecondWindow::CreateControls()
{
    m_mainPanel = new wxPanel(this);
    wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
    m_mainPanel->SetBackgroundColour(wxColour(40, 44, 52)); // Consistent background

    // --- Top panel with MongoDB button ---
    wxPanel *topPanel = new wxPanel(m_mainPanel);
    topPanel->SetBackgroundColour(wxColour(20, 20, 20)); // Darker shade
    wxBoxSizer *topSizer = new wxBoxSizer(wxHORIZONTAL);

    wxButton *mongoButton = new wxButton(topPanel, ID_MONGODB_BUTTON, "MongoDB",
                                         wxDefaultPosition, wxSize(100, 30), wxBORDER_NONE);
    // Style the button (Consider using ThemeConfig)
    mongoButton->SetBackgroundColour(wxColour(77, 171, 68)); // MongoDB green
    mongoButton->SetForegroundColour(*wxWHITE);
    wxFont topButtonFont = mongoButton->GetFont();
    topButtonFont.SetWeight(wxFONTWEIGHT_BOLD);
    mongoButton->SetFont(topButtonFont);

    topSizer->AddStretchSpacer(1); // Push button to the right
    topSizer->Add(mongoButton, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    topSizer->AddSpacer(10); // Right margin
    topPanel->SetSizer(topSizer);

    // --- Tab Bar ---
    wxPanel *tabPanel = new wxPanel(m_mainPanel);
    tabPanel->SetBackgroundColour(wxColour(30, 30, 30));         // Tab bar background
    wxBoxSizer *tabSizer = new wxBoxSizer(wxHORIZONTAL);         // Main sizer for centering
    wxBoxSizer *centeredTabSizer = new wxBoxSizer(wxHORIZONTAL); // Holds the actual buttons

    wxButton *terminalButton = new wxButton(tabPanel, ID_TERMINAL_TAB, "TERMINAL",
                                            wxDefaultPosition, wxSize(120, 40), wxBORDER_NONE); // Wider buttons
    wxButton *sqlButton = new wxButton(tabPanel, ID_SQL_TAB, "CONFIG",                          // Renamed from SQL
                                       wxDefaultPosition, wxSize(120, 40), wxBORDER_NONE);

    // Initial styling (Terminal tab active by default)
    terminalButton->SetBackgroundColour(wxColour(44, 49, 58)); // Active tab color
    terminalButton->SetForegroundColour(*wxWHITE);
    sqlButton->SetBackgroundColour(wxColour(30, 30, 30));    // Inactive tab color
    sqlButton->SetForegroundColour(wxColour(156, 163, 175)); // Dim text for inactive

    wxFont tabFont = terminalButton->GetFont();
    tabFont.SetWeight(wxFONTWEIGHT_BOLD);
    terminalButton->SetFont(tabFont);
    sqlButton->SetFont(tabFont);

    // Add buttons to the centered sizer
    centeredTabSizer->Add(terminalButton, 0, wxEXPAND);
    // Optional: Add a visual separator if desired
    // wxStaticLine* separator = new wxStaticLine(tabPanel, wxID_ANY, wxDefaultPosition, wxSize(-1, 20), wxLI_VERTICAL);
    // separator->SetBackgroundColour(wxColour(60,60,60)); // Separator color
    // centeredTabSizer->Add(separator, 0, wxEXPAND | wxLEFT | wxRIGHT, 0);
    centeredTabSizer->Add(sqlButton, 0, wxEXPAND);

    // Center the button group horizontally
    tabSizer->AddStretchSpacer(1);
    tabSizer->Add(centeredTabSizer, 0, wxALIGN_CENTER);
    tabSizer->AddStretchSpacer(1);
    tabPanel->SetSizer(tabSizer);

    // --- Content Panels ---
    // Terminal Tab Panel
    m_terminalTab = new wxPanel(m_mainPanel);
    m_terminalTab->SetBackgroundColour(wxColour(30, 30, 30)); // Match tab bar color or slightly different
    wxBoxSizer *terminalSizer = new wxBoxSizer(wxVERTICAL);

    OSDetector::OS currentOS = m_osDetector.GetCurrentOS();
    if (currentOS == OSDetector::OS::Linux)
    {
#ifdef __WXGTK__ // Ensure Linux-specific code is guarded
        m_terminalPanel = new LinuxTerminalPanel(m_terminalTab);
        m_terminalPanel->SetMinSize(wxSize(680, 400));               // Minimum size
        terminalSizer->Add(m_terminalPanel, 1, wxEXPAND | wxALL, 5); // Add with padding
#else
        // Fallback for Linux builds without GTK or specific terminal widget
        wxStaticText *linuxPlaceholder = new wxStaticText(m_terminalTab, wxID_ANY, "Linux Terminal Placeholder");
        terminalSizer->Add(linuxPlaceholder, 1, wxEXPAND | wxALL, 5);
#endif
    }
#ifdef __WXMSW__
    else if (currentOS == OSDetector::OS::Windows)
    {
        wxPanel *winTerminalPanel = new wxPanel(m_terminalTab, wxID_ANY, wxDefaultPosition, wxDefaultSize);
        // Let the sizer manage the size, no need for explicit size here unless required
        winTerminalPanel->SetBackgroundColour(wxColour(12, 12, 12));  // Standard Windows Terminal black
        terminalSizer->Add(winTerminalPanel, 1, wxEXPAND | wxALL, 0); // No padding for embedded terminal

        m_winTerminalManager = new WinTerminalManager(winTerminalPanel);
        if (!m_winTerminalManager->Initialize())
        {
            wxLogError("Failed to initialize Windows terminal");
            // Optionally display an error message in the panel
            delete m_winTerminalManager;
            m_winTerminalManager = nullptr;
            wxStaticText *errorText = new wxStaticText(winTerminalPanel, wxID_ANY, "Failed to initialize embedded terminal.");
            wxBoxSizer *errorSizer = new wxBoxSizer(wxVERTICAL);
            errorSizer->Add(errorText, 1, wxCENTER | wxALL, 10);
            winTerminalPanel->SetSizer(errorSizer);
        }
    }
#endif
    else
    {
        // Placeholder for other OS or if no specific terminal is implemented
        wxStaticText *osPlaceholder = new wxStaticText(m_terminalTab, wxID_ANY, "Terminal not available on this OS.");
        terminalSizer->Add(osPlaceholder, 1, wxEXPAND | wxALL, 5);
    }
    m_terminalTab->SetSizer(terminalSizer);

    // Config Tab Panel (using SQLTab class)
    m_sqlTab = new SQLTab(m_mainPanel, m_projectDir); // Pass projectDir
    m_sqlTab->Hide();                                 // Starts hidden

    // MongoDB Panel
    m_mongoPanel = new MongoDBPanel(m_mainPanel); // Create instance
    m_mongoPanel->Hide();                         // Starts hidden

    // --- Bottom Button Panel ---
    wxPanel *buttonPanel = new wxPanel(m_mainPanel);
    buttonPanel->SetBackgroundColour(wxColour(30, 30, 30)); // Match tab bar
    wxBoxSizer *buttonSizer = new wxBoxSizer(wxHORIZONTAL);

    wxButton *nextButton = new wxButton(buttonPanel, ID_NEXT_BUTTON, "Create ISO", // More descriptive label
                                        wxDefaultPosition, wxSize(140, 35));       // Wider button
    // Style the button (Consider using ThemeConfig)
    nextButton->SetBackgroundColour(wxColour(189, 147, 249)); // A distinct accent color
    nextButton->SetForegroundColour(*wxWHITE);
    wxFont bottomButtonFont = nextButton->GetFont();
    bottomButtonFont.SetWeight(wxFONTWEIGHT_BOLD);
    nextButton->SetFont(bottomButtonFont);

    buttonSizer->AddStretchSpacer(1);                                     // Push button to the right
    buttonSizer->Add(nextButton, 0, wxALL | wxALIGN_CENTER_VERTICAL, 10); // Add padding
    buttonPanel->SetSizer(buttonSizer);

    // --- Assemble Main Layout ---
    mainSizer->Add(topPanel, 0, wxEXPAND);
    // Optional: Add a separator line below top bar
    // mainSizer->Add(new wxStaticLine(m_mainPanel, wxID_ANY), 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
    mainSizer->Add(tabPanel, 0, wxEXPAND);      // Add the tab bar
    mainSizer->Add(m_terminalTab, 1, wxEXPAND); // Add terminal content panel (initially shown)
    mainSizer->Add(m_sqlTab, 1, wxEXPAND);      // Add config content panel (initially hidden)
    mainSizer->Add(m_mongoPanel, 1, wxEXPAND);  // Add mongo content panel (initially hidden)
    // Add a separator line above bottom buttons
    mainSizer->Add(new wxStaticLine(m_mainPanel, wxID_ANY), 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 5);
    mainSizer->Add(buttonPanel, 0, wxEXPAND); // Add bottom button panel

    m_mainPanel->SetSizer(mainSizer);

    // Set overall frame size constraints
    SetMinSize(wxSize(800, 650)); // Adjusted minimum size
    SetSize(wxSize(850, 700));    // Slightly larger default size
}

void SecondWindow::OnClose(wxCloseEvent &event)
{
    if (m_isClosing)
    {
        wxLogDebug("SecondWindow::OnClose - Already in closing process, skipping.");
        event.Skip(); // Allow default processing if needed, or just return
        return;
    }
    wxLogDebug("SecondWindow::OnClose - Starting close process.");
    m_isClosing = true;
    Hide(); // Hide the window immediately

    // Start the asynchronous cleanup
    CleanupContainerAsync();

    // Start a timer to periodically check if the cleanup thread is done
    if (!m_closeTimer)
    {
        m_closeTimer = new wxTimer(this); // Use 'this' as the owner
    }
    // Start timer with a reasonable interval (e.g., 200ms)
    m_closeTimer->Start(200);

    // Don't destroy the window yet, let the timer handle it
    // event.Skip(false); // Prevent immediate destruction if default is to destroy
}

void SecondWindow::OnCloseTimer(wxTimerEvent &event)
{
    wxLogDebug("SecondWindow::OnCloseTimer - Check");
    // Check if the cleanup thread exists and is still running
    if (m_cleanupThread && m_cleanupThread->IsRunning())
    {
        wxLogDebug("SecondWindow::OnCloseTimer - Cleanup thread still running, waiting...");
        return; // Wait for the next timer event
    }

    // If timer is running, stop it
    if (m_closeTimer && m_closeTimer->IsRunning())
    {
        m_closeTimer->Stop();
        wxLogDebug("SecondWindow::OnCloseTimer - Timer stopped.");
    }

    wxLogDebug("SecondWindow::OnCloseTimer - Cleanup thread finished or not running, proceeding to destroy window.");
    // Use CallAfter to ensure destruction happens safely outside the event handler
    CallAfter(&SecondWindow::DoDestroy);
}

void SecondWindow::DoDestroy()
{
    wxLogDebug("SecondWindow::DoDestroy - Calling Destroy().");
    Destroy(); // Actually destroy the window object
}

void SecondWindow::CleanupContainerAsync()
{
    wxLogDebug("SecondWindow::CleanupContainerAsync - Attempting to start cleanup thread.");
    if (m_containerId.IsEmpty())
    {
        wxLogDebug("SecondWindow::CleanupContainerAsync - No container ID, skipping cleanup thread.");
        // If closing, ensure the close timer logic progresses IF the timer exists
        if (m_isClosing && m_closeTimer && !m_closeTimer->IsRunning())
        {
            // If timer isn't running somehow, force destroy check
            CallAfter(&SecondWindow::DoDestroy);
        }
        return;
    }

    // Check if thread already exists and is running
    if (m_cleanupThread && m_cleanupThread->IsRunning())
    {
        wxLogDebug("SecondWindow::CleanupContainerAsync - Cleanup thread is already running.");
        return; // Don't start another one
    }

    // Clean up old thread object if it exists but isn't running
    if (m_cleanupThread)
    {
        // No need to Wait() here, as we checked IsRunning() above
        delete m_cleanupThread;
        m_cleanupThread = nullptr;
        wxLogDebug("SecondWindow::CleanupContainerAsync - Deleted previous (non-running) cleanup thread object.");
    }

    // Create and run the new cleanup thread
    m_cleanupThread = new ContainerCleanupThread(m_containerId);
    if (m_cleanupThread->Run() != wxTHREAD_NO_ERROR)
    {
        wxLogError("SecondWindow::CleanupContainerAsync - Failed to create or run container cleanup thread.");
        delete m_cleanupThread; // Clean up if Run failed
        m_cleanupThread = nullptr;

        // If we are in the closing sequence, and couldn't start the thread,
        // allow the close process to continue via the timer check.
        if (m_isClosing && m_closeTimer && !m_closeTimer->IsRunning())
        {
            CallAfter(&SecondWindow::DoDestroy); // Force destroy check if timer isn't running
        }
    }
    else
    {
        wxLogDebug("SecondWindow::CleanupContainerAsync - Cleanup thread started successfully for container %s.", m_containerId);
    }
}

void SecondWindow::OnNext(wxCommandEvent &event)
{
    wxButton *nextButton = wxDynamicCast(FindWindow(ID_NEXT_BUTTON), wxButton);
    if (nextButton)
        nextButton->Disable(); // Disable button immediately

    wxString containerId;
    wxString containerIdPath = wxFileName(m_projectDir, "container_id.txt").GetFullPath();
    wxLogDebug("OnNext: Checking for container ID at: %s", containerIdPath);

    if (wxFileExists(containerIdPath))
    {
        wxFile file;
        if (file.Open(containerIdPath))
        {
            file.ReadAll(&containerId);
            containerId.Trim();
            wxLogDebug("OnNext: Read container ID: %s", containerId);
            file.Close(); // Close the file
        }
        else
        {
            wxLogError("OnNext: Container ID file exists but could not be opened!");
            wxMessageBox("Error: Could not read container information.", "File Error", wxICON_ERROR);
            if (nextButton)
                nextButton->Enable(); // Re-enable button on error
            return;
        }
    }
    else
    {
        wxLogError("OnNext: Container ID file not found!");
        wxMessageBox("Error: Container information not found. Has the initial setup completed?", "Configuration Error", wxICON_ERROR);
        if (nextButton)
            nextButton->Enable(); // Re-enable button
        return;
    }

    // Ensure container ID is not empty after reading
    if (containerId.IsEmpty())
    {
        wxLogError("OnNext: Container ID is empty after reading the file!");
        wxMessageBox("Error: Container information is invalid.", "Configuration Error", wxICON_ERROR);
        if (nextButton)
            nextButton->Enable();
        return;
    }

    // Show overlay
    if (m_overlay)
    { // Destroy existing overlay if any
        m_overlay->Destroy();
        m_overlay = nullptr;
    }
    m_overlay = new OverlayFrame(this);
    wxLogDebug("OnNext: Overlay shown.");

    // Execute the ISO creation script
    wxString command = wxString::Format("docker exec %s /create_iso.sh", containerId);
    wxLogDebug("OnNext: Executing ISO creation command: %s", command);

    DockerExecProcess *proc = new DockerExecProcess(this); // Handles OnTerminate
    long pid = wxExecute(command, wxEXEC_ASYNC | wxEXEC_HIDE_CONSOLE, proc);

    if (pid == 0)
    {
        wxLogError("OnNext: Failed to start docker exec for create_iso.sh");
        wxMessageBox("Failed to start the ISO creation process!", "Execution Error", wxICON_ERROR);
        delete proc;    // Clean up process object
        CloseOverlay(); // Close the loading overlay
        if (nextButton)
            nextButton->Enable(); // Re-enable button
    }
    else
    {
        wxLogDebug("OnNext: ISO creation process started, PID: %ld", pid);
        // The button will be re-enabled in DockerExecProcess::OnTerminate
    }
}

void SecondWindow::OnTabChanged(wxCommandEvent &event)
{
    wxButton *terminalButton = wxDynamicCast(FindWindow(ID_TERMINAL_TAB), wxButton);
    wxButton *configButton = wxDynamicCast(FindWindow(ID_SQL_TAB), wxButton); // Changed variable name

    // Deactivate both buttons visually first
    const wxColour inactiveBg(30, 30, 30);
    const wxColour inactiveFg(156, 163, 175);
    const wxColour activeBg(44, 49, 58);
    const wxColour activeFg(*wxWHITE);

    if (terminalButton)
    {
        terminalButton->SetBackgroundColour(inactiveBg);
        terminalButton->SetForegroundColour(inactiveFg);
        terminalButton->Refresh(); // Apply changes
    }
    if (configButton)
    {
        configButton->SetBackgroundColour(inactiveBg);
        configButton->SetForegroundColour(inactiveFg);
        configButton->Refresh(); // Apply changes
    }

    // Hide all main content panels initially
    m_terminalTab->Hide();
    m_sqlTab->Hide();
    m_mongoPanel->Hide(); // Also hide mongo panel if it was open

    // Show the selected tab and activate its button
    int id = event.GetId();
    if (id == ID_TERMINAL_TAB)
    {
        m_terminalTab->Show();
        if (terminalButton)
        {
            terminalButton->SetBackgroundColour(activeBg);
            terminalButton->SetForegroundColour(activeFg);
            terminalButton->Refresh();
        }
        m_lastTab = m_terminalTab; // Update last active tab
        wxLogDebug("Switched to Terminal Tab");
    }
    else if (id == ID_SQL_TAB)
    { // Changed ID check
        m_sqlTab->Show();
        if (configButton)
        {
            configButton->SetBackgroundColour(activeBg);
            configButton->SetForegroundColour(activeFg);
            configButton->Refresh();
        }
        m_lastTab = m_sqlTab; // Update last active tab
        wxLogDebug("Switched to Config Tab");
    }

    // Reset MongoDB button appearance if it was active
    wxButton *mongoButton = wxDynamicCast(FindWindow(ID_MONGODB_BUTTON), wxButton);
    if (mongoButton)
    {
        mongoButton->SetBackgroundColour(wxColour(77, 171, 68)); // Original green
        mongoButton->Refresh();
    }

    m_mainPanel->Layout(); // Recalculate layout for the main panel
}

void SecondWindow::OnMongoDBButton(wxCommandEvent &event)
{
    wxLogDebug("MongoDB button clicked");
    // Store which tab was previously visible
    if (m_terminalTab->IsShown())
    {
        m_lastTab = m_terminalTab;
    }
    else if (m_sqlTab->IsShown())
    {
        m_lastTab = m_sqlTab;
    }
    else
    {
        m_lastTab = m_terminalTab;
    } // Default fallback if neither known tab is shown

    wxLogDebug("Last active tab stored.");

    // Hide other tabs
    m_terminalTab->Hide();
    m_sqlTab->Hide();
    wxLogDebug("Terminal and Config tabs hidden.");

    // Show and load MongoDB panel
    m_mongoPanel->Show();
    wxLogDebug("MongoDB panel shown.");
    m_mongoPanel->LoadMongoDBContent(); // Load/refresh data when shown
    wxLogDebug("MongoDB content loading initiated.");

    // Optional: Change button appearance to indicate active state
    wxButton *mongoButton = wxDynamicCast(event.GetEventObject(), wxButton);
    if (mongoButton)
    {
        mongoButton->SetBackgroundColour(wxColour(100, 200, 100)); // Example: Lighter green
        mongoButton->Refresh();
        wxLogDebug("MongoDB button appearance updated to active state.");
    }

    m_mainPanel->Layout(); // Update layout
    wxLogDebug("Main panel layout updated.");
}

// Handler in SecondWindow to catch the close event from MongoDBPanel
void SecondWindow::OnMongoDBPanelClose(wxCommandEvent &event)
{
    wxLogDebug("SecondWindow::OnMongoDBPanelClose received event");
    m_mongoPanel->Hide(); // Hide the MongoDB panel

    // Show the previously active tab
    if (m_lastTab)
    {
        m_lastTab->Show();
        wxLogDebug("Restored last active tab.");
    }
    else
    {
        m_terminalTab->Show(); // Default fallback to terminal tab
        wxLogDebug("Fell back to showing Terminal tab.");
    }

    // Reset MongoDB button appearance to its normal state
    wxButton *mongoButton = wxDynamicCast(FindWindow(ID_MONGODB_BUTTON), wxButton);
    if (mongoButton)
    {
        mongoButton->SetBackgroundColour(wxColour(77, 171, 68)); // Original MongoDB green
        mongoButton->Refresh();
        wxLogDebug("MongoDB button appearance reset.");
    }

    m_mainPanel->Layout(); // Update layout
    wxLogDebug("Main panel layout updated after closing MongoDB panel.");
}