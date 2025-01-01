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
        m_cmdPanel->SetISOPath(m_isoPath);
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

bool SecondWindow::RetryFailedOperation(const wxString& operation, int maxAttempts) {
    for (int attempt = 1; attempt <= maxAttempts; attempt++) {
        if (ExecuteOperation(operation)) {
            return true;
        }
        wxMilliSleep(1000 * attempt); // Exponential backoff
    }
    return false;
}

void SecondWindow::OnNext(wxCommandEvent& event) {
    wxProgressDialog progress("Creating Custom ISO", "Preparing environment...", 100, this, wxPD_APP_MODAL | wxPD_AUTO_HIDE | wxPD_SMOOTH | wxPD_ELAPSED_TIME);

    // Step 1: Read the container ID from container_id.txt
    progress.Update(10, "Reading container ID...");
    wxString containerIdFilePath = "I:\\Files\\Desktop\\LinuxGUI\\build\\container_id.txt"; // Update this path
    wxLogMessage("Reading container ID from file: %s", containerIdFilePath);

    wxFile containerIdFile;
    if (!containerIdFile.Open(containerIdFilePath, wxFile::read)) {
        wxLogError("Failed to open container_id.txt.");
        wxMessageBox("Failed to read container ID. Please check if the file exists.", "Error", wxOK | wxICON_ERROR);
        return;
    }

    wxString containerId;
    if (!containerIdFile.ReadAll(&containerId)) {
        wxLogError("Failed to read container ID from file.");
        wxMessageBox("Failed to read container ID from file.", "Error", wxOK | wxICON_ERROR);
        containerIdFile.Close();
        return;
    }
    containerIdFile.Close();

    containerId = containerId.Trim().Trim(false); // Remove leading/trailing whitespace
    if (containerId.IsEmpty()) {
        wxLogError("Container ID is empty.");
        wxMessageBox("Container ID is empty.", "Error", wxOK | wxICON_ERROR);
        return;
    }
    wxLogMessage("Container ID read from file: %s", containerId);

    // Step 2: Verify the container exists and is running
    progress.Update(20, "Verifying container...");
    wxString checkContainerCmd = wxString::Format("docker ps -q --filter \"id=%s\"", containerId);
    wxArrayString containerOutput, containerErrors;
    if (wxExecute(checkContainerCmd, containerOutput, containerErrors, wxEXEC_SYNC) != 0 || containerOutput.IsEmpty()) {
        wxLogError("Container verification failed. Errors:");
        for (const auto& error : containerErrors) {
            wxLogError(" - %s", error);
        }
        wxMessageBox("Container does not exist or is not running.", "Error", wxOK | wxICON_ERROR);
        return;
    }
    wxLogMessage("Container verified and running.");

    // Step 3: Read the process ID from the file
    progress.Update(30, "Reading process ID...");
    wxString readPidCmd = wxString::Format("docker exec %s cat /root/custom_iso/squashfs-root/process_id.txt", containerId);
    wxArrayString pidOutput, pidErrors;
    if (wxExecute(readPidCmd, pidOutput, pidErrors, wxEXEC_SYNC) != 0) {
        wxLogError("Failed to read process ID. Errors:");
        for (const auto& error : pidErrors) {
            wxLogError(" - %s", error);
        }
        wxMessageBox("Failed to read process ID.", "Error", wxOK | wxICON_ERROR);
        return;
    }

    // Extract the process ID from the output
    if (pidOutput.IsEmpty()) {
        wxLogError("Process ID file is empty.");
        wxMessageBox("Process ID file is empty.", "Error", wxOK | wxICON_ERROR);
        return;
    }
    wxString processId = pidOutput[0].Trim().Trim(false); // Remove leading/trailing whitespace
    if (processId.IsEmpty()) {
        wxLogError("Invalid process ID.");
        wxMessageBox("Invalid process ID.", "Error", wxOK | wxICON_ERROR);
        return;
    }
    wxLogMessage("Process ID read successfully: %s", processId);

    // Step 4: Send the kill -HUP signal to the process (exit chroot)
    progress.Update(40, "Exiting chroot environment...");
    wxString killCmd = wxString::Format("docker exec %s kill -9 %s", containerId, processId);
    wxArrayString killOutput, killErrors;
    if (wxExecute(killCmd, killOutput, killErrors, wxEXEC_SYNC) != 0) {
        wxLogError("Failed to send kill -HUP. Errors:");
        for (const auto& error : killErrors) {
            wxLogError(" - %s", error);
        }
        wxMessageBox("Failed to exit chroot environment.", "Error", wxOK | wxICON_ERROR);
        return;
    }
    wxLogMessage("Successfully sent kill -HUP to process ID: %s", processId);

    // Step 5: Trigger Step 7 (execute create_iso.sh)
    progress.Update(50, "Creating custom ISO...");
    m_cmdPanel->ContinueInitialization(); // Trigger Step 7 in WindowsCmdPanel
}

bool SecondWindow::RunScript(const wxString& containerId, const wxString& script) {
   wxString cmd = wxString::Format("docker exec %s %s", containerId, script);
   return ExecuteOperation(cmd);
}

bool SecondWindow::ExecuteOperation(const wxString& operation) {
   wxArrayString output, errors;
   int exitCode = wxExecute(operation, output, errors, wxEXEC_SYNC);

   for(const auto& line : output) {
       LogToFile("build.log", line);
   }
   for(const auto& error : errors) {
       LogToFile("build.log", "ERROR: " + error);
   }

   return exitCode == 0;
}

void SecondWindow::LogToFile(const wxString& logPath, const wxString& message) {
   wxFile file;
   if (!file.Open(logPath, wxFile::write_append)) {
       return;
   }
   wxString logMessage = wxString::Format("[%s] %s\n",
       wxDateTime::Now().Format("%Y-%m-%d %H:%M:%S"), message);
   file.Write(logMessage);
   file.Close();
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