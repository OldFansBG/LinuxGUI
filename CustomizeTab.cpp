#include "CustomizeTab.h"
#include "SecondWindow.h"
#include <wx/filename.h>
#include <wx/wfstream.h>
#include <wx/dir.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/log.h>

wxDEFINE_EVENT(wxEVT_WALLPAPER_READY, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_WALLPAPER_LOAD_COMPLETE, wxCommandEvent);

wxBEGIN_EVENT_TABLE(CustomizeTab, wxPanel)
    EVT_COMMAND(wxID_ANY, wxEVT_WALLPAPER_READY, CustomizeTab::OnWallpaperReady)
    EVT_COMMAND(wxID_ANY, wxEVT_WALLPAPER_LOAD_COMPLETE, CustomizeTab::OnWallpaperLoadComplete)
wxEND_EVENT_TABLE()

CustomizeTab::CustomizeTab(wxWindow* parent) 
    : wxPanel(parent), 
      wallpaperPanel(nullptr), 
      m_loadingText(nullptr), 
      m_stopWallpaperLoad(false),
      m_isDestroyed(false)
{
    CreateContent();
}

CustomizeTab::~CustomizeTab() {
    wxLogDebug("CustomizeTab::~CustomizeTab - Entering destructor");
    m_stopWallpaperLoad = true;  // Signal thread to stop
    m_isDestroyed = true;        // Mark as destroyed
    wxMilliSleep(100);           // Give thread time to exit (optional, since detached)
    wxLogDebug("CustomizeTab::~CustomizeTab - Destructor complete");
}

void CustomizeTab::CreateContent() {
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    wxStaticBox* themeBox = new wxStaticBox(this, wxID_ANY, "Theme");
    themeBox->SetForegroundColour(*wxWHITE);
    wxStaticBoxSizer* themeSizer = new wxStaticBoxSizer(themeBox, wxVERTICAL);

    wxFlexGridSizer* themeGrid = new wxFlexGridSizer(2, 2, 10, 10);
    themeGrid->AddGrowableCol(1, 1);

    auto themeLabel = new wxStaticText(this, wxID_ANY, "Global Theme");
    themeLabel->SetForegroundColour(*wxWHITE);
    wxChoice* theme = new wxChoice(this, wxID_ANY);
    theme->Append({"Default", "Nord", "Dracula", "Gruvbox", "Tokyo Night", "Catppuccin"});
    theme->SetSelection(0);

    themeGrid->Add(themeLabel, 0, wxALIGN_CENTER_VERTICAL);
    themeGrid->Add(theme, 1, wxEXPAND);

    auto accentLabel = new wxStaticText(this, wxID_ANY, "Accent Color");
    accentLabel->SetForegroundColour(*wxWHITE);
    wxPanel* colorPanel = new wxPanel(this);
    colorPanel->SetBackgroundColour(wxColour(37, 99, 235));

    themeGrid->Add(accentLabel, 0, wxALIGN_CENTER_VERTICAL);
    themeGrid->Add(colorPanel, 1, wxEXPAND);

    themeSizer->Add(themeGrid, 1, wxEXPAND | wxALL, 5);
    sizer->Add(themeSizer, 0, wxEXPAND | wxALL, 10);

    wxStaticBox* wallpaperBox = new wxStaticBox(this, wxID_ANY, "Wallpapers");
    wallpaperBox->SetForegroundColour(*wxWHITE);
    wxStaticBoxSizer* wallpaperSizer = new wxStaticBoxSizer(wallpaperBox, wxVERTICAL);

    wallpaperPanel = new wxScrolledWindow(this, wxID_ANY);
    wallpaperPanel->SetScrollRate(10, 10);
    wallpaperPanel->SetBackgroundColour(wxColour(32, 32, 32));
    
    wxGridSizer* gridSizer = new wxGridSizer(4, 5, 5);
    wallpaperPanel->SetSizer(gridSizer);

    m_loadingText = new wxStaticText(wallpaperPanel, wxID_ANY, "Loading wallpapers...");
    m_loadingText->SetForegroundColour(*wxWHITE);
    gridSizer->Add(m_loadingText, 0, wxALIGN_CENTER | wxTOP, 20);
    m_loadingText->Hide();

    wallpaperSizer->Add(wallpaperPanel, 1, wxEXPAND | wxALL, 5);
    sizer->Add(wallpaperSizer, 1, wxEXPAND | wxALL, 10);

    this->SetSizer(sizer);
    this->Layout();
}

