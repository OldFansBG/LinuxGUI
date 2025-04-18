#include "MainFrame.h"
#include "ISOReader.h"
#include "SettingsDialog.h"
#include <wx/filedlg.h>
#include <wx/dirdlg.h>
#include <wx/msgdlg.h>
#include <wx/tokenzr.h>
#include <algorithm>
#include "FilesystemSelectionDialog.h"
#include <filesystem>
#include <wx/artprov.h>
#include <wx/bmpbuttn.h>
#include <wx/settings.h>
#include <wx/display.h>
#include <wx/filename.h>
#include "SystemTheme.h"
#include "DesktopTab.h"
#include <wx/textfile.h> // Add this line

BEGIN_EVENT_TABLE(MainFrame, wxFrame)
EVT_BUTTON(ID_BROWSE_ISO, MainFrame::OnBrowseISO)
EVT_BUTTON(ID_BROWSE_WORKDIR, MainFrame::OnBrowseWorkDir)
EVT_BUTTON(ID_DETECT, MainFrame::OnDetect)
EVT_BUTTON(ID_NEXT, MainFrame::OnNextButton) // Changed from ID_EXTRACT
EVT_BUTTON(ID_CANCEL, MainFrame::OnCancel)
EVT_BUTTON(ID_SETTINGS, MainFrame::OnSettings)
END_EVENT_TABLE()

MainFrame::MainFrame(const wxString &title)
    : wxFrame()
{
    Create(NULL, wxID_ANY, title, wxDefaultPosition, wxDefaultSize,
           wxCAPTION | wxCLOSE_BOX | wxMINIMIZE_BOX | wxSYSTEM_MENU | wxNO_BORDER | wxCLIP_CHILDREN);

    // Load and set the application icon
    wxIcon icon;
    icon.LoadFile(wxT("IDI_MAIN_ICON"), wxBITMAP_TYPE_ICO_RESOURCE);
    if (icon.IsOk())
    {
        SetIcon(icon);
    }
    else
    {
        wxLogWarning("Failed to load application icon 'linuxisopro.ico'. Check if the file is in the correct directory.");
    }

    SetBackgroundStyle(wxBG_STYLE_PAINT);

    // Load the themes first
    ThemeConfig &themeConfig = ThemeConfig::Get();
    themeConfig.LoadThemes("themes.json");

    // Register for system theme changes
    SystemTheme::RegisterForThemeChanges(this);

    // Register this window with ThemeConfig
    themeConfig.RegisterWindow(this);

#ifdef __WXMSW__
    HWND hwnd = GetHandle();
    if (hwnd)
    {
        LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
        style &= ~(WS_CAPTION | WS_OVERLAPPED | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
        style &= ~WS_THICKFRAME;
        SetWindowLongPtr(hwnd, GWL_STYLE, style);

        LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
        exStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE | WS_EX_WINDOWEDGE);
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle);

        BOOL value = TRUE;
        DwmSetWindowAttribute(hwnd, 19, &value, sizeof(value)); // Dark mode
        DwmSetWindowAttribute(hwnd, DWMWA_MICA_EFFECT, &value, sizeof(value));

        SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
                     SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
    }
#endif

    m_titleBar = new CustomTitleBar(this);
    m_statusBar = new CustomStatusBar(this);

    if (!LoadConfig())
    {
        SetStatusText("Failed to load configuration file. Using default values.");
    }

    CreateFrameControls();

    // Initialize with system theme
    themeConfig.InitializeWithSystemTheme();

    SetMinSize(wxSize(600, 500));
    SetSize(wxSize(600, 500));

    m_progressGauge->SetValue(0);
    SetStatusText("Ready");

    Centre();
    Layout();

    // Create DesktopTab instance
    m_desktopTab = new DesktopTab(this);

    // Bind custom event to handler
    Bind(FILE_COPY_COMPLETE_EVENT, &MainFrame::OnGUIDetected, this);
}

MainFrame::~MainFrame()
{
    // Unregister from ThemeConfig when window is destroyed
    ThemeConfig::Get().UnregisterWindow(this);
}

