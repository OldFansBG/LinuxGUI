#include "MainFrame.h"
#include "ISOReader.h"
#include "SettingsDialog.h"
#include <wx/filedlg.h>
#include <wx/dirdlg.h>
#include <wx/msgdlg.h>
#include <wx/tokenzr.h>
#include <algorithm>
#include <filesystem>
#include <wx/artprov.h>
#include <wx/bmpbuttn.h>
#include "MyButton.h"
#include <wx/settings.h>
#include <wx/display.h>
#include <wx/filename.h>

BEGIN_EVENT_TABLE(MainFrame, wxFrame)
   EVT_BUTTON(ID_BROWSE_ISO, MainFrame::OnBrowseISO)
   EVT_BUTTON(ID_BROWSE_WORKDIR, MainFrame::OnBrowseWorkDir)
   EVT_BUTTON(ID_DETECT, MainFrame::OnDetect)
   EVT_BUTTON(ID_EXTRACT, MainFrame::OnExtract)
   EVT_BUTTON(ID_CANCEL, MainFrame::OnCancel)
   EVT_BUTTON(ID_SETTINGS, MainFrame::OnSettings)
END_EVENT_TABLE()

MainFrame::MainFrame(const wxString& title)
   : wxFrame() 
{
    Create(NULL, wxID_ANY, title, wxDefaultPosition, wxDefaultSize,
           wxNO_BORDER | wxCLIP_CHILDREN);

    SetBackgroundStyle(wxBG_STYLE_PAINT);

    ThemeConfig::Get().LoadThemes();
    ThemeConfig::Get().ApplyTheme(this, "light");

    #ifdef __WXMSW__
        HWND hwnd = GetHandle();
        if (hwnd) {
            LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
            style &= ~(WS_CAPTION | WS_OVERLAPPED | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
            style |= WS_THICKFRAME;
            SetWindowLongPtr(hwnd, GWL_STYLE, style);

            LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
            exStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE | WS_EX_WINDOWEDGE);
            SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle);

            BOOL value = TRUE;
            DwmSetWindowAttribute(hwnd, 19, &value, sizeof(value));  // Dark mode
            DwmSetWindowAttribute(hwnd, DWMWA_MICA_EFFECT, &value, sizeof(value));
            
            SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
                        SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
        }
    #endif

    m_titleBar = new CustomTitleBar(this);
    m_statusBar = new CustomStatusBar(this);

    if (!LoadConfig()) {
        SetStatusText("Failed to load configuration file. Using default values.");
    }

    CreateFrameControls();
    
    SetMinSize(wxSize(600, 500));
    SetSize(wxSize(600, 500));
    
    m_progressGauge->SetValue(0);
    SetStatusText("Ready");
   
    Centre();
    Layout();
}

void MainFrame::CreateFrameControls() 
{
    wxPanel* mainPanel = new wxPanel(this);
    
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->Add(CreateLogoPanel(mainPanel), 0, wxEXPAND | wxALL, 5);
    mainSizer->Add(CreateDetectionPanel(mainPanel), 0, wxEXPAND | wxALL, 5);
    mainSizer->Add(CreateProjectPanel(mainPanel), 0, wxEXPAND | wxALL, 5);
    mainSizer->Add(CreateProgressPanel(mainPanel), 0, wxEXPAND | wxALL, 5);
    
    mainPanel->SetSizer(mainSizer);

    wxBoxSizer* frameSizer = new wxBoxSizer(wxVERTICAL);
    frameSizer->Add(m_titleBar, 0, wxEXPAND);
    frameSizer->Add(mainPanel, 1, wxEXPAND);
    frameSizer->Add(m_statusBar, 0, wxEXPAND);
    SetSizer(frameSizer);
}

void MainFrame::SetStatusText(const wxString& text)
{
    if (m_statusBar) {
        m_statusBar->SetStatusText(text);
    }
}
wxPanel* MainFrame::CreateLogoPanel(wxWindow* parent) {
    wxPanel* panel = new wxPanel(parent);
    wxStaticBoxSizer* sizer = new wxStaticBoxSizer(wxVERTICAL, panel, "");
    wxPanel* headerPanel = new wxPanel(panel);
    
    wxBoxSizer* headerSizer = new wxBoxSizer(wxHORIZONTAL);
    
    m_settingsButton = new MyButton(headerPanel, ID_SETTINGS, 
                                  "I:\\Files\\Desktop\\LinuxGUI\\gear.png", 
                                  wxDefaultPosition, wxSize(32, 32));
    m_settingsButton->SetAlwaysShowButton(true);
    
    headerSizer->AddStretchSpacer();
    headerSizer->Add(m_settingsButton, 0, wxALIGN_CENTER_VERTICAL | wxALL, 10);
    
    headerPanel->SetSizer(headerSizer);
    sizer->Add(headerPanel, 0, wxEXPAND | wxALL, 5);
    
    panel->SetSizer(sizer);
    return panel;
}

