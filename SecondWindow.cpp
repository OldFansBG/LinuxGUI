// SecondWindow.cpp
#include "SecondWindow.h"
#include <wx/utils.h>
#include <wx/statline.h>
#include <wx/process.h>
#include <wx/dcbuffer.h>
#include <wx/filename.h>
#include <wx/file.h>
#include <cmath>
#include <chrono>
#ifdef __WXMSW__
#include <windows.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

//---------------------------------------------------------------------
// Custom wxProcess subclass for Python executable
class PythonProcess : public wxProcess {
public:
    PythonProcess(SecondWindow* parent)
        : wxProcess(parent),
          m_parent(parent)
    {
#ifdef wxPROCESS_AUTO_DELETE
        SetExtraStyle(wxPROCESS_AUTO_DELETE);
#endif
    }

    virtual void OnTerminate(int pid, int status) override {
        if (m_parent) {
            m_parent->CloseOverlay();

            // Use the public getter to access m_projectDir
            wxString containerIdPath = wxFileName(m_parent->GetProjectDir(), "container_id.txt").GetFullPath();
            wxFile file;
            if (file.Open(containerIdPath)) {
                wxString containerId;
                file.ReadAll(&containerId);
                containerId.Trim();

                // Execute Docker command in the terminal
                m_parent->ExecuteDockerCommand(containerId);
            } else {
                wxMessageBox("Container ID not found!", "Error", wxICON_ERROR);
            }
        }
#ifndef wxPROCESS_AUTO_DELETE
        delete this;
#endif
    }

private:
    SecondWindow* m_parent;
};
//---------------------------------------------------------------------

//---------------------------------------------------------------------
// Custom wxProcess subclass for Docker exec command
class DockerExecProcess : public wxProcess {
public:
    DockerExecProcess(SecondWindow* parent)
        : wxProcess(parent),
          m_parent(parent)
    {
#ifdef wxPROCESS_AUTO_DELETE
        SetExtraStyle(wxPROCESS_AUTO_DELETE);
#endif
    }

    virtual void OnTerminate(int pid, int status) override {
        if (m_parent) {
            m_parent->CloseOverlay();
            if (status == 0) {
                wxMessageBox("ISO created successfully!", "Success", wxOK | wxICON_INFORMATION);
            } else {
                wxMessageBox("ISO creation failed. Check logs for details.", "Error", wxICON_ERROR);
            }
        }
#ifndef wxPROCESS_AUTO_DELETE
        delete this;
#endif
    }

private:
    SecondWindow* m_parent;
};
//---------------------------------------------------------------------

//---------------------------------------------------------------------
// OverlayFrame implementation
class OverlayFrame : public wxDialog {
public:
    OverlayFrame(wxWindow* parent)
        : wxDialog(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                   wxNO_BORDER | wxFRAME_FLOAT_ON_PARENT)
    {
        SetBackgroundColour(wxColour(0, 0, 0));
        SetTransparent(200);
        SetBackgroundStyle(wxBG_STYLE_PAINT);

        m_parentWindow = parent;

        UpdatePositionAndSize();

        m_animationAngle = 0.0;

        Bind(wxEVT_PAINT, &OverlayFrame::OnPaint, this);
        Bind(wxEVT_TIMER, &OverlayFrame::OnTimer, this);

        if (parent) {
            parent->Bind(wxEVT_MOVE, &OverlayFrame::OnParentMoveOrResize, this);
            parent->Bind(wxEVT_SIZE, &OverlayFrame::OnParentMoveOrResize, this);
        }

        m_timer.SetOwner(this);
        m_timer.Start(30);
        Show(true);
    }

    ~OverlayFrame() {
        if (m_parentWindow) {
            m_parentWindow->Unbind(wxEVT_MOVE, &OverlayFrame::OnParentMoveOrResize, this);
            m_parentWindow->Unbind(wxEVT_SIZE, &OverlayFrame::OnParentMoveOrResize, this);
        }
        m_timer.Stop();
    }

private:
    void UpdatePositionAndSize() {
        if (m_parentWindow) {
            wxRect clientRect = m_parentWindow->GetClientRect();
            SetPosition(m_parentWindow->ClientToScreen(clientRect.GetTopLeft()));
            SetSize(clientRect.GetSize());
        }
    }