void MainFrame::CreateFrameControls()
{
    wxPanel *mainPanel = new wxPanel(this);

    wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->Add(CreateLogoPanel(mainPanel), 0, wxEXPAND | wxALL, 5);
    mainSizer->Add(CreateDetectionPanel(mainPanel), 0, wxEXPAND | wxALL, 5);
    mainSizer->Add(CreateProjectPanel(mainPanel), 0, wxEXPAND | wxALL, 5);
    mainSizer->Add(CreateProgressPanel(mainPanel), 0, wxEXPAND | wxALL, 5);

    mainPanel->SetSizer(mainSizer);

    wxBoxSizer *frameSizer = new wxBoxSizer(wxVERTICAL);
    frameSizer->Add(m_titleBar, 0, wxEXPAND);
    frameSizer->Add(mainPanel, 1, wxEXPAND);
    frameSizer->Add(m_statusBar, 0, wxEXPAND);
    SetSizer(frameSizer);
}

void MainFrame::SetStatusText(const wxString &text)
{
    if (m_statusBar)
    {
        m_statusBar->SetStatusText(text);
    }
}

wxPanel *MainFrame::CreateLogoPanel(wxWindow *parent)
{
    wxPanel *panel = new wxPanel(parent);
    wxStaticBoxSizer *sizer = new wxStaticBoxSizer(wxVERTICAL, panel, "");
    wxPanel *headerPanel = new wxPanel(panel);

    wxBoxSizer *headerSizer = new wxBoxSizer(wxHORIZONTAL);

    m_settingsButton = new MyButton(headerPanel, ID_SETTINGS,
                                    "gear.png",
                                    wxDefaultPosition, wxSize(32, 32));
    m_settingsButton->SetAlwaysShowButton(true);

    headerSizer->AddStretchSpacer();
    headerSizer->Add(m_settingsButton, 0, wxALIGN_CENTER_VERTICAL | wxALL, 10);

    headerPanel->SetSizer(headerSizer);
    sizer->Add(headerPanel, 0, wxEXPAND | wxALL, 5);

    panel->SetSizer(sizer);
    return panel;
}

wxPanel *MainFrame::CreateDetectionPanel(wxWindow *parent)
{
    wxPanel *panel = new wxPanel(parent);
    wxStaticBoxSizer *sizer = new wxStaticBoxSizer(wxVERTICAL, panel, "");
    wxPanel *contentPanel = new wxPanel(panel);

    wxBoxSizer *isoSizer = new wxBoxSizer(wxHORIZONTAL);
    auto isoLabel = new wxStaticText(contentPanel, wxID_ANY, "ISO File:");
    isoSizer->Add(isoLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);

    m_isoPathCtrl = new wxTextCtrl(contentPanel, wxID_ANY, "",
                                   wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE);
    isoSizer->Add(m_isoPathCtrl, 1, wxRIGHT, 5);

    wxButton *browseButton = new wxButton(contentPanel, ID_BROWSE_ISO, "Browse",
                                          wxDefaultPosition, wxSize(-1, 25), wxBORDER_NONE);
    isoSizer->Add(browseButton, 0);

    wxBoxSizer *detectSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton *detectButton = new wxButton(contentPanel, ID_DETECT, "Detect",
                                          wxDefaultPosition, wxSize(-1, 25), wxBORDER_NONE);
    detectSizer->Add(detectButton, 0, wxRIGHT, 5);

    auto distroLabel = new wxStaticText(contentPanel, wxID_ANY, "Detected Distribution:");
    detectSizer->Add(distroLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);

    m_distroCtrl = new wxTextCtrl(contentPanel, wxID_ANY, "",
                                  wxDefaultPosition, wxDefaultSize, wxTE_READONLY | wxBORDER_SIMPLE);
    detectSizer->Add(m_distroCtrl, 1);

    wxBoxSizer *contentSizer = new wxBoxSizer(wxVERTICAL);
    contentSizer->Add(isoSizer, 0, wxEXPAND | wxALL, 5);
    contentSizer->Add(detectSizer, 0, wxEXPAND | wxALL, 5);
    contentPanel->SetSizer(contentSizer);

    sizer->Add(contentPanel, 1, wxEXPAND | wxALL, 2);
    panel->SetSizer(sizer);
    return panel;
}

