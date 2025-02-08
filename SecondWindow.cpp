#include "SecondWindow.h"

wxBEGIN_EVENT_TABLE(SecondWindow, wxFrame)
    EVT_CLOSE(SecondWindow::OnClose)
    EVT_BUTTON(ID_NEXT_BUTTON, SecondWindow::OnNext)
    EVT_BUTTON(ID_TERMINAL_TAB, SecondWindow::OnTabChanged)
    EVT_BUTTON(ID_SQL_TAB, SecondWindow::OnTabChanged)
wxEND_EVENT_TABLE()

SecondWindow::SecondWindow(wxWindow* parent, const wxString& title, const wxString& isoPath)
    : wxFrame(parent, wxID_ANY, title, wxDefaultPosition, wxSize(800, 650)),
      m_isoPath(isoPath) {
    // Use ContainerManager to get container ID
    m_containerId = ContainerManager::Get().GetCurrentContainerId();

    CreateControls();
    Centre();
    SetBackgroundColour(wxColour(40, 44, 52));
}

SecondWindow::~SecondWindow() {
    // Cleanup if we haven't already
    if (!m_containerId.IsEmpty()) {
        ContainerManager::Get().CleanupContainer(m_containerId);
    }

    if (m_cmdPanel) { // Safely delete WindowsCmdPanel
        delete m_cmdPanel;
        m_cmdPanel = nullptr;
    }
}

void SecondWindow::CreateControls() {
    m_mainPanel = new wxPanel(this);
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    if (!HasFlag(wxFRAME_NO_TASKBAR)) {
        wxPanel* titleBar = new wxPanel(m_mainPanel);
        titleBar->SetBackgroundColour(wxColour(26, 26, 26));
        wxBoxSizer* titleSizer = new wxBoxSizer(wxHORIZONTAL);
        wxStaticText* titleText = new wxStaticText(titleBar, wxID_ANY, "Terminal");
        titleText->SetForegroundColour(wxColour(229, 229, 229));
        titleSizer->Add(titleText, 0, wxALL | wxALIGN_CENTER_VERTICAL, 10);
        titleBar->SetSizer(titleSizer);
        mainSizer->Add(titleBar, 0, wxEXPAND);
    }

    wxPanel* tabPanel = new wxPanel(m_mainPanel);
    tabPanel->SetBackgroundColour(wxColour(30, 30, 30));
    wxBoxSizer* tabSizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* centeredTabSizer = new wxBoxSizer(wxHORIZONTAL);

    wxButton* terminalButton = new wxButton(tabPanel, ID_TERMINAL_TAB, "TERMINAL",
                                          wxDefaultPosition, wxSize(100, 40),
                                          wxBORDER_NONE);
    terminalButton->SetBackgroundColour(wxColour(44, 49, 58));
    terminalButton->SetForegroundColour(*wxWHITE);

    wxStaticLine* separator = new wxStaticLine(tabPanel, wxID_ANY,
                                         wxDefaultPosition, wxDefaultSize,
                                         wxLI_VERTICAL);
    separator->SetBackgroundColour(wxColour(64, 64, 64));

    wxButton* sqlButton = new wxButton(tabPanel, ID_SQL_TAB, "SQL",
                                     wxDefaultPosition, wxSize(100, 40),
                                     wxBORDER_NONE);
    sqlButton->SetBackgroundColour(wxColour(30, 30, 30));
    sqlButton->SetForegroundColour(wxColour(128, 128, 128));

    centeredTabSizer->Add(terminalButton, 0, wxEXPAND);
    centeredTabSizer->Add(separator, 0, wxEXPAND | wxTOP | wxBOTTOM, 5);
    centeredTabSizer->Add(sqlButton, 0, wxEXPAND);

    tabSizer->AddStretchSpacer(1);
    tabSizer->Add(centeredTabSizer, 0, wxALIGN_CENTER);
    tabSizer->AddStretchSpacer(1);

    tabPanel->SetSizer(tabSizer);

    m_terminalTab = new wxPanel(m_mainPanel);
    m_sqlTab = new SQLTab(m_mainPanel);  // Use SQLTab instead of wxPanel

    m_terminalSizer = new wxBoxSizer(wxVERTICAL);

    m_terminalTab->SetBackgroundColour(wxColour(30, 30, 30));

    OSDetector::OS currentOS = m_osDetector.GetCurrentOS();
    if (currentOS == OSDetector::OS::Windows) {
        m_cmdPanel = new WindowsCmdPanel(m_terminalTab);
        m_terminalSizer->Add(m_cmdPanel, 1, wxEXPAND | wxALL, 5);
    } else {
        m_terminalPanel = new LinuxTerminalPanel(m_terminalTab);
        m_terminalPanel->SetMinSize(wxSize(680, 400));
        m_terminalSizer->Add(m_terminalPanel, 1, wxEXPAND | wxALL, 5);
    }

    m_terminalTab->SetSizer(m_terminalSizer);

    m_sqlTab->Hide();
    m_terminalTab->Show();

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

    mainSizer->Add(tabPanel, 0, wxEXPAND);
    mainSizer->Add(m_terminalTab, 1, wxEXPAND);
    mainSizer->Add(m_sqlTab, 1, wxEXPAND);
    mainSizer->Add(buttonPanel, 0, wxEXPAND);

    m_mainPanel->SetSizer(mainSizer);
    SetMinSize(wxSize(800, 600));
}

void SecondWindow::OnClose(wxCloseEvent& event) {
    // Container cleanup will now be handled by ContainerManager
    if (!m_containerId.IsEmpty()) {
        ContainerManager::Get().CleanupContainer(m_containerId);
    }

    if (GetParent()) {
        GetParent()->Show();
    }
    Destroy();
}

void SecondWindow::OnNext(wxCommandEvent& event) {
    // Initialize the button without any functionality
    wxLogMessage("Next button clicked (no functionality)");
}

void SecondWindow::OnTabChanged(wxCommandEvent& event) {
    if (event.GetId() == ID_TERMINAL_TAB) {
        m_terminalTab->Show();
        m_sqlTab->Hide();

        wxButton* terminalButton = wxDynamicCast(FindWindow(ID_TERMINAL_TAB), wxButton);
        wxButton* sqlButton = wxDynamicCast(FindWindow(ID_SQL_TAB), wxButton);

        if(terminalButton && sqlButton) {
            terminalButton->SetBackgroundColour(wxColour(44, 49, 58));
            terminalButton->SetForegroundColour(*wxWHITE);
            sqlButton->SetBackgroundColour(wxColour(30, 30, 30));
            sqlButton->SetForegroundColour(wxColour(128, 128, 128));
        }
    } else {
        m_terminalTab->Hide();
        m_sqlTab->Show();

        wxButton* terminalButton = wxDynamicCast(FindWindow(ID_TERMINAL_TAB), wxButton);
        wxButton* sqlButton = wxDynamicCast(FindWindow(ID_SQL_TAB), wxButton);

        if(terminalButton && sqlButton) {
            terminalButton->SetBackgroundColour(wxColour(30, 30, 30));
            terminalButton->SetForegroundColour(wxColour(128, 128, 128));
            sqlButton->SetBackgroundColour(wxColour(44, 49, 58));
            sqlButton->SetForegroundColour(*wxWHITE);
        }
    }

    m_mainPanel->Layout();
}