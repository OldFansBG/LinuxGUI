#include "SecondWindow.h"
#include <wx/utils.h>  // For wxExecute
#include <wx/statline.h>


wxDEFINE_EVENT(PYTHON_TASK_COMPLETED, wxCommandEvent);
wxDEFINE_EVENT(PYTHON_LOG_UPDATE, wxCommandEvent);

wxBEGIN_EVENT_TABLE(SecondWindow, wxFrame)
    EVT_CLOSE(SecondWindow::OnClose)
    EVT_BUTTON(ID_TERMINAL_TAB, SecondWindow::OnTabChanged)
    EVT_BUTTON(ID_SQL_TAB, SecondWindow::OnTabChanged)
    EVT_BUTTON(ID_NEXT_BUTTON, SecondWindow::OnNext)
wxEND_EVENT_TABLE()

SecondWindow::SecondWindow(wxWindow* parent, const wxString& title, const wxString& isoPath)
    : wxFrame(parent, wxID_ANY, title, wxDefaultPosition, wxSize(800, 650)),
      m_isoPath(isoPath),
      m_threadRunning(false) {
    
    m_containerId = ContainerManager::Get().GetCurrentContainerId();
    CreateControls();
    Centre();
    SetBackgroundColour(wxColour(40, 44, 52));

    // Start the external Python executable
    StartPythonExecutable();
}

void SecondWindow::StartPythonExecutable() {
    wxString pythonExePath = "I:\\Files\\Desktop\\LinuxGUI\\dist\\tesr.exe";

    // Use wxEXEC_HIDE_CONSOLE to hide the console window
    long pid = wxExecute(pythonExePath, wxEXEC_ASYNC | wxEXEC_HIDE_CONSOLE);
    if (pid == 0) {
        wxMessageBox("Failed to start Python executable!", "Error", wxICON_ERROR);
    } else {
        m_threadRunning = true;
    }
}


SecondWindow::~SecondWindow() {
    CleanupThread();
    
    if (!m_containerId.IsEmpty()) {
        ContainerManager::Get().CleanupContainer(m_containerId);
    }
}

void SecondWindow::CleanupThread() {
    if (m_threadRunning) {
        // Here you could implement process termination if needed
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
    separator->SetBackgroundColour(wxColour(64, 64, 64));

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
    } else {
        m_terminalTab->Hide();
        m_sqlTab->Show();
    }
    m_mainPanel->Layout();
}