void CustomizeTab::LoadWallpaper() {
    wxLogDebug("CustomizeTab::LoadWallpaper - Starting wallpaper load process");

    wallpaperPanel->GetSizer()->Clear(true);
    wallpaperPanel->GetSizer()->Add(m_loadingText, 0, wxALIGN_CENTER | wxTOP, 20);
    m_loadingText->Show();
    wallpaperPanel->Layout();

    if (m_stopWallpaperLoad) {
        m_stopWallpaperLoad = false;
    }

    WallpaperLoadThread* thread = new WallpaperLoadThread(this, m_stopWallpaperLoad);
    if (thread->Run() != wxTHREAD_NO_ERROR) {
        delete thread;
        m_loadingText->SetLabel("Failed to load wallpapers");
        wxLogDebug("CustomizeTab::LoadWallpaper - Failed to start WallpaperLoadThread");
        return;
    }
    wxLogDebug("CustomizeTab::LoadWallpaper - WallpaperLoadThread started successfully");
}

wxString CustomizeTab::GetProjectDir() {
    wxLogDebug("CustomizeTab::GetProjectDir - Attempting to find project directory");
    wxWindow* parent = GetParent();
    while (parent) {
        SecondWindow* secondWindow = wxDynamicCast(parent, SecondWindow);
        if (secondWindow) {
            wxString projectDir = secondWindow->GetProjectDir();
            wxLogDebug("CustomizeTab::GetProjectDir - Found SecondWindow, project dir: %s", projectDir);
            return projectDir;
        }
        parent = parent->GetParent();
    }
    wxString cwd = wxGetCwd();
    wxLogDebug("CustomizeTab::GetProjectDir - No SecondWindow found, falling back to cwd: %s", cwd);
    return cwd;
}

wxString CustomizeTab::GetWallpaperDirFromCommand(const wxString& containerId, const wxString& detectedDE) {
    wxString command;
    if (detectedDE == "GNOME") {
        command = "gsettings get org.gnome.desktop.background picture-uri";
    } else if (detectedDE == "Cinnamon") {
        command = "gsettings get org.cinnamon.desktop.background picture-uri";
    } else if (detectedDE == "KDE Plasma") {
        command = "qdbus org.kde.plasma.desktop /Wallpaper org.kde.plasma.Wallpaper.image";
    } else if (detectedDE == "XFCE") {
        command = "xfconf-query -c xfce4-desktop -p /backdrop/screen0/monitor0/image-path";
    } else {
        return wxEmptyString;
    }

    wxString fullCommand = wxString::Format(
        "docker exec %s /bin/bash -c \"chroot /root/custom_iso/squashfs-root /bin/bash -c '%s'\"",
        containerId, command
    );
    wxLogDebug("CustomizeTab::GetWallpaperDirFromCommand - Executing command: %s", fullCommand);

    wxArrayString output;
    int exitCode = wxExecute(fullCommand, output);
    if (exitCode == 0 && !output.IsEmpty()) {
        wxString wallpaperPath = output[0].Trim();
        wxLogDebug("CustomizeTab::GetWallpaperDirFromCommand - Command output: %s", wallpaperPath);

        if (wallpaperPath.StartsWith("'file://")) {
            wallpaperPath = wallpaperPath.Mid(8).BeforeLast('\'');
        } else if (wallpaperPath.StartsWith("file://")) {
            wallpaperPath = wallpaperPath.Mid(7);
        }
        wxString fullDirPath = "/root/custom_iso/squashfs-root" + wallpaperPath.BeforeLast('/');
        if (!fullDirPath.EndsWith("/")) {
            fullDirPath += "/";
        }
        wxLogDebug("CustomizeTab::GetWallpaperDirFromCommand - Detected wallpaper directory: %s", fullDirPath);
        return fullDirPath;
    }
    return wxEmptyString;
}

void CustomizeTab::OnWallpaperReady(wxCommandEvent& event) {
    if (m_isDestroyed || IsBeingDeleted()) {
        wxLogDebug("CustomizeTab::OnWallpaperReady - Tab is destroyed, ignoring event");
        delete static_cast<WallpaperImageData*>(event.GetClientData());
        return;
    }

    wxLogDebug("CustomizeTab::OnWallpaperReady - Processing wallpaper event");
    WallpaperImageData* data = static_cast<WallpaperImageData*>(event.GetClientData());
    if (data && data->image.IsOk() && wallpaperPanel && wallpaperPanel->GetSizer()) {
        wxBitmap bitmap(data->image);
        wxStaticBitmap* wallpaperImage = new wxStaticBitmap(wallpaperPanel, wxID_ANY, bitmap);
        wallpaperPanel->GetSizer()->Add(wallpaperImage, 0, wxALL, 5);
        wallpaperPanel->Layout();
        wxLogDebug("CustomizeTab::OnWallpaperReady - Added wallpaper to grid");
    } else {
        wxLogDebug("CustomizeTab::OnWallpaperReady - Invalid data or panel state");
    }
    delete data;
}