    void OnParentMoveOrResize(wxEvent& event) {
        UpdatePositionAndSize();
    }

    void OnPaint(wxPaintEvent& event) {
        wxAutoBufferedPaintDC dc(this);
        dc.Clear();

        wxSize size = GetClientSize();

        // Dim background (90% opacity)
        dc.SetBrush(wxBrush(wxColour(0, 0, 0, 230)));
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawRectangle(0, 0, size.GetWidth(), size.GetHeight());

        DrawLoadingAnimation(dc, size);
    }

    void DrawLoadingAnimation(wxDC& dc, const wxSize& size) {
        wxPoint center(size.GetWidth()/2, size.GetHeight()/2);
        int radius = std::min(size.GetWidth(), size.GetHeight()) / 6;
        int numPoints = 8;
        int pointRadius = radius / 4;

        dc.SetBrush(*wxWHITE_BRUSH);
        dc.SetPen(*wxWHITE_PEN);

        for (int i = 0; i < numPoints; ++i) {
            double angle = m_animationAngle + (2.0 * M_PI * i / numPoints);
            int x = center.x + static_cast<int>(radius * cos(angle));
            int y = center.y + static_cast<int>(radius * sin(angle));
            dc.DrawCircle(x, y, pointRadius);
        }
    }

    void OnTimer(wxTimerEvent& event) {
        m_animationAngle += 0.1;
        if (m_animationAngle > 2.0 * M_PI) m_animationAngle -= 2.0 * M_PI;
        Refresh();
    }

    double m_animationAngle;
    wxTimer m_timer;
    wxWindow* m_parentWindow;
};
//---------------------------------------------------------------------

wxDEFINE_EVENT(PYTHON_TASK_COMPLETED, wxCommandEvent);
wxDEFINE_EVENT(PYTHON_LOG_UPDATE, wxCommandEvent);

wxBEGIN_EVENT_TABLE(SecondWindow, wxFrame)
    EVT_CLOSE(SecondWindow::OnClose)
    EVT_BUTTON(ID_TERMINAL_TAB, SecondWindow::OnTabChanged)
    EVT_BUTTON(ID_SQL_TAB, SecondWindow::OnTabChanged)
    EVT_BUTTON(ID_NEXT_BUTTON, SecondWindow::OnNext)
wxEND_EVENT_TABLE()

SecondWindow::SecondWindow(wxWindow* parent,
        const wxString& title,
        const wxString& isoPath,
        const wxString& projectDir)
    : wxFrame(parent, wxID_ANY, title, 
            wxDefaultPosition, wxSize(800, 650),
            wxDEFAULT_FRAME_STYLE | wxSYSTEM_MENU | wxCAPTION | 
            wxCLOSE_BOX | wxCLIP_CHILDREN | wxMINIMIZE_BOX | 
            wxMAXIMIZE_BOX | wxRESIZE_BORDER),
    m_isoPath(isoPath),
    m_projectDir(projectDir),
    m_threadRunning(false),
    m_overlay(nullptr)
#ifdef __WXMSW__
    , m_winTerminalManager(nullptr)
#endif
{
    // Enable dark mode title bar
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

    // Initialize the terminal with a clear command (optional)
#ifdef __WXMSW__
    if (m_winTerminalManager) {
        m_winTerminalManager->SendCommand(L" ");
    }
#endif
}

SecondWindow::~SecondWindow() {
    CleanupThread();
    if (!m_containerId.IsEmpty()) {
        ContainerManager::Get().CleanupContainer(m_containerId);
    }
#ifdef __WXMSW__
    delete m_winTerminalManager;
#endif
}

void SecondWindow::CloseOverlay() {
    if (m_overlay) {
        m_overlay->Destroy();
        m_overlay = nullptr;
    }
}

