#include "SecondWindow.h"
#include "DesktopTab.h"
#include <wx/utils.h>
#include <wx/statline.h>
#include <wx/process.h>
#include <wx/dcbuffer.h>
#include <wx/filename.h>
#include <wx/file.h>
#include <wx/arrstr.h>
#include <wx/log.h>
#include <cmath>
#include <chrono>
#include <mongocxx/uri.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/collection.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>

#ifdef __WXMSW__
#include <windows.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

// Initialize static member
mongocxx::instance SecondWindow::m_mongoInstance{};

//---------------------------------------------------------------------
// PythonProcess class
class PythonProcess : public wxProcess
{
public:
    PythonProcess(SecondWindow *parent) : wxProcess(parent), m_parent(parent)
    {
    }

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

                wxString guiDetectCommand = wxString::Format("docker exec %s cat /root/custom_iso/detected_gui.txt", containerId);
                wxArrayString output, errors;
                int exitCode = wxExecute(guiDetectCommand, output, errors, wxEXEC_SYNC | wxEXEC_HIDE_CONSOLE);

                if (exitCode == 0 && !output.IsEmpty())
                {
                    wxString guiName = output[0];
                    guiName.Trim(true).Trim(false);
                    guiName.Replace("\n", "");

                    wxLogDebug("Detected GUI environment before event: %s", guiName);

                    wxString localGuiPath = wxFileName(m_parent->GetProjectDir(), "detected_gui.txt").GetFullPath();
                    wxFile guiFile;
                    if (guiFile.Create(localGuiPath, true) && guiFile.IsOpened())
                    {
                        guiFile.Write(guiName);
                        guiFile.Close();
                        wxLogDebug("GUI name saved to file: %s", localGuiPath);
                    }

                    if (m_parent->GetDesktopTab())
                    {
                        wxCommandEvent *guiEvent = new wxCommandEvent(FILE_COPY_COMPLETE_EVENT);
                        guiEvent->SetString(guiName);
                        wxLogDebug("Posting event with GUI name: %s", guiName);
                        wxPostEvent(m_parent->GetDesktopTab(), *guiEvent);
                        delete guiEvent;
                    }
                }
                m_parent->ExecuteDockerCommand(containerId);
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
    DockerExecProcess(SecondWindow *parent) : wxProcess(parent), m_parent(parent)
    {
    }

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
            }
        }
    }

private:
    SecondWindow *m_parent;
};
//---------------------------------------------------------------------

//---------------------------------------------------------------------
// OverlayFrame class (unchanged)
class OverlayFrame : public wxDialog
{
public:
    OverlayFrame(wxWindow *parent) : wxDialog(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxNO_BORDER | wxFRAME_FLOAT_ON_PARENT)
    {
        SetBackgroundColour(wxColour(0, 0, 0));
        SetTransparent(200);
        SetBackgroundStyle(wxBG_STYLE_PAINT);

        m_parentWindow = parent;
        UpdatePositionAndSize();

        m_animationAngle = 0.0;

        Bind(wxEVT_PAINT, &OverlayFrame::OnPaint, this);
        Bind(wxEVT_TIMER, &OverlayFrame::OnTimer, this);

        if (parent)
        {
            parent->Bind(wxEVT_MOVE, &OverlayFrame::OnParentMoveOrResize, this);
            parent->Bind(wxEVT_SIZE, &OverlayFrame::OnParentMoveOrResize, this);
        }

        m_timer.SetOwner(this);
        m_timer.Start(30);
        Show(true);
    }

    ~OverlayFrame()
    {
        if (m_parentWindow)
        {
            m_parentWindow->Unbind(wxEVT_MOVE, &OverlayFrame::OnParentMoveOrResize, this);
            m_parentWindow->Unbind(wxEVT_SIZE, &OverlayFrame::OnParentMoveOrResize, this);
        }
        m_timer.Stop();
    }

private:
    void UpdatePositionAndSize()
    {
        if (m_parentWindow)
        {
            wxRect clientRect = m_parentWindow->GetClientRect();
            SetPosition(m_parentWindow->ClientToScreen(clientRect.GetTopLeft()));
            SetSize(clientRect.GetSize());
        }
    }

