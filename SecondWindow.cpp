#include "SecondWindow.h"
#include <wx/utils.h>
#include <wx/statline.h>
#include <wx/process.h>
#include <wx/dcbuffer.h>
#include <cmath>
#include <chrono>

//---------------------------------------------------------------------
// Custom wxProcess subclass for Python executable
class PythonProcess : public wxProcess {
public:
    PythonProcess(SecondWindow* parent)  // Changed to accept SecondWindow*
        : wxProcess(parent),
          m_parent(parent)  // Store reference to parent window
    {
#ifdef wxPROCESS_AUTO_DELETE
        SetExtraStyle(wxPROCESS_AUTO_DELETE);
#endif
    }

    virtual void OnTerminate(int pid, int status) override {
        // Notify parent to close overlay
        if (m_parent) {
            m_parent->CloseOverlay();
            wxMessageBox("Python executable has completed.",
                        "Process Completed",
                        wxOK | wxICON_INFORMATION);
        }
#ifndef wxPROCESS_AUTO_DELETE
        delete this;
#endif
    }

private:
    SecondWindow* m_parent;  // Reference to parent window
};
//---------------------------------------------------------------------

//---------------------------------------------------------------------
// OverlayFrame implementation (adjusted for proper layering)
class OverlayFrame : public wxDialog {
public:
    OverlayFrame(wxWindow* parent)
        : wxDialog(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                   wxNO_BORDER | wxFRAME_FLOAT_ON_PARENT)  // Changed flag
    {
        SetBackgroundColour(wxColour(0, 0, 0));
        SetTransparent(200);
        SetBackgroundStyle(wxBG_STYLE_PAINT);

        m_parentWindow = parent; // Store parent window

        UpdatePositionAndSize(); // Initial positioning and sizing

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
        dc.SetBrush(wxBrush(wxColour(0, 0, 0, 230)));  // Darker overlay
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawRectangle(0, 0, size.GetWidth(), size.GetHeight());

        // Draw animation
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
    wxWindow* m_parentWindow; // Store parent window pointer
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
    : wxFrame(parent, wxID_ANY, title, wxDefaultPosition, wxSize(800, 650)),
    m_isoPath(isoPath),
    m_projectDir(projectDir),
    m_threadRunning(false),
    m_overlay(nullptr)
{
    m_containerId = ContainerManager::Get().GetCurrentContainerId();
    CreateControls();
    Centre();
    SetBackgroundColour(wxColour(40, 44, 52));

    // Show main window first before creating overlay
    Show(true);
    m_overlay = new OverlayFrame(this);

    StartPythonExecutable();
}

SecondWindow::~SecondWindow() {
    CleanupThread();
    if (!m_containerId.IsEmpty()) {
        ContainerManager::Get().CleanupContainer(m_containerId);
    }
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

    wxString command = wxString::Format("\"%s\" --project-dir \"%s\"",
                                      pythonExePath, projectDir);

    PythonProcess* proc = new PythonProcess(this);  // Pass SecondWindow* to process

    long pid = wxExecute(command, wxEXEC_ASYNC | wxEXEC_HIDE_CONSOLE, proc);

    if (pid == 0) {
        wxMessageBox("Failed to start Python executable!", "Error", wxICON_ERROR);
        delete proc;
        m_logTextCtrl->AppendText("\n[ERROR] Failed to launch script.exe\n");
        CloseOverlay();  // Close overlay on failure
    } else {
        m_threadRunning = true;
        m_logTextCtrl->AppendText(wxString::Format(
            "\n[STATUS] Started processing in project directory: %s\n",
            projectDir
        ));
    }
}

void SecondWindow::CleanupThread() {
    if (m_threadRunning) {
        // If needed, implement process termination logic here.
        m_threadRunning = false;
    }
}

void SecondWindow::CreateControls() {
    m_mainPanel = new wxPanel(this);
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Title Bar
    wxPanel* titleBar = new wxPanel(m_mainPanel);
    titleBar->SetBackgroundColour(wxColour(26, 26, 26));
    wxBoxSizer* titleSizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText* titleText = new wxStaticText(titleBar, wxID_ANY, "Terminal");
    titleText->SetForegroundColour(wxColour(229, 229, 229));
    titleSizer->Add(titleText, 0, wxALL | wxALIGN_CENTER_VERTICAL, 10);
    titleBar->SetSizer(titleSizer);
    mainSizer->Add(titleBar, 0, wxEXPAND);

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

    // Add a log text control
    m_logTextCtrl = new wxTextCtrl(m_terminalTab, ID_LOG_TEXTCTRL, wxEmptyString,
        wxDefaultPosition, wxSize(680, 150), wxTE_MULTILINE | wxTE_READONLY);
    m_logTextCtrl->SetBackgroundColour(wxColour(30, 30, 30));
    m_logTextCtrl->SetForegroundColour(wxColour(229, 229, 229));
    terminalSizer->Add(m_logTextCtrl, 0, wxEXPAND | wxALL, 5);

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

    wxMessageBox("Next button clicked!", "Info", wxICON_INFORMATION);

    if (nextButton) nextButton->Enable();
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