void SecondWindow::StartPythonExecutable() {
    wxString pythonExePath = "script.exe";
    wxString projectDir = m_projectDir;
    wxString isoPath = m_isoPath;

    wxString command = wxString::Format("\"%s\" --project-dir \"%s\" --iso-path \"%s\"",
                                      pythonExePath, projectDir, isoPath);

    PythonProcess* proc = new PythonProcess(this);

    long pid = wxExecute(command, wxEXEC_ASYNC | wxEXEC_HIDE_CONSOLE, proc);

    if (pid == 0) {
        wxMessageBox("Failed to start Python executable!", "Error", wxICON_ERROR);
        delete proc;
        CloseOverlay();
    } else {
        m_threadRunning = true;
    }
}

void SecondWindow::CleanupThread() {
    if (m_threadRunning) {
        m_threadRunning = false;
    }
}

void SecondWindow::ExecuteDockerCommand(const wxString& containerId) {
    #ifdef __WXMSW__
        if (m_winTerminalManager) {
            // Enter the container FIRST, then run chroot interactively
            wxString command = wxString::Format(
                "cls && docker exec -it %s /bin/bash\r\n"
                "chroot /root/custom_iso/squashfs-root /bin/bash\r\n",
                containerId
            );
            m_winTerminalManager->SendCommand(command.ToStdWstring());
        }
    #endif
}