    void OnParentMoveOrResize(wxEvent &event)
    {
        UpdatePositionAndSize();
    }

    void OnPaint(wxPaintEvent &event)
    {
        wxAutoBufferedPaintDC dc(this);
        dc.Clear();
        wxSize size = GetClientSize();
        dc.SetBrush(wxBrush(wxColour(0, 0, 0, 230)));
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawRectangle(0, 0, size.GetWidth(), size.GetHeight());
        DrawLoadingAnimation(dc, size);
    }

    void DrawLoadingAnimation(wxDC &dc, const wxSize &size)
    {
        wxPoint center(size.GetWidth() / 2, size.GetHeight() / 2);
        int radius = std::min(size.GetWidth(), size.GetHeight()) / 6;
        int numPoints = 8;
        int pointRadius = radius / 4;

        dc.SetBrush(*wxWHITE_BRUSH);
        dc.SetPen(*wxWHITE_PEN);

        for (int i = 0; i < numPoints; ++i)
        {
            double angle = m_animationAngle + (2.0 * M_PI * i / numPoints);
            int x = center.x + static_cast<int>(radius * cos(angle));
            int y = center.y + static_cast<int>(radius * sin(angle));
            dc.DrawCircle(x, y, pointRadius);
        }
    }

    void OnTimer(wxTimerEvent &event)
    {
        m_animationAngle += 0.1;
        if (m_animationAngle > 2.0 * M_PI)
            m_animationAngle -= 2.0 * M_PI;
        Refresh();
    }

    double m_animationAngle;
    wxTimer m_timer;
    wxWindow *m_parentWindow;
};
//---------------------------------------------------------------------

wxDEFINE_EVENT(PYTHON_TASK_COMPLETED, wxCommandEvent);
wxDEFINE_EVENT(PYTHON_LOG_UPDATE, wxCommandEvent);

// Event table
wxBEGIN_EVENT_TABLE(SecondWindow, wxFrame)
    EVT_CLOSE(SecondWindow::OnClose)
        EVT_BUTTON(ID_TERMINAL_TAB, SecondWindow::OnTabChanged)
            EVT_BUTTON(ID_SQL_TAB, SecondWindow::OnTabChanged)
                EVT_BUTTON(ID_NEXT_BUTTON, SecondWindow::OnNext)
                    EVT_BUTTON(ID_MONGODB_BUTTON, SecondWindow::OnMongoDBButton)
                        EVT_BUTTON(ID_MONGODB_PANEL_CLOSE, SecondWindow::OnMongoDBPanelClose)
                            wxEND_EVENT_TABLE()

    // Add the thread class definition before SecondWindow.
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
            return (ExitCode)0; // Exit immediately
        }
        ContainerManager::Get().CleanupContainer(m_containerId);
        wxLogDebug("ContainerCleanupThread::Entry - Cleanup thread finished for container: %s", m_containerId);
        return (ExitCode)0; // Important for detached threads.
    }

private:
    wxString m_containerId;
};

SecondWindow::SecondWindow(wxWindow *parent,
                           const wxString &title,
                           const wxString &isoPath,
                           const wxString &projectDir,
                           DesktopTab *desktopTab)
    : wxFrame(parent, wxID_ANY, title,
              wxDefaultPosition, wxSize(800, 650),
              wxDEFAULT_FRAME_STYLE | wxSYSTEM_MENU | wxCAPTION |
                  wxCLOSE_BOX | wxCLIP_CHILDREN | wxMINIMIZE_BOX |
                  wxMAXIMIZE_BOX | wxRESIZE_BORDER),
      m_isoPath(isoPath),
      m_projectDir(projectDir),
      m_threadRunning(false),
      m_overlay(nullptr),
      m_desktopTab(desktopTab),
      m_mongoPanel(nullptr),
      m_cleanupThread(nullptr),
      m_isClosing(false),
      m_closeTimer(nullptr)