wxPanel *MainFrame::CreateProjectPanel(wxWindow *parent)
{
    wxPanel *panel = new wxPanel(parent);
    wxStaticBoxSizer *sizer = new wxStaticBoxSizer(wxVERTICAL, panel, "");
    wxPanel *contentPanel = new wxPanel(panel);

    wxFlexGridSizer *gridSizer = new wxFlexGridSizer(2, 2, 5, 5);
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

    wxBoxSizer *dirSizer = new wxBoxSizer(wxHORIZONTAL);
    auto dirLabel = new wxStaticText(contentPanel, wxID_ANY, "Working Directory:");
    dirSizer->Add(dirLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);

    m_workDirCtrl = new wxTextCtrl(contentPanel, wxID_ANY, "",
                                   wxDefaultPosition, wxSize(-1, 25), wxBORDER_SIMPLE);
    dirSizer->Add(m_workDirCtrl, 1, wxRIGHT, 5);

    wxButton *workDirButton = new wxButton(contentPanel, ID_BROWSE_WORKDIR, "Browse",
                                           wxDefaultPosition, wxSize(-1, 25), wxBORDER_NONE);
    dirSizer->Add(workDirButton, 0);

    wxBoxSizer *contentSizer = new wxBoxSizer(wxVERTICAL);
    contentSizer->Add(gridSizer, 0, wxEXPAND | wxALL, 5);
    contentSizer->Add(dirSizer, 0, wxEXPAND | wxALL, 5);
    contentPanel->SetSizer(contentSizer);

    sizer->Add(contentPanel, 1, wxEXPAND | wxALL, 2);
    panel->SetSizer(sizer);
    return panel;
}

wxPanel *MainFrame::CreateProgressPanel(wxWindow *parent)
{
    wxPanel *panel = new wxPanel(parent);
    wxStaticBoxSizer *sizer = new wxStaticBoxSizer(wxVERTICAL, panel, "");
    wxPanel *contentPanel = new wxPanel(panel);

    wxBoxSizer *contentSizer = new wxBoxSizer(wxVERTICAL);

    m_progressGauge = new wxGauge(contentPanel, wxID_ANY, 100,
                                  wxDefaultPosition, wxSize(-1, 20));
    contentSizer->Add(m_progressGauge, 0, wxEXPAND | wxALL, 5);

    m_statusText = new wxStaticText(contentPanel, wxID_ANY, "");
    contentSizer->Add(m_statusText, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);

    wxBoxSizer *buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->AddStretchSpacer();

    wxButton *nextButton = new wxButton(contentPanel, ID_NEXT, "Next", // Changed from "Extract"
                                        wxDefaultPosition, wxSize(-1, 25), wxBORDER_NONE);
    wxButton *cancelButton = new wxButton(contentPanel, ID_CANCEL, "Cancel",
                                          wxDefaultPosition, wxSize(-1, 25), wxBORDER_NONE);

    buttonSizer->Add(nextButton, 0, wxRIGHT, 5);
    buttonSizer->Add(cancelButton, 0);

    contentSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 5);
    contentPanel->SetSizer(contentSizer);
    sizer->Add(contentPanel, 1, wxEXPAND | wxALL, 2);
    panel->SetSizer(sizer);
    return panel;
}

void MainFrame::OpenSecondWindow()
{
    SecondWindow *secondWindow = new SecondWindow(this, "Analyzer",
                                                  m_isoPathCtrl->GetValue(),
                                                  m_workDirCtrl->GetValue(),
                                                  m_desktopTab); // Add this as the 5th parameter
    this->Hide();
    secondWindow->Show(true);
}

