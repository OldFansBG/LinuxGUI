#include "SecondWindow.h"
#include <wx/notebook.h>
#include <wx/statline.h>

wxBEGIN_EVENT_TABLE(SecondWindow, wxFrame)
    EVT_CLOSE(SecondWindow::OnClose)
    EVT_BUTTON(ID_NEXT_BUTTON, SecondWindow::OnNext)
    EVT_BUTTON(ID_TERMINAL_TAB, SecondWindow::OnTabChanged)
    EVT_BUTTON(ID_SQL_TAB, SecondWindow::OnTabChanged)
wxEND_EVENT_TABLE()

SecondWindow::SecondWindow(wxWindow* parent, const wxString& title, const wxString& isoPath)
    : wxFrame(parent, wxID_ANY, title, wxDefaultPosition, wxSize(800, 650)),
      m_isoPath(isoPath), m_cmdPanel(nullptr), m_terminalPanel(nullptr)
{
    CreateControls();
    Centre();
    
    // Set initial theme
    SetBackgroundColour(wxColour(40, 44, 52)); // #282C34
}

void SecondWindow::CreateControls()
{
    // Main panel
    m_mainPanel = new wxPanel(this);
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Title bar (optional, based on your needs)
    if (!HasFlag(wxFRAME_NO_TASKBAR)) {
        wxPanel* titleBar = new wxPanel(m_mainPanel);
        titleBar->SetBackgroundColour(wxColour(26, 26, 26)); // #1A1A1A
        wxBoxSizer* titleSizer = new wxBoxSizer(wxHORIZONTAL);
        wxStaticText* titleText = new wxStaticText(titleBar, wxID_ANY, "Terminal");
        titleText->SetForegroundColour(wxColour(229, 229, 229)); // #E5E5E5
        titleSizer->Add(titleText, 0, wxALL | wxALIGN_CENTER_VERTICAL, 10);
        titleBar->SetSizer(titleSizer);
        mainSizer->Add(titleBar, 0, wxEXPAND);
    }

    // Create tab buttons panel with dark background
    wxPanel* tabPanel = new wxPanel(m_mainPanel);
    tabPanel->SetBackgroundColour(wxColour(30, 30, 30)); // #1E1E1E
    wxBoxSizer* tabSizer = new wxBoxSizer(wxHORIZONTAL);

    // Create a container for centered buttons
    wxBoxSizer* centeredTabSizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Terminal tab button
    wxButton* terminalButton = new wxButton(tabPanel, ID_TERMINAL_TAB, "TERMINAL",
                                          wxDefaultPosition, wxSize(100, 40),
                                          wxBORDER_NONE);
    terminalButton->SetBackgroundColour(wxColour(44, 49, 58)); // #2C313A
    terminalButton->SetForegroundColour(*wxWHITE);

    // Vertical separator line
    wxStaticLine* separator = new wxStaticLine(tabPanel, wxID_ANY, 
                                             wxDefaultPosition, wxDefaultSize,
                                             wxLI_VERTICAL);
    separator->SetBackgroundColour(wxColour(64, 64, 64)); // #404040

    // SQL tab button
    wxButton* sqlButton = new wxButton(tabPanel, ID_SQL_TAB, "SQL",
                                     wxDefaultPosition, wxSize(100, 40),
                                     wxBORDER_NONE);
    sqlButton->SetBackgroundColour(wxColour(30, 30, 30)); // #1E1E1E
    sqlButton->SetForegroundColour(wxColour(128, 128, 128));

    // Add buttons to centered container
    centeredTabSizer->Add(terminalButton, 0, wxEXPAND);
    centeredTabSizer->Add(separator, 0, wxEXPAND | wxTOP | wxBOTTOM, 5);
    centeredTabSizer->Add(sqlButton, 0, wxEXPAND);

    // Center the buttons
    tabSizer->AddStretchSpacer(1);
    tabSizer->Add(centeredTabSizer, 0, wxALIGN_CENTER);
    tabSizer->AddStretchSpacer(1);

    tabPanel->SetSizer(tabSizer);

    // Create content panels
    m_terminalTab = new wxPanel(m_mainPanel);
    m_sqlTab = new wxPanel(m_mainPanel);
    
    m_terminalSizer = new wxBoxSizer(wxVERTICAL);
    m_sqlSizer = new wxBoxSizer(wxVERTICAL);

    // Set background colors for content panels
    m_terminalTab->SetBackgroundColour(wxColour(30, 30, 30)); // #1E1E1E
    m_sqlTab->SetBackgroundColour(wxColour(30, 30, 30)); // #1E1E1E

    // Set up terminal panel based on OS
    OSDetector::OS currentOS = m_osDetector.GetCurrentOS();
    if (currentOS == OSDetector::OS::Windows) {
        m_cmdPanel = new WindowsCmdPanel(m_terminalTab);
        m_cmdPanel->SetISOPath(m_isoPath);
        m_terminalSizer->Add(m_cmdPanel, 1, wxEXPAND | wxALL, 5);
    } else {
        m_terminalPanel = new LinuxTerminalPanel(m_terminalTab);
        m_terminalPanel->SetMinSize(wxSize(680, 400));
        m_terminalSizer->Add(m_terminalPanel, 1, wxEXPAND | wxALL, 5);
    }

    // Setup SQL panel (placeholder for now)
    wxTextCtrl* sqlPlaceholder = new wxTextCtrl(m_sqlTab, wxID_ANY, 
                                               "SQL Interface Coming Soon...",
                                               wxDefaultPosition, wxDefaultSize,
                                               wxTE_MULTILINE | wxTE_READONLY);
    sqlPlaceholder->SetBackgroundColour(wxColour(30, 30, 30));
    sqlPlaceholder->SetForegroundColour(*wxWHITE);
    m_sqlSizer->Add(sqlPlaceholder, 1, wxEXPAND | wxALL, 5);

    m_terminalTab->SetSizer(m_terminalSizer);
    m_sqlTab->SetSizer(m_sqlSizer);

    // Initially show terminal tab, hide SQL tab
    m_sqlTab->Hide();
    m_terminalTab->Show();

    // Create bottom panel for Next button
    wxPanel* buttonPanel = new wxPanel(m_mainPanel);
    buttonPanel->SetBackgroundColour(wxColour(30, 30, 30)); // #1E1E1E
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Create Next button with theme styling
    wxButton* nextButton = new wxButton(buttonPanel, ID_NEXT_BUTTON, "Next",
                                      wxDefaultPosition, wxSize(120, 35));
    nextButton->SetBackgroundColour(wxColour(189, 147, 249)); // #BD93F9
    nextButton->SetForegroundColour(*wxWHITE);

    buttonSizer->AddStretchSpacer(1);
    buttonSizer->Add(nextButton, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    buttonPanel->SetSizer(buttonSizer);

    // Add all components to main sizer
    mainSizer->Add(tabPanel, 0, wxEXPAND);
    mainSizer->Add(m_terminalTab, 1, wxEXPAND);
    mainSizer->Add(m_sqlTab, 1, wxEXPAND);
    mainSizer->Add(buttonPanel, 0, wxEXPAND);

    m_mainPanel->SetSizer(mainSizer);

    // Set minimum size for the window
    SetMinSize(wxSize(800, 600));
}

void SecondWindow::OnTabChanged(wxCommandEvent& event)
{
    // Get the buttons
    wxButton* terminalButton = wxDynamicCast(FindWindow(ID_TERMINAL_TAB), wxButton);
    wxButton* sqlButton = wxDynamicCast(FindWindow(ID_SQL_TAB), wxButton);
    
    if (event.GetId() == ID_TERMINAL_TAB) {
        // Switch to Terminal tab
        m_terminalTab->Show();
        m_sqlTab->Hide();
        
        // Update button styles
        terminalButton->SetBackgroundColour(wxColour(44, 49, 58)); // #2C313A
        terminalButton->SetForegroundColour(*wxWHITE);
        sqlButton->SetBackgroundColour(wxColour(30, 30, 30)); // #1E1E1E
        sqlButton->SetForegroundColour(wxColour(128, 128, 128));
    } else {
        // Switch to SQL tab
        m_terminalTab->Hide();
        m_sqlTab->Show();
        
        // Update button styles
        terminalButton->SetBackgroundColour(wxColour(30, 30, 30)); // #1E1E1E
        terminalButton->SetForegroundColour(wxColour(128, 128, 128));
        sqlButton->SetBackgroundColour(wxColour(44, 49, 58)); // #2C313A
        sqlButton->SetForegroundColour(*wxWHITE);
    }
    
    // Refresh the layout
    m_mainPanel->Layout();
    terminalButton->Refresh();
    sqlButton->Refresh();
}

void SecondWindow::OnNext(wxCommandEvent& event)
{
    wxString currentTab = m_terminalTab->IsShown() ? "Terminal" : "SQL";
    wxMessageBox(wxString::Format("Moving to next step from %s tab...", currentTab),
                "Next Step", wxOK | wxICON_INFORMATION);
    // Add your next step logic here
}

void SecondWindow::OnClose(wxCloseEvent& event)
{
    if (GetParent()) {
        GetParent()->Show();
    }
    Destroy();
}