#ifdef __WXMSW__
      ,
      m_winTerminalManager(nullptr)
#endif
{
    wxLogDebug("SecondWindow constructor: desktopTab pointer = %p", m_desktopTab);

#ifdef __WXMSW__
    BOOL value = TRUE;
    DwmSetWindowAttribute(GetHandle(), DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
#endif

    m_containerId = ContainerManager::Get().GetCurrentContainerId();
    CreateControls();
    Centre();
    SetBackgroundColour(wxColour(40, 44, 52));

    Show(true);
    m_overlay = new OverlayFrame(this);

    StartPythonExecutable();

#ifdef __WXMSW__
    if (m_winTerminalManager)
    {
        // m_winTerminalManager->SendCommand(L" "); //Removed usless line.
    }
#endif
}

SecondWindow::~SecondWindow()
{
    wxLogDebug("SecondWindow::~SecondWindow - START");

    // Clean up the timer
    if (m_closeTimer)
    {
        if (m_closeTimer->IsRunning())
        {
            m_closeTimer->Stop();
        }
        delete m_closeTimer;
        m_closeTimer = nullptr;
    }

    CleanupThread(); // Call CleanupThread again in destructor as a safety measure

    // Cleanup the container cleanup thread, if it exists:
    if (m_cleanupThread)
    {
        wxLogDebug("SecondWindow::~SecondWindow - Waiting for cleanup thread to finish.");
        if (m_cleanupThread->IsRunning())
            m_cleanupThread->Wait(); // Wait for the thread to finish.  Important.
        delete m_cleanupThread;      // safe to delete after waiting.
        m_cleanupThread = nullptr;
    }

    if (!m_containerId.IsEmpty())
    {
        // DO NOT CALL `CleanupContainer` HERE. IT SHOULD BE DONE ASYNCHRONOUSLY

        wxLogDebug("SecondWindow::~SecondWindow - Skipping synchronous ContainerManager::CleanupContainer(). Should be handled by thread.");
    }
    else
    {
        wxLogDebug("SecondWindow::~SecondWindow - m_containerId is empty, skipping CleanupContainer()");
    }
    wxLogDebug("SecondWindow::~SecondWindow - END");

#ifdef __WXMSW__
    delete m_winTerminalManager;
    wxLogDebug("SecondWindow::~SecondWindow - m_winTerminalManager deleted");
#endif
}

void SecondWindow::CloseOverlay()
{
    if (m_overlay)
    {
        m_overlay->Destroy();
        m_overlay = nullptr;
    }
}

void SecondWindow::StartPythonExecutable()
{
    wxString pythonExePath = "script.exe";
    wxString command = wxString::Format("\"%s\" --project-dir \"%s\" --iso-path \"%s\"",
                                        pythonExePath, m_projectDir, m_isoPath);

    PythonProcess *proc = new PythonProcess(this);
    long pid = wxExecute(command, wxEXEC_ASYNC | wxEXEC_HIDE_CONSOLE, proc);

    if (pid == 0)
    {
        wxMessageBox("Failed to start Python executable!", "Error", wxICON_ERROR);
        delete proc;
        CloseOverlay();
    }
    else
    {
        m_threadRunning = true;
    }
}
// Modify SecondWindow::CleanupThread function to avoid dead lock
void SecondWindow::CleanupThread()
{
    wxLogDebug("SecondWindow::CleanupThread - START");
    if (m_threadRunning)
    {
        wxLogDebug("SecondWindow::CleanupThread - m_threadRunning is true, setting to false");
        m_threadRunning = false;
    }
    else
    {
        wxLogDebug("SecondWindow::CleanupThread - m_threadRunning is already false");
    }
    wxLogDebug("SecondWindow::CleanupThread - END");
}

void SecondWindow::ExecuteDockerCommand(const wxString &containerId)
{
#ifdef __WXMSW__
    if (m_winTerminalManager)
    {
        wxString command = wxString::Format(
            "cls && docker exec -it %s /bin/bash\r\n"
            "chroot /root/custom_iso/squashfs-root /bin/bash\r\n",
            containerId);
        m_winTerminalManager->SendCommand(command.ToStdWstring());
    }
#endif
}

void SecondWindow::CreateControls()
{
    m_mainPanel = new wxPanel(this);
    wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);

    // Top panel with MongoDB button
    wxPanel *topPanel = new wxPanel(m_mainPanel);
    topPanel->SetBackgroundColour(wxColour(30, 30, 30));
    wxBoxSizer *topSizer = new wxBoxSizer(wxHORIZONTAL);

    wxButton *mongoButton = new wxButton(topPanel, ID_MONGODB_BUTTON, "MongoDB",
                                         wxDefaultPosition, wxSize(80, 30), wxBORDER_NONE);
    mongoButton->SetBackgroundColour(wxColour(44, 49, 58));
    mongoButton->SetForegroundColour(*wxWHITE);

    topSizer->Add(mongoButton, 0, wxALL, 5);
    topSizer->AddStretchSpacer(1);
    topPanel->SetSizer(topSizer);

    // Tab Bar
    wxPanel *tabPanel = new wxPanel(m_mainPanel);
    tabPanel->SetBackgroundColour(wxColour(30, 30, 30));
    wxBoxSizer *tabSizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *centeredTabSizer = new wxBoxSizer(wxHORIZONTAL);

    wxButton *terminalButton = new wxButton(tabPanel, ID_TERMINAL_TAB, "TERMINAL",
                                            wxDefaultPosition, wxSize(100, 40), wxBORDER_NONE);
    terminalButton->SetBackgroundColour(wxColour(44, 49, 58));
    terminalButton->SetForegroundColour(*wxWHITE);

    wxStaticLine *separator = new wxStaticLine(tabPanel, wxID_ANY,
                                               wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL);

    wxButton *sqlButton = new wxButton(tabPanel, ID_SQL_TAB, "CONFIG",
                                       wxDefaultPosition, wxSize(100, 40), wxBORDER_NONE);
    sqlButton->SetBackgroundColour(wxColour(30, 30, 30));
    sqlButton->SetForegroundColour(wxColour(128, 128, 128));

    centeredTabSizer->Add(terminalButton, 0, wxEXPAND);
    centeredTabSizer->Add(separator, 0, wxEXPAND | wxTOP | wxBOTTOM, 5);
    centeredTabSizer->Add(sqlButton, 0, wxEXPAND);

    tabSizer->AddStretchSpacer(1);
    tabSizer->Add(centeredTabSizer, 0, wxALIGN_CENTER);
    tabSizer->AddStretchSpacer(1);
    tabPanel->SetSizer(tabSizer);

    // Terminal/SQL/MongoDB Content
    m_terminalTab = new wxPanel(m_mainPanel);
    m_sqlTab = new SQLTab(m_mainPanel, m_projectDir);
    m_mongoPanel = new MongoDBPanel(m_mainPanel);

    wxBoxSizer *terminalSizer = new wxBoxSizer(wxVERTICAL);
    m_terminalTab->SetBackgroundColour(wxColour(30, 30, 30));

    OSDetector::OS currentOS = m_osDetector.GetCurrentOS();
    if (currentOS == OSDetector::OS::Linux)
    {
        m_terminalPanel = new LinuxTerminalPanel(m_terminalTab);
        m_terminalPanel->SetMinSize(wxSize(680, 400));
        terminalSizer->Add(m_terminalPanel, 1, wxEXPAND | wxALL, 5);
    }
#ifdef __WXMSW__
    else
    {
        wxPanel *winTerminalPanel = new wxPanel(m_terminalTab, wxID_ANY,
                                                wxDefaultPosition, wxSize(680, 400));
        winTerminalPanel->SetBackgroundColour(wxColour(30, 30, 30));
        terminalSizer->Add(winTerminalPanel, 1, wxEXPAND | wxALL, 5);

        m_winTerminalManager = new WinTerminalManager(winTerminalPanel);
        if (!m_winTerminalManager->Initialize())
        {
            wxLogError("Failed to initialize Windows terminal");
        }
    }
#endif

    m_terminalTab->SetSizer(terminalSizer);
    m_sqlTab->Hide();
    m_mongoPanel->Hide();

    // Flatpak Store
    m_flatpakStore = new FlatpakStore(m_mainPanel, m_projectDir);
    m_flatpakStore->Hide();

    // Bottom Button Panel
    wxPanel *buttonPanel = new wxPanel(m_mainPanel);
    buttonPanel->SetBackgroundColour(wxColour(30, 30, 30));
    wxBoxSizer *buttonSizer = new wxBoxSizer(wxHORIZONTAL);

    wxButton *nextButton = new wxButton(buttonPanel, ID_NEXT_BUTTON, "Next",
                                        wxDefaultPosition, wxSize(120, 35));
    nextButton->SetBackgroundColour(wxColour(189, 147, 249));
    nextButton->SetForegroundColour(*wxWHITE);

    buttonSizer->AddStretchSpacer(1);
    buttonSizer->Add(nextButton, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    buttonPanel->SetSizer(buttonSizer);

    // Assemble Main Layout
    mainSizer->Add(topPanel, 0, wxEXPAND);
    mainSizer->Add(tabPanel, 0, wxEXPAND);
    mainSizer->Add(m_terminalTab, 1, wxEXPAND);
    mainSizer->Add(m_sqlTab, 1, wxEXPAND);
    mainSizer->Add(m_mongoPanel, 1, wxEXPAND);
    mainSizer->Add(m_flatpakStore, 1, wxEXPAND);
    mainSizer->Add(buttonPanel, 0, wxEXPAND);

    m_mainPanel->SetSizer(mainSizer);
    SetMinSize(wxSize(800, 600));
}

void SecondWindow::OnClose(wxCloseEvent &event)
{
    wxLogDebug("SecondWindow::OnClose - START");

    // If we're already in the process of closing, don't do anything
    if (m_isClosing)
    {
        event.Skip();
        return;
    }

    // Set the closing flag
    m_isClosing = true;

    // Don't destroy the window immediately when closing
    if (event.CanVeto())
    {
        event.Veto();

        // Hide the window while we clean up
        Hide();

        CleanupThread(); // Ensure any previous operations are stopped

        // Start asynchronous container cleanup
        if (!m_containerId.IsEmpty())
        {
            wxLogDebug("SecondWindow::OnClose - Starting asynchronous container cleanup.");
            CleanupContainerAsync(); // Start the cleanup thread

            // Start a timer to check if we can close
            if (!m_closeTimer)
            {
                m_closeTimer = new wxTimer(this);
            }
            m_closeTimer->Start(500); // Check every 500ms
        }
        else
        {
            wxLogDebug("SecondWindow::OnClose - m_containerId is empty, skipping CleanupContainer()");
            // No container to clean up, destroy immediately
            CallAfter(&SecondWindow::DoDestroy);
        }
    }
    else
    {
        // If we can't veto, destroy anyway
        event.Skip();
    }

    wxLogDebug("SecondWindow::OnClose - END");
}

void SecondWindow::OnCloseTimer(wxTimerEvent &event)
{
    // Check if the cleanup thread is still running
    if (m_cleanupThread && m_cleanupThread->IsRunning())
    {
        wxLogDebug("SecondWindow::OnCloseTimer - Cleanup thread still running, waiting...");
        return; // Wait longer
    }

    // Stop the timer
    if (m_closeTimer)
    {
        m_closeTimer->Stop();
    }

    // Now it's safe to destroy the window
    wxLogDebug("SecondWindow::OnCloseTimer - Cleanup thread finished, destroying window");
    CallAfter(&SecondWindow::DoDestroy);
}

void SecondWindow::DoDestroy()
{
    wxLogDebug("SecondWindow::DoDestroy - Actually destroying window");
    Destroy();
}

void SecondWindow::CleanupContainerAsync()
{
    // If we already have a thread and it's running, don't create another
    if (m_cleanupThread && m_cleanupThread->IsRunning())
    {
        wxLogDebug("SecondWindow::CleanupContainerAsync - Cleanup thread is still running.");
        return;
    }

    // If we have a thread but it's not running, delete it
    if (m_cleanupThread)
    {
        delete m_cleanupThread;
        m_cleanupThread = nullptr;
    }

    // Create a new thread
    m_cleanupThread = new ContainerCleanupThread(m_containerId);
    if (m_cleanupThread->Run() != wxTHREAD_NO_ERROR)
    {
        wxLogError("SecondWindow::CleanupContainerAsync - Failed to create cleanup thread.");
        delete m_cleanupThread;
        m_cleanupThread = nullptr;

        // If we can't create a thread, we should still try to close properly
        if (m_isClosing && m_closeTimer)
        {
            m_closeTimer->Stop();
            CallAfter(&SecondWindow::DoDestroy);
        }
    }
}

void SecondWindow::OnNext(wxCommandEvent &event)
{
    wxButton *nextButton = wxDynamicCast(FindWindow(ID_NEXT_BUTTON), wxButton);
    if (nextButton)
        nextButton->Disable();

    wxString containerId;
    wxString containerIdPath = wxFileName(m_projectDir, "container_id.txt").GetFullPath();

    // Check if the file exists *before* trying to open it:
    if (wxFileExists(containerIdPath))
    {
        wxFile file;
        if (file.Open(containerIdPath))
        {
            file.ReadAll(&containerId);
            containerId.Trim();
        }
        else
        {
            wxMessageBox("Container ID file exists but could not be opened!", "Error", wxICON_ERROR);
            if (nextButton)
                nextButton->Enable();
            return;
        }
    }
    else
    {
        wxMessageBox("Container ID not found!", "Error", wxICON_ERROR);
        if (nextButton)
            nextButton->Enable();
        return;
    }

    if (m_overlay)
    {
        m_overlay->Destroy();
    }
    m_overlay = new OverlayFrame(this);

    wxString command = wxString::Format("docker exec %s /create_iso.sh", containerId);
    DockerExecProcess *proc = new DockerExecProcess(this);
    long pid = wxExecute(command, wxEXEC_ASYNC | wxEXEC_HIDE_CONSOLE, proc);

    if (pid == 0)
    {
        wxMessageBox("Failed to start ISO creation process!", "Error", wxICON_ERROR);
        delete proc;
        CloseOverlay();
        if (nextButton)
            nextButton->Enable();
    }
}

void SecondWindow::OnTabChanged(wxCommandEvent &event)
{
    if (event.GetId() == ID_TERMINAL_TAB)
    {
        m_terminalTab->Show();
        m_sqlTab->Hide();
        m_mongoPanel->Hide();
        m_flatpakStore->Hide();
        wxButton *terminalButton = wxDynamicCast(FindWindow(ID_TERMINAL_TAB), wxButton);
        wxButton *sqlButton = wxDynamicCast(FindWindow(ID_SQL_TAB), wxButton);
        if (terminalButton)
        {
            terminalButton->SetBackgroundColour(wxColour(44, 49, 58));
            terminalButton->SetForegroundColour(*wxWHITE);
        }
        if (sqlButton)
        {
            sqlButton->SetBackgroundColour(wxColour(30, 30, 30));
            sqlButton->SetForegroundColour(wxColour(128, 128, 128));
        }
    }
    else if (event.GetId() == ID_SQL_TAB)
    {
        m_terminalTab->Hide();
        m_sqlTab->Show();
        m_mongoPanel->Hide();
        m_flatpakStore->Hide();
        wxButton *terminalButton = wxDynamicCast(FindWindow(ID_TERMINAL_TAB), wxButton);
        wxButton *sqlButton = wxDynamicCast(FindWindow(ID_SQL_TAB), wxButton);
        if (terminalButton)
        {
            terminalButton->SetBackgroundColour(wxColour(30, 30, 30));
            terminalButton->SetForegroundColour(wxColour(128, 128, 128));
        }
        if (sqlButton)
        {
            sqlButton->SetBackgroundColour(wxColour(44, 49, 58));
            sqlButton->SetForegroundColour(*wxWHITE);
        }
    }
    m_mainPanel->Layout();
}

void SecondWindow::OnMongoDBButton(wxCommandEvent &event)
{
    m_terminalTab->Hide();
    m_sqlTab->Hide();
    m_flatpakStore->Hide();
    m_mongoPanel->Show();
    m_mongoPanel->LoadMongoDBContent();
    m_mainPanel->Layout();
}

void SecondWindow::OnMongoDBPanelClose(wxCommandEvent &event)
{
    m_mongoPanel->Hide();
    m_terminalTab->Show();
    m_mainPanel->Layout();
}

// MongoDBPanel implementation
MongoDBPanel::MongoDBPanel(wxWindow *parent) : wxPanel(parent)
{
    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);

    wxButton *closeButton = new wxButton(this, ID_MONGODB_PANEL_CLOSE, "X",
                                         wxDefaultPosition, wxSize(30, 30));
    closeButton->SetBackgroundColour(wxColour(200, 50, 50));
    closeButton->SetForegroundColour(*wxWHITE);

    m_contentDisplay = new wxTextCtrl(this, wxID_ANY, "",
                                      wxDefaultPosition, wxDefaultSize,
                                      wxTE_MULTILINE | wxTE_READONLY);
    m_contentDisplay->SetBackgroundColour(wxColour(30, 30, 30));
    m_contentDisplay->SetForegroundColour(*wxWHITE);

    sizer->Add(closeButton, 0, wxALIGN_RIGHT | wxALL, 5);
    sizer->Add(m_contentDisplay, 1, wxEXPAND | wxALL, 5);

    SetSizer(sizer);
}

void MongoDBPanel::LoadMongoDBContent()
{
    m_contentDisplay->Clear();
    try
    {
        // Replace with your actual MongoDB Atlas connection string
        mongocxx::uri uri("mongodb+srv://WERcvbdfg32:ED6Rlo6dP1hosLvJ@cluster0.xxx.mongodb.net/?retryWrites=true&w=majority");
        mongocxx::client client(uri);
        mongocxx::database db = client["testdb"];
        mongocxx::collection coll = db["testcollection"];

        mongocxx::cursor cursor = coll.find({});

        m_contentDisplay->AppendText("MongoDB Contents:\n\n");
        for (auto &&doc : cursor)
        {
            std::string json = bsoncxx::to_json(doc);
            m_contentDisplay->AppendText(wxString(json) + "\n");
        }

        if (!m_contentDisplay->GetValue().Contains("{"))
        {
            m_contentDisplay->AppendText("No documents found in collection\n");
        }
    }
    catch (const std::exception &e)
    {
        m_contentDisplay->AppendText("Error: " + wxString(e.what()) + "\n");
        m_contentDisplay->AppendText("Check MongoDB connection string and network\n");
    }
}