void SecondWindow::CreateControls() {
    m_mainPanel = new wxPanel(this);
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Tab Bar
    wxPanel* tabPanel = new wxPanel(m_mainPanel);
    tabPanel->SetBackgroundColour(wxColour(30, 30, 30));
    wxBoxSizer* tabSizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* centeredTabSizer = new wxBoxSizer(wxHORIZONTAL);

    wxButton* terminalButton = new wxButton(tabPanel, ID_TERMINAL_TAB, "TERMINAL",
        wxDefaultPosition, wxSize(100, 40), wxBORDER_NONE);
    terminalButton->SetBackgroundColour(wxColour(44, 49, 58));
    terminalButton->SetForegroundColour(*wxWHITE);

    wxStaticLine* separator = new wxStaticLine(tabPanel, wxID_ANY,
        wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL);

    wxButton* sqlButton = new wxButton(tabPanel, ID_SQL_TAB, "SQL",
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

    // Terminal/SQL Content
    m_terminalTab = new wxPanel(m_mainPanel);
    m_sqlTab = new SQLTab(m_mainPanel);

    wxBoxSizer* terminalSizer = new wxBoxSizer(wxVERTICAL);
    m_terminalTab->SetBackgroundColour(wxColour(30, 30, 30));

    OSDetector::OS currentOS = m_osDetector.GetCurrentOS();
    if (currentOS == OSDetector::OS::Linux) {
        m_terminalPanel = new LinuxTerminalPanel(m_terminalTab);
        m_terminalPanel->SetMinSize(wxSize(680, 400));
        terminalSizer->Add(m_terminalPanel, 1, wxEXPAND | wxALL, 5);
    }
#ifdef __WXMSW__
    else {
        wxPanel* winTerminalPanel = new wxPanel(m_terminalTab, wxID_ANY, 
            wxDefaultPosition, wxSize(680, 400));
        winTerminalPanel->SetBackgroundColour(wxColour(30, 30, 30));
        terminalSizer->Add(winTerminalPanel, 1, wxEXPAND | wxALL, 5);
        
        m_winTerminalManager = new WinTerminalManager(winTerminalPanel);
        if (!m_winTerminalManager->Initialize()) {
            wxLogError("Failed to initialize Windows terminal");
        }
    }
#endif

    m_terminalTab->SetSizer(terminalSizer);
    m_sqlTab->Hide();

    // Bottom Button Panel
    wxPanel* buttonPanel = new wxPanel(m_mainPanel);
    buttonPanel->SetBackgroundColour(wxColour(30, 30, 30));
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);

    wxButton* nextButton = new wxButton(buttonPanel, ID_NEXT_BUTTON, "Next",
        wxDefaultPosition, wxSize(120, 35));
    nextButton->SetBackgroundColour(wxColour(189, 147, 249));
    nextButton->SetForegroundColour(*wxWHITE);

    buttonSizer->AddStretchSpacer(1);
    buttonSizer->Add(nextButton, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    buttonPanel->SetSizer(buttonSizer);

    // Assemble Main Layout
    mainSizer->Add(tabPanel, 0, wxEXPAND);
    mainSizer->Add(m_terminalTab, 1, wxEXPAND);
    mainSizer->Add(m_sqlTab, 1, wxEXPAND);
    mainSizer->Add(buttonPanel, 0, wxEXPAND);

    m_mainPanel->SetSizer(mainSizer);
    SetMinSize(wxSize(800, 600));
}

void SecondWindow::OnClose(wxCloseEvent& event) {
    CleanupThread();

    if (!m_containerId.IsEmpty()) {
        ContainerManager::Get().CleanupContainer(m_containerId);
    }

    if (GetParent()) {
        GetParent()->Show();
    }

    Destroy();
}

void SecondWindow::OnNext(wxCommandEvent& event) {
    wxButton* nextButton = wxDynamicCast(FindWindow(ID_NEXT_BUTTON), wxButton);
    if (nextButton) nextButton->Disable();

    // Read container ID from container_id.txt
    wxString containerId;
    wxString containerIdPath = wxFileName(m_projectDir, "container_id.txt").GetFullPath();
    wxFile file;
    if (file.Open(containerIdPath)) {
        file.ReadAll(&containerId);
        containerId.Trim();
    } else {
        wxMessageBox("Container ID not found!", "Error", wxICON_ERROR);
        if (nextButton) nextButton->Enable();
        return;
    }

    // Show processing overlay
    if (m_overlay) {
        m_overlay->Destroy();
    }
    m_overlay = new OverlayFrame(this);

    // Execute docker exec command
    wxString command = wxString::Format("docker exec %s /create_iso.sh", containerId);
    DockerExecProcess* proc = new DockerExecProcess(this);
    long pid = wxExecute(command, wxEXEC_ASYNC | wxEXEC_HIDE_CONSOLE, proc);

    if (pid == 0) {
        wxMessageBox("Failed to start ISO creation process!", "Error", wxICON_ERROR);
        delete proc;
        CloseOverlay();
        if (nextButton) nextButton->Enable();
    }
}

void SecondWindow::OnTabChanged(wxCommandEvent& event) {
    if (event.GetId() == ID_TERMINAL_TAB) {
        m_terminalTab->Show();
        m_sqlTab->Hide();
        wxButton* terminalButton = wxDynamicCast(FindWindow(ID_TERMINAL_TAB), wxButton);
        wxButton* sqlButton = wxDynamicCast(FindWindow(ID_SQL_TAB), wxButton);
        if (terminalButton) {
            terminalButton->SetBackgroundColour(wxColour(44, 49, 58));
            terminalButton->SetForegroundColour(*wxWHITE);
        }
        if (sqlButton) {
            sqlButton->SetBackgroundColour(wxColour(30, 30, 30));
            sqlButton->SetForegroundColour(wxColour(128, 128, 128));
        }
    } else if (event.GetId() == ID_SQL_TAB) {
        m_terminalTab->Hide();
        m_sqlTab->Show();
        wxButton* terminalButton = wxDynamicCast(FindWindow(ID_TERMINAL_TAB), wxButton);
        wxButton* sqlButton = wxDynamicCast(FindWindow(ID_SQL_TAB), wxButton);
        if (terminalButton) {
            terminalButton->SetBackgroundColour(wxColour(30, 30, 30));
            terminalButton->SetForegroundColour(wxColour(128, 128, 128));
        }
        if (sqlButton) {
            sqlButton->SetBackgroundColour(wxColour(44, 49, 58));
            sqlButton->SetForegroundColour(*wxWHITE);
        }
    }
    m_mainPanel->Layout();
}