void MainFrame::OnBrowseISO(wxCommandEvent &event)
{
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

void MainFrame::OnBrowseWorkDir(wxCommandEvent &event)
{
    wxDirDialog dirDialog(this, "Choose working directory", "",
                          wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
    if (dirDialog.ShowModal() == wxID_CANCEL)
        return;
    m_workDirCtrl->SetValue(dirDialog.GetPath());
}

void MainFrame::OnNextButton(wxCommandEvent &event)
{
    // First save settings
    AppSettings settings;
    settings.isoPath = m_isoPathCtrl->GetValue();
    settings.workDir = m_workDirCtrl->GetValue();

    // Remove spaces from the project name before saving
    wxString projectName = m_projectNameCtrl->GetValue();
    projectName.Replace(" ", ""); // Replace spaces with empty string
    settings.projectName = projectName;

    settings.version = m_versionCtrl->GetValue();
    settings.detectedDistro = m_distroCtrl->GetValue();

    wxString configPath = m_workDirCtrl->GetValue() + wxFILE_SEP_PATH + "settings.json";

    if (!m_settingsManager.SaveSettings(settings, configPath))
    {
        wxMessageBox("Failed to save settings!", "Error", wxOK | wxICON_ERROR);
        return; // Stop if save fails
    }

    // Then perform original extraction logic
    if (m_workDirCtrl->IsEmpty() || m_isoPathCtrl->IsEmpty())
    {
        wxMessageBox("Please select both ISO file and working directory.",
                     "Error", wxOK | wxICON_ERROR);
        return;
    }

    if (m_projectNameCtrl->IsEmpty())
    {
        wxMessageBox("Please enter a project name.", "Error", wxOK | wxICON_ERROR);
        return;
    }

    // Search for SquashFS files in the ISO
    wxArrayString squashfsFiles;
    if (!SearchSquashFS(m_isoPathCtrl->GetValue(), squashfsFiles))
    {
        wxMessageBox("No SquashFS/SFS files found in the ISO.\nThe ISO might not be a Linux distribution or might be corrupted.",
                     "Error", wxOK | wxICON_ERROR);
        return;
    }

    // Too many squashfs files - present user with selection dialog
    wxString selectedFsFile;
    if (squashfsFiles.GetCount() > 1)
    {
        wxArrayString prioritizedFiles;
        wxArrayString descriptions;

        // Create prioritized list with descriptions
        for (const wxString &file : squashfsFiles)
        {
            wxString lowerFile = file.Lower();
            wxString description;

            // Add descriptions to help user identify files
            if (lowerFile.Contains("airootfs.sfs"))
                description = " (Arch Linux main filesystem)";
            else if (lowerFile.Contains("minimal.standard") && !lowerFile.Contains("enhanced") && !lowerFile.Contains("languages"))
                description = " (Ubuntu standard filesystem)";
            else if (lowerFile.Contains("rootfs"))
                description = " (Root filesystem)";

            prioritizedFiles.Add(file + description);
        }

        // Show custom selection dialog
        FilesystemSelectionDialog dialog(this,
                                         "Multiple filesystem files found. Please select the one to use:",
                                         prioritizedFiles);

        if (dialog.ShowModal() == wxID_OK)
        {
            selectedFsFile = dialog.GetSelectedFilesystem(); // Directly get the sanitized filename.
        }
        else
        {
            // User canceled
            return;
        }
    }
    else if (squashfsFiles.GetCount() == 1)
    {
        // Just one file, use it directly
        selectedFsFile = squashfsFiles[0];
    }

    // Save the selected filesystem file path
    wxString fsFilePath = m_workDirCtrl->GetValue() + wxFILE_SEP_PATH + projectName + "_selected_fs.txt";
    wxTextFile file(fsFilePath);

    if (selectedFsFile.StartsWith("/"))
    {
        selectedFsFile = selectedFsFile.Mid(1); // Remove leading slash
    }

    if (file.Exists())
        file.Open();
    else
        file.Create();

    file.Clear();
    file.AddLine(selectedFsFile);

    if (!file.Write())
    {
        wxMessageBox("Failed to save selected filesystem file path!", "Warning", wxOK | wxICON_WARNING);
    }
    file.Close();

    // Log what was found and selected
    wxString message = wxString::Format("Found %zu filesystem files. Selected: %s",
                                        squashfsFiles.GetCount(), selectedFsFile);
    SetStatusText(message);

    // Store current ISO path and proceed
    m_currentISOPath = m_isoPathCtrl->GetValue();

    // Open second window
    SecondWindow *secondWindow = new SecondWindow(this, "Analyzer",
                                                  m_currentISOPath,
                                                  m_workDirCtrl->GetValue(),
                                                  m_desktopTab); // Add this
    this->Hide();
    secondWindow->Show(true);
}

void MainFrame::OnCancel(wxCommandEvent &event)
{
    wxWindowList &windows = wxTopLevelWindows;
    for (wxWindowList::iterator it = windows.begin(); it != windows.end(); ++it)
    {
        (*it)->Close(true);
    }
}

void MainFrame::OnSettings(wxCommandEvent &event)
{
    SettingsDialog *dialog = new SettingsDialog(this);
    if (dialog->ShowModal() == wxID_OK)
    {
        wxString selectedTheme = dialog->GetSelectedTheme();
        ThemeConfig::Get().LoadThemes(); // Reload themes
        ThemeConfig::Get().SetCurrentTheme(selectedTheme);
        ThemeConfig::Get().ApplyTheme(this, selectedTheme);
        Layout();
        Refresh(true);
        Update();
    }
    dialog->Destroy();
}

bool MainFrame::LoadConfig()
{
    try
    {
        std::filesystem::path exePath = std::filesystem::current_path() / "config.yaml";
        if (std::filesystem::exists(exePath))
        {
            m_config = YAML::LoadFile(exePath.string());
            return true;
        }

        std::filesystem::path buildPath = std::filesystem::current_path() / "build" / "config.yaml";
        if (std::filesystem::exists(buildPath))
        {
            m_config = YAML::LoadFile(buildPath.string());
            return true;
        }
        return false;
    }
    catch (const YAML::Exception &e)
    {
        wxMessageBox("Error loading configuration: " + wxString(e.what()),
                     "Error", wxOK | wxICON_ERROR);
        return false;
    }
}

bool MainFrame::SearchSquashFS(const wxString &isoPath, wxArrayString &foundFiles)
{
    try
    {
        ISOReader reader(isoPath.ToStdString());
        if (!reader.open())
        {
            wxMessageBox("Failed to open ISO file: " + wxString(reader.getLastError()),
                         "Error", wxOK | wxICON_ERROR);
            return false;
        }

        auto files = reader.listFiles();
        bool found = false;

        for (const auto &file : files)
        {
            wxString wxFile = wxString::FromUTF8(file);
            wxString extension = wxFileName(wxFile).GetExt().Lower();

            if (extension == "squashfs" || extension == "sfs" || extension == "img" || extension == "cramfs")
            {
                foundFiles.Add(wxFile);
                found = true;
            }
        }

        return found;
    }
    catch (const std::exception &e)
    {
        wxMessageBox("Error reading ISO: " + wxString(e.what()),
                     "Error", wxOK | wxICON_ERROR);
        return false;
    }
}

void MainFrame::OnDetect(wxCommandEvent &event)
{
    wxString isoPath = m_isoPathCtrl->GetValue();
    if (isoPath.IsEmpty())
    {
        wxMessageBox("Please select an ISO file first.", "Error",
                     wxOK | wxICON_ERROR);
        return;
    }

    m_distroCtrl->SetValue("Detecting...");
    m_progressGauge->SetValue(10);
    m_statusText->SetLabel("Analyzing ISO file...");
    Update();

    wxString distributionName;
    if (SearchGrubEnv(isoPath, distributionName))
    {
        m_distroCtrl->SetValue(distributionName);
        m_progressGauge->SetValue(100);
        m_statusText->SetLabel("Detection completed successfully using grubenv");

        if (m_projectNameCtrl->IsEmpty())
        {
            m_projectNameCtrl->SetValue(distributionName);
        }
        return;
    }

    wxString releaseContent;
    if (!SearchReleaseFile(isoPath, releaseContent))
    {
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

    if (m_projectNameCtrl->IsEmpty())
    {
        m_projectNameCtrl->SetValue(distribution);
    }
}

bool MainFrame::SearchReleaseFile(const wxString &isoPath, wxString &releaseContent)
{
    try
    {
        ISOReader reader(isoPath.ToStdString());
        if (!reader.open())
            return false;

        const auto &releasePaths = m_config["release_paths"];
        if (releasePaths && releasePaths.IsSequence())
        {
            for (const auto &pathNode : releasePaths)
            {
                std::vector<char> content;
                std::string path = pathNode.as<std::string>();
                if (reader.readFile(path, content))
                {
                    releaseContent = wxString(content.data(), content.size());
                    return true;
                }
            }
        }

        auto files = reader.listFiles();
        for (const auto &file : files)
        {
            std::string lowerFile = file;
            std::transform(lowerFile.begin(), lowerFile.end(), lowerFile.begin(), ::tolower);

            if (lowerFile.find("release") != std::string::npos ||
                lowerFile.find("version") != std::string::npos)
            {
                std::vector<char> content;
                if (reader.readFile(file, content))
                {
                    releaseContent = wxString(content.data(), content.size());
                    return true;
                }
            }
        }
        return false;
    }
    catch (const std::exception &e)
    {
        wxMessageBox("Error reading ISO: " + wxString(e.what()), "Error", wxOK | wxICON_ERROR);
        return false;
    }
}

bool MainFrame::SearchGrubEnv(const wxString &isoPath, wxString &distributionName)
{
    try
    {
        ISOReader reader(isoPath.ToStdString());
        if (!reader.open())
            return false;

        const auto &grubenvPaths = m_config["grubenv_paths"];
        if (grubenvPaths && grubenvPaths.IsSequence())
        {
            for (const auto &pathNode : grubenvPaths)
            {
                std::vector<char> content;
                std::string path = pathNode.as<std::string>();
                if (reader.readFile(path, content))
                {
                    wxString envContent(content.data(), content.size());
                    distributionName = ExtractNameFromGrubEnv(envContent);
                    if (!distributionName.IsEmpty())
                        return true;
                }
            }
        }

        auto files = reader.listFiles();
        for (const auto &file : files)
        {
            std::string lowerFile = file;
            std::transform(lowerFile.begin(), lowerFile.end(), lowerFile.begin(), ::tolower);

            if (lowerFile.find("grubenv") != std::string::npos)
            {
                std::vector<char> content;
                if (reader.readFile(file, content))
                {
                    wxString envContent(content.data(), content.size());
                    distributionName = ExtractNameFromGrubEnv(envContent);
                    if (!distributionName.IsEmpty())
                        return true;
                }
            }
        }
        return false;
    }
    catch (const std::exception &e)
    {
        wxMessageBox("Error reading ISO: " + wxString(e.what()), "Error", wxOK | wxICON_ERROR);
        return false;
    }
}

wxString MainFrame::ExtractNameFromGrubEnv(const wxString &content)
{
    wxString name;
    wxString version;

    wxStringTokenizer tokenizer(content, "\n");
    while (tokenizer.HasMoreTokens())
    {
        wxString line = tokenizer.GetNextToken().Trim(true).Trim(false);

        if (line.StartsWith("NAME="))
        {
            name = line.Mid(5);
            name.Replace("\"", "");
        }
        else if (line.StartsWith("VERSION="))
        {
            version = line.Mid(8);
            version.Replace("\"", "");
        }
    }

    if (!name.IsEmpty() && !version.IsEmpty())
    {
        return name + " " + version;
    }
    else if (!name.IsEmpty())
    {
        return name;
    }
    return "";
}

wxString MainFrame::DetectDistribution(const wxString &releaseContent)
{
    wxString lowerContent = releaseContent.Lower();

    const auto &distributions = m_config["distributions"];
    if (distributions && distributions.IsSequence())
    {
        for (const auto &distro : distributions)
        {
            if (!distro["pattern"] || !distro["name"])
                continue;

            wxString pattern = wxString::FromUTF8(distro["pattern"].as<std::string>());
            if (lowerContent.Contains(pattern))
            {
                wxString name = wxString::FromUTF8(distro["name"].as<std::string>());

                wxString version;
                int versionPos = lowerContent.Find("version");
                if (versionPos != wxNOT_FOUND)
                {
                    wxString remaining = releaseContent.Mid(versionPos + 7);
                    for (size_t i = 0; i < remaining.Length(); i++)
                    {
                        if (isdigit(remaining[i]) || remaining[i] == '.')
                        {
                            version += remaining[i];
                        }
                        else if (!version.IsEmpty())
                        {
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

void MainFrame::OnGUIDetected(wxCommandEvent &event)
{
    wxString guiName = event.GetString();
    if (m_desktopTab)
    {
        m_desktopTab->UpdateGUILabel(guiName); // Add this method to DesktopTab
    }
}