wxPanel* MainFrame::CreateDetectionPanel(wxWindow* parent) {
    wxPanel* panel = new wxPanel(parent);
    wxStaticBoxSizer* sizer = new wxStaticBoxSizer(wxVERTICAL, panel, "");
    wxPanel* contentPanel = new wxPanel(panel);
    
    wxBoxSizer* isoSizer = new wxBoxSizer(wxHORIZONTAL);
    auto isoLabel = new wxStaticText(contentPanel, wxID_ANY, "ISO File:");
    isoSizer->Add(isoLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    
    m_isoPathCtrl = new wxTextCtrl(contentPanel, wxID_ANY, "", 
                                  wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE);
    isoSizer->Add(m_isoPathCtrl, 1, wxRIGHT, 5);
    
    wxButton* browseButton = new wxButton(contentPanel, ID_BROWSE_ISO, "Browse", 
                                        wxDefaultPosition, wxSize(-1, 25), wxBORDER_NONE);
    isoSizer->Add(browseButton, 0);

    wxBoxSizer* detectSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton* detectButton = new wxButton(contentPanel, ID_DETECT, "Detect", 
                                        wxDefaultPosition, wxSize(-1, 25), wxBORDER_NONE);
    detectSizer->Add(detectButton, 0, wxRIGHT, 5);
    
    auto distroLabel = new wxStaticText(contentPanel, wxID_ANY, "Detected Distribution:");
    detectSizer->Add(distroLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    
    m_distroCtrl = new wxTextCtrl(contentPanel, wxID_ANY, "", 
                                 wxDefaultPosition, wxDefaultSize, wxTE_READONLY | wxBORDER_SIMPLE);
    detectSizer->Add(m_distroCtrl, 1);

    wxBoxSizer* contentSizer = new wxBoxSizer(wxVERTICAL);
    contentSizer->Add(isoSizer, 0, wxEXPAND | wxALL, 5);
    contentSizer->Add(detectSizer, 0, wxEXPAND | wxALL, 5);
    contentPanel->SetSizer(contentSizer);
    
    sizer->Add(contentPanel, 1, wxEXPAND | wxALL, 2);
    panel->SetSizer(sizer);
    return panel;
}
wxPanel* MainFrame::CreateProjectPanel(wxWindow* parent) {
    wxPanel* panel = new wxPanel(parent);
    wxStaticBoxSizer* sizer = new wxStaticBoxSizer(wxVERTICAL, panel, "");
    wxPanel* contentPanel = new wxPanel(panel);
    
    wxFlexGridSizer* gridSizer = new wxFlexGridSizer(2, 2, 5, 5);
    gridSizer->AddGrowableCol(1, 1);

    auto nameLabel = new wxStaticText(contentPanel, wxID_ANY, "Project Name:");
    gridSizer->Add(nameLabel, 0, wxALIGN_CENTER_VERTICAL);
    
    m_projectNameCtrl = new wxTextCtrl(contentPanel, wxID_ANY, "", 
                                     wxDefaultPosition, wxSize(-1, 25), wxBORDER_SIMPLE);
    gridSizer->Add(m_projectNameCtrl, 1, wxEXPAND);

    auto versionLabel = new wxStaticText(contentPanel, wxID_ANY, "Version:");
    gridSizer->Add(versionLabel, 0, wxALIGN_CENTER_VERTICAL);
    
    m_versionCtrl = new wxTextCtrl(contentPanel, wxID_ANY, "", 
                                  wxDefaultPosition, wxSize(-1, 25), wxBORDER_SIMPLE);
    gridSizer->Add(m_versionCtrl, 1, wxEXPAND);

    wxBoxSizer* dirSizer = new wxBoxSizer(wxHORIZONTAL);
    auto dirLabel = new wxStaticText(contentPanel, wxID_ANY, "Working Directory:");
    dirSizer->Add(dirLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    
    m_workDirCtrl = new wxTextCtrl(contentPanel, wxID_ANY, "", 
                                  wxDefaultPosition, wxSize(-1, 25), wxBORDER_SIMPLE);
    dirSizer->Add(m_workDirCtrl, 1, wxRIGHT, 5);
    
    wxButton* workDirButton = new wxButton(contentPanel, ID_BROWSE_WORKDIR, "Browse", 
                                         wxDefaultPosition, wxSize(-1, 25), wxBORDER_NONE);
    dirSizer->Add(workDirButton, 0);

    wxBoxSizer* contentSizer = new wxBoxSizer(wxVERTICAL);
    contentSizer->Add(gridSizer, 0, wxEXPAND | wxALL, 5);
    contentSizer->Add(dirSizer, 0, wxEXPAND | wxALL, 5);
    contentPanel->SetSizer(contentSizer);
    
    sizer->Add(contentPanel, 1, wxEXPAND | wxALL, 2);
    panel->SetSizer(sizer);
    return panel;
}

wxPanel* MainFrame::CreateProgressPanel(wxWindow* parent) {
    wxPanel* panel = new wxPanel(parent);
    wxStaticBoxSizer* sizer = new wxStaticBoxSizer(wxVERTICAL, panel, "");
    wxPanel* contentPanel = new wxPanel(panel);
    
    wxBoxSizer* contentSizer = new wxBoxSizer(wxVERTICAL);
    
    m_progressGauge = new wxGauge(contentPanel, wxID_ANY, 100, 
                                 wxDefaultPosition, wxSize(-1, 20));
    contentSizer->Add(m_progressGauge, 0, wxEXPAND | wxALL, 5);
    
    m_statusText = new wxStaticText(contentPanel, wxID_ANY, "");
    contentSizer->Add(m_statusText, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);

    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->AddStretchSpacer();
    
    wxButton* extractButton = new wxButton(contentPanel, ID_EXTRACT, "Extract", 
                                         wxDefaultPosition, wxSize(-1, 25), wxBORDER_NONE);
    wxButton* cancelButton = new wxButton(contentPanel, ID_CANCEL, "Cancel", 
                                        wxDefaultPosition, wxSize(-1, 25), wxBORDER_NONE);
    
    buttonSizer->Add(extractButton, 0, wxRIGHT, 5);
    buttonSizer->Add(cancelButton, 0);
    
    contentSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 5);
    contentPanel->SetSizer(contentSizer);
    sizer->Add(contentPanel, 1, wxEXPAND | wxALL, 2);
    panel->SetSizer(sizer);
    return panel;
}

void MainFrame::CreateSettingsMenu() {
    wxMenu* settingsMenu = new wxMenu;
    settingsMenu->Append(wxID_ANY, "Configure Paths...");
    settingsMenu->Append(wxID_ANY, "Detection Rules...");
    settingsMenu->AppendSeparator();
    settingsMenu->Append(wxID_ANY, "Reset to Defaults");
    
    wxPoint pos = m_settingsButton->GetPosition();
    pos.y += m_settingsButton->GetSize().GetHeight();
    PopupMenu(settingsMenu, pos);
    
    delete settingsMenu;
}

void MainFrame::OpenSecondWindow() {
    SecondWindow* secondWindow = new SecondWindow(this, "Terminal", m_isoPathCtrl->GetValue());
    this->Hide();
    secondWindow->Show(true);
}
void MainFrame::OnBrowseISO(wxCommandEvent& event) {
    wxFileDialog openFileDialog(this, "Open ISO file", "", "",
                              "ISO files (*.iso)|*.iso",
                              wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (openFileDialog.ShowModal() == wxID_CANCEL)
        return;
        
    m_isoPathCtrl->SetValue(openFileDialog.GetPath());
    m_distroCtrl->SetValue("");
    m_progressGauge->SetValue(0);
    SetStatusText("ISO file selected. Click Detect to analyze.");
}

void MainFrame::OnBrowseWorkDir(wxCommandEvent& event) {
    wxDirDialog dirDialog(this, "Choose working directory", "",
                         wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
    if (dirDialog.ShowModal() == wxID_CANCEL)
        return;
    m_workDirCtrl->SetValue(dirDialog.GetPath());
}

void MainFrame::OnExtract(wxCommandEvent& event) {
    if (m_workDirCtrl->IsEmpty() || m_isoPathCtrl->IsEmpty()) {
        wxMessageBox("Please select both ISO file and working directory.", 
                    "Error", wxOK | wxICON_ERROR);
        return;
    }

    if (m_projectNameCtrl->IsEmpty()) {
        wxMessageBox("Please enter a project name.", "Error", wxOK | wxICON_ERROR);
        return;
    }

    wxString projectPath = wxFileName(m_workDirCtrl->GetValue(), 
                                    m_projectNameCtrl->GetValue()).GetFullPath();
    if (!wxFileName::Mkdir(projectPath, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL)) {
        wxMessageBox("Failed to create project directory.", 
                    "Error", wxOK | wxICON_ERROR);
        return;
    }

    m_currentExtractor = new ISOExtractor(
        m_isoPathCtrl->GetValue(), 
        projectPath,
        [this](int progress, const wxString& status) {
            CallAfter(&MainFrame::UpdateExtractionProgress, progress, status);
        }
    );

    if (m_currentExtractor->Run() != wxTHREAD_NO_ERROR) {
        wxMessageBox("Failed to start extraction thread.", 
                    "Error", wxOK | wxICON_ERROR);
        delete m_currentExtractor;
        m_currentExtractor = nullptr;
        return;
    }

    m_currentISOPath = m_isoPathCtrl->GetValue();
    FindWindow(ID_EXTRACT)->Enable(false);
    FindWindow(ID_CANCEL)->Enable(true);
}

void MainFrame::OnCancel(wxCommandEvent& event) {
    if (m_currentExtractor) {
        m_currentExtractor->RequestStop();
    }
    m_progressGauge->SetValue(0);
    m_statusText->SetLabel("Extraction cancelled");
    SetStatusText("Operation cancelled");
    
    FindWindow(ID_EXTRACT)->Enable(true);
    FindWindow(ID_CANCEL)->Enable(false);
}

void MainFrame::OnSettings(wxCommandEvent& event) {
    SettingsDialog* dialog = new SettingsDialog(this);
    if (dialog->ShowModal() == wxID_OK) {
        wxString selectedTheme = dialog->GetSelectedTheme();
        ThemeConfig::Get().SetCurrentTheme(selectedTheme);
        ThemeConfig::Get().ApplyTheme(this, selectedTheme);
        Refresh();
        Update();
    }
    dialog->Destroy();
}

void MainFrame::UpdateExtractionProgress(int progress, const wxString& status) {
    m_progressGauge->SetValue(progress);
    m_statusText->SetLabel(status);
    
    if (progress == 100) {
        FindWindow(ID_EXTRACT)->Enable(true);
        FindWindow(ID_CANCEL)->Enable(false);
        SetStatusText("Extraction completed successfully");
        
        SecondWindow* secondWindow = new SecondWindow(this, "Terminal", m_currentISOPath);
        this->Hide();
        secondWindow->Show(true);
    } else if (progress == 0) {
        FindWindow(ID_EXTRACT)->Enable(true);
        FindWindow(ID_CANCEL)->Enable(false);
    }
}
bool MainFrame::LoadConfig() {
    try {
        std::filesystem::path exePath = std::filesystem::current_path() / "config.yaml";
        if (std::filesystem::exists(exePath)) {
            m_config = YAML::LoadFile(exePath.string());
            return true;
        }

        std::filesystem::path buildPath = std::filesystem::current_path() / "build" / "config.yaml";
        if (std::filesystem::exists(buildPath)) {
            m_config = YAML::LoadFile(buildPath.string());
            return true;
        }
        return false;
    }
    catch (const YAML::Exception& e) {
        wxMessageBox("Error loading configuration: " + wxString(e.what()),
                    "Error", wxOK | wxICON_ERROR);
        return false;
    }
}

void MainFrame::OnDetect(wxCommandEvent& event) {
    wxString isoPath = m_isoPathCtrl->GetValue();
    if (isoPath.IsEmpty()) {
        wxMessageBox("Please select an ISO file first.", "Error",
                    wxOK | wxICON_ERROR);
        return;
    }

    m_distroCtrl->SetValue("Detecting...");
    m_progressGauge->SetValue(10);
    m_statusText->SetLabel("Analyzing ISO file...");
    Update();

    wxString distributionName;
    if (SearchGrubEnv(isoPath, distributionName)) {
        m_distroCtrl->SetValue(distributionName);
        m_progressGauge->SetValue(100);
        m_statusText->SetLabel("Detection completed successfully using grubenv");
        
        if (m_projectNameCtrl->IsEmpty()) {
            m_projectNameCtrl->SetValue(distributionName);
        }
        return;
    }

    wxString releaseContent;
    if (!SearchReleaseFile(isoPath, releaseContent)) {
        m_distroCtrl->SetValue("Detection failed - No identification files found");
        m_progressGauge->SetValue(0);
        m_statusText->SetLabel("Failed to find identification files");
        return;
    }

    m_progressGauge->SetValue(50);
    m_statusText->SetLabel("Reading distribution information...");
    Update();

    wxString distribution = DetectDistribution(releaseContent);
    m_distroCtrl->SetValue(distribution);
    m_progressGauge->SetValue(100);
    m_statusText->SetLabel("Detection completed successfully using release file");
    
    if (m_projectNameCtrl->IsEmpty()) {
        m_projectNameCtrl->SetValue(distribution);
    }
}
bool MainFrame::SearchReleaseFile(const wxString& isoPath, wxString& releaseContent) {
    try {
        ISOReader reader(isoPath.ToStdString());
        if (!reader.open()) return false;

        const auto& releasePaths = m_config["release_paths"];
        if (releasePaths && releasePaths.IsSequence()) {
            for (const auto& pathNode : releasePaths) {
                std::vector<char> content;
                std::string path = pathNode.as<std::string>();
                if (reader.readFile(path, content)) {
                    releaseContent = wxString(content.data(), content.size());
                    return true;
                }
            }
        }

        auto files = reader.listFiles();
        for (const auto& file : files) {
            std::string lowerFile = file;
            std::transform(lowerFile.begin(), lowerFile.end(), lowerFile.begin(), ::tolower);
            
            if (lowerFile.find("release") != std::string::npos ||
                lowerFile.find("version") != std::string::npos) {
                std::vector<char> content;
                if (reader.readFile(file, content)) {
                    releaseContent = wxString(content.data(), content.size());
                    return true;
                }
            }
        }
        return false;
    }
    catch (const std::exception& e) {
        wxMessageBox("Error reading ISO: " + wxString(e.what()), "Error", wxOK | wxICON_ERROR);
        return false;
    }
}

bool MainFrame::SearchGrubEnv(const wxString& isoPath, wxString& distributionName) {
    try {
        ISOReader reader(isoPath.ToStdString());
        if (!reader.open()) return false;

        const auto& grubenvPaths = m_config["grubenv_paths"];
        if (grubenvPaths && grubenvPaths.IsSequence()) {
            for (const auto& pathNode : grubenvPaths) {
                std::vector<char> content;
                std::string path = pathNode.as<std::string>();
                if (reader.readFile(path, content)) {
                    wxString envContent(content.data(), content.size());
                    distributionName = ExtractNameFromGrubEnv(envContent);
                    if (!distributionName.IsEmpty()) return true;
                }
            }
        }

        auto files = reader.listFiles();
        for (const auto& file : files) {
            std::string lowerFile = file;
            std::transform(lowerFile.begin(), lowerFile.end(), lowerFile.begin(), ::tolower);
            
            if (lowerFile.find("grubenv") != std::string::npos) {
                std::vector<char> content;
                if (reader.readFile(file, content)) {
                    wxString envContent(content.data(), content.size());
                    distributionName = ExtractNameFromGrubEnv(envContent);
                    if (!distributionName.IsEmpty()) return true;
                }
            }
        }
        return false;
    }
    catch (const std::exception& e) {
        wxMessageBox("Error reading ISO: " + wxString(e.what()), "Error", wxOK | wxICON_ERROR);
        return false;
    }
}

wxString MainFrame::ExtractNameFromGrubEnv(const wxString& content) {
    wxString name;
    wxString version;
    
    wxStringTokenizer tokenizer(content, "\n");
    while (tokenizer.HasMoreTokens()) {
        wxString line = tokenizer.GetNextToken().Trim(true).Trim(false);
        
        if (line.StartsWith("NAME=")) {
            name = line.Mid(5);
            name.Replace("\"", "");
        }
        else if (line.StartsWith("VERSION=")) {
            version = line.Mid(8);
            version.Replace("\"", "");
        }
    }

    if (!name.IsEmpty() && !version.IsEmpty()) {
        return name + " " + version;
    }
    else if (!name.IsEmpty()) {
        return name;
    }
    return "";
}

wxString MainFrame::DetectDistribution(const wxString& releaseContent) {
    wxString lowerContent = releaseContent.Lower();
    
    const auto& distributions = m_config["distributions"];
    if (distributions && distributions.IsSequence()) {
        for (const auto& distro : distributions) {
            if (!distro["pattern"] || !distro["name"]) continue;
            
            wxString pattern = wxString::FromUTF8(distro["pattern"].as<std::string>());
            if (lowerContent.Contains(pattern)) {
                wxString name = wxString::FromUTF8(distro["name"].as<std::string>());
                
                wxString version;
                int versionPos = lowerContent.Find("version");
                if (versionPos != wxNOT_FOUND) {
                    wxString remaining = releaseContent.Mid(versionPos + 7);
                    for (size_t i = 0; i < remaining.Length(); i++) {
                        if (isdigit(remaining[i]) || remaining[i] == '.') {
                            version += remaining[i];
                        } else if (!version.IsEmpty()) {
                            break;
                        }
                    }
                }
                return version.IsEmpty() ? name : name + " " + version;
            }
        }
    }
    return "Unknown Distribution";
}