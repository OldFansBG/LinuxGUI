#include "SecondWindow.h"

wxDEFINE_EVENT(PYTHON_TASK_COMPLETED, wxCommandEvent);
wxDEFINE_EVENT(PYTHON_LOG_UPDATE, wxCommandEvent);

wxBEGIN_EVENT_TABLE(SecondWindow, wxFrame)
    EVT_CLOSE(SecondWindow::OnClose)
    EVT_BUTTON(ID_TERMINAL_TAB, SecondWindow::OnTabChanged)
    EVT_BUTTON(ID_SQL_TAB, SecondWindow::OnTabChanged)
    EVT_BUTTON(ID_NEXT_BUTTON, SecondWindow::OnNext)
    EVT_COMMAND(wxID_ANY, PYTHON_TASK_COMPLETED, SecondWindow::OnPythonTaskCompleted)
    EVT_COMMAND(wxID_ANY, PYTHON_LOG_UPDATE, SecondWindow::OnPythonLogUpdate)
wxEND_EVENT_TABLE()

SecondWindow::SecondWindow(wxWindow* parent, const wxString& title, const wxString& isoPath)
    : wxFrame(parent, wxID_ANY, title, wxDefaultPosition, wxSize(800, 650)),
      m_isoPath(isoPath),
      m_workerThread(nullptr),
      m_threadRunning(false) {
    
    // Initialize Python before creating the worker thread
    Py_InitializeEx(0);  // Initialize without signal handlers
    PyEval_InitThreads(); // Initialize and acquire the GIL
    PyEval_SaveThread();  // Release the GIL

    m_containerId = ContainerManager::Get().GetCurrentContainerId();
    CreateControls();
    Centre();
    SetBackgroundColour(wxColour(40, 44, 52));

    // Create and start the Python worker thread
    StartPythonThread();
}


void SecondWindow::StartPythonThread() {
    if (m_workerThread == nullptr) {
        m_workerThread = new PythonWorkerThread(this, m_isoPath);
        if (m_workerThread->Run() == wxTHREAD_NO_ERROR) {
            m_threadRunning = true;
        } else {
            wxMessageBox("Failed to start Python task!", "Error", wxICON_ERROR);
            delete m_workerThread;
            m_workerThread = nullptr;
        }
    }
}

SecondWindow::~SecondWindow() {
    CleanupThread();
    
    if (!m_containerId.IsEmpty()) {
        ContainerManager::Get().CleanupContainer(m_containerId);
    }

    // Finalize Python in the main thread
    PyGILState_STATE gstate = PyGILState_Ensure();
    Py_FinalizeEx();
    PyGILState_Release(gstate);
}

void SecondWindow::CleanupThread() {
    if (m_workerThread && m_threadRunning) {
        m_threadRunning = false;
        
        // Signal the thread to stop and wait for it
        if (m_workerThread->Delete() != wxTHREAD_NO_ERROR) {
            m_workerThread->Wait();
        }
        
        delete m_workerThread;
        m_workerThread = nullptr;
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

    // Add your custom logic here for the "Next" button
    wxMessageBox("Next button clicked!", "Info", wxICON_INFORMATION);

    if (nextButton) nextButton->Enable();
}

void SecondWindow::OnTabChanged(wxCommandEvent& event) {
    if (event.GetId() == ID_TERMINAL_TAB) {
        m_terminalTab->Show();
        m_sqlTab->Hide();

        wxButton* terminalButton = wxDynamicCast(FindWindow(ID_TERMINAL_TAB), wxButton);
        wxButton* sqlButton = wxDynamicCast(FindWindow(ID_SQL_TAB), wxButton);
        if (terminalButton && sqlButton) {
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
        if (terminalButton && sqlButton) {
            terminalButton->SetBackgroundColour(wxColour(30, 30, 30));
            terminalButton->SetForegroundColour(wxColour(128, 128, 128));
            sqlButton->SetBackgroundColour(wxColour(44, 49, 58));
            sqlButton->SetForegroundColour(*wxWHITE);
        }
    }
    m_mainPanel->Layout();
}

void SecondWindow::OnPythonTaskCompleted(wxCommandEvent& event) {
    m_threadRunning = false;
    bool success = event.GetInt() != 0;
    wxString errorMsg = event.GetString();

    if (success) {
        wxMessageBox("Python task completed successfully!", "Success", wxOK | wxICON_INFORMATION);
    } else {
        wxMessageBox("Error: " + errorMsg, "Failure", wxOK | wxICON_ERROR);
    }
}

void SecondWindow::OnPythonLogUpdate(wxCommandEvent& event) {
    if (m_logTextCtrl) {
        wxString logMessage = event.GetString();
        m_logTextCtrl->AppendText(logMessage + "\n");
    }
}