void CustomizeTab::OnWallpaperLoadComplete(wxCommandEvent& event) {
    if (m_isDestroyed || IsBeingDeleted()) {
        wxLogDebug("CustomizeTab::OnWallpaperLoadComplete - Tab is destroyed, ignoring event");
        return;
    }

    wxLogDebug("CustomizeTab::OnWallpaperLoadComplete - Completing wallpaper load");
    if (m_loadingText) {
        m_loadingText->Hide();
    }
    if (wallpaperPanel) {
        wallpaperPanel->FitInside();
        wallpaperPanel->Layout();
    }
    wxLogDebug("CustomizeTab::OnWallpaperLoadComplete - Wallpaper loading completed");
}

wxThread::ExitCode WallpaperLoadThread::Entry() {
    wxLogDebug("WallpaperLoadThread::Entry - Starting background thread");

    wxString projectDir = m_tab->GetProjectDir();
    if (projectDir.IsEmpty()) {
        wxLogDebug("WallpaperLoadThread::Entry - Failed to get project directory");
        return (ExitCode)1;
    }

    wxString containerIdPath = wxFileName(projectDir, "container_id.txt").GetFullPath();
    wxString containerId;
    if (wxFileExists(containerIdPath)) {
        wxFile(containerIdPath).ReadAll(&containerId);
        containerId.Trim();
    } else {
        wxLogDebug("WallpaperLoadThread::Entry - container_id.txt not found at %s", containerIdPath);
        return (ExitCode)1;
    }

    wxString detectedGuiPath = wxFileName(projectDir, "detected_gui.txt").GetFullPath();
    wxString detectedDE;
    if (wxFileExists(detectedGuiPath)) {
        wxFile(detectedGuiPath).ReadAll(&detectedDE);
        detectedDE.Trim();
    } else {
        detectedDE = "Unknown";
    }

    wxString wallpaperDir = m_tab->GetWallpaperDirFromCommand(containerId, detectedDE);
    if (wallpaperDir.IsEmpty()) {
        wxLogDebug("WallpaperLoadThread::Entry - No wallpaper directory detected");
        return (ExitCode)1;
    }

    wxString localWallpaperDir = wxFileName(projectDir, "wallpapers").GetFullPath();
    if (!wxDirExists(localWallpaperDir)) {
        wxMkdir(localWallpaperDir);
    } else {
        wxDir dir(localWallpaperDir);
        wxString filename;
        bool cont = dir.GetFirst(&filename);
        while (cont) {
            wxRemoveFile(wxFileName(localWallpaperDir, filename).GetFullPath());
            cont = dir.GetNext(&filename);
        }
    }

    wxString cpCommand = wxString::Format("docker cp %s:%s \"%s\"", containerId, wallpaperDir, localWallpaperDir);
    wxLogDebug("WallpaperLoadThread::Entry - Executing: %s", cpCommand);
    int cpExitCode = wxExecute(cpCommand, wxEXEC_SYNC);
    if (cpExitCode != 0) {
        wxLogDebug("WallpaperLoadThread::Entry - Failed to copy wallpapers, exit code: %d", cpExitCode);
        return (ExitCode)1;
    }

    auto processDirectory = [&](wxDir& dir, const wxString& currentPath, auto& processDir) -> void {
        wxString filename;
        bool cont = dir.GetFirst(&filename, wxEmptyString, wxDIR_FILES | wxDIR_DIRS);
        while (cont && !m_stopFlag) {
            wxString filepath = wxFileName(currentPath, filename).GetFullPath();
            if (wxDirExists(filepath)) {
                wxDir subDir(filepath);
                if (subDir.IsOpened()) {
                    processDir(subDir, filepath, processDir);
                }
            } else {
                wxString ext = filename.Lower().AfterLast('.');
                if (ext == "jpg" || ext == "jpeg" || ext == "png") {
                    wxImage image;
                    if (image.LoadFile(filepath)) {
                        image.Rescale(100, 100, wxIMAGE_QUALITY_HIGH);
                        WallpaperImageData* data = new WallpaperImageData{image};
                        wxCommandEvent* event = new wxCommandEvent(wxEVT_WALLPAPER_READY);
                        event->SetClientData(data);
                        wxQueueEvent(m_tab, event);
                        wxMilliSleep(10);  // Small delay to allow event processing
                    }
                }
            }
            cont = dir.GetNext(&filename);
        }
    };

    wxDir dir(localWallpaperDir);
    if (dir.IsOpened()) {
        processDirectory(dir, localWallpaperDir, processDirectory);
    }

    if (!m_stopFlag) {
        wxCommandEvent* completeEvent = new wxCommandEvent(wxEVT_WALLPAPER_LOAD_COMPLETE);
        wxQueueEvent(m_tab, completeEvent);
    }

    wxLogDebug("WallpaperLoadThread::Entry - Thread exiting");
    return (ExitCode)0;
}