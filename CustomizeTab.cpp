#include "CustomizeTab.h"
#include "SecondWindow.h"
#include <wx/filename.h>
#include <wx/wfstream.h>
#include <wx/dir.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>

wxBEGIN_EVENT_TABLE(CustomizeTab, wxPanel)
// Event table is empty for now but included for future extensibility
wxEND_EVENT_TABLE()

CustomizeTab::CustomizeTab(wxWindow* parent) 
    : wxPanel(parent), wallpaperPanel(nullptr)
{
    CreateContent();
}

void CustomizeTab::CreateContent() {
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    // **Theme Section**
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

    // **Wallpaper Section**
    wxStaticBox* wallpaperBox = new wxStaticBox(this, wxID_ANY, "Wallpapers");
    wallpaperBox->SetForegroundColour(*wxWHITE);
    wxStaticBoxSizer* wallpaperSizer = new wxStaticBoxSizer(wallpaperBox, wxVERTICAL);

    wallpaperPanel = new wxScrolledWindow(this, wxID_ANY);
    wallpaperPanel->SetScrollRate(10, 10);  // Enable scrolling
    wallpaperPanel->SetBackgroundColour(wxColour(32, 32, 32));
    
    // Use wxGridSizer for a photo gallery layout (e.g., 4 columns)
    wxGridSizer* gridSizer = new wxGridSizer(4, 5, 5);  // 4 columns, 5px gap
    wallpaperPanel->SetSizer(gridSizer);

    wallpaperSizer->Add(wallpaperPanel, 1, wxEXPAND | wxALL, 5);
    sizer->Add(wallpaperSizer, 1, wxEXPAND | wxALL, 10);

    this->SetSizer(sizer);
    this->Layout();

    // Load wallpapers after initializing the UI
    LoadWallpaper();
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
        return wxEmptyString;  // Return empty string for unsupported DEs
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

        // Parse the wallpaper path to extract the directory
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

void CustomizeTab::LoadWallpaper() {
    wxLogDebug("CustomizeTab::LoadWallpaper - Starting wallpaper load process");

    // Retrieve project directory
    wxString projectDir = GetProjectDir();
    if (projectDir.IsEmpty()) {
        wxLogDebug("CustomizeTab::LoadWallpaper - Failed to get project directory");
        return;
    }
    wxLogDebug("CustomizeTab::LoadWallpaper - Project directory: %s", projectDir);

    // Read container ID
    wxString containerIdPath = wxFileName(projectDir, "container_id.txt").GetFullPath();
    wxString containerId;
    if (wxFileExists(containerIdPath)) {
        wxFile(containerIdPath).ReadAll(&containerId);
        containerId.Trim();
        wxLogDebug("CustomizeTab::LoadWallpaper - Container ID read: %s", containerId);
    } else {
        wxLogDebug("CustomizeTab::LoadWallpaper - container_id.txt not found at %s", containerIdPath);
        return;
    }

    // Read detected desktop environment
    wxString detectedGuiPath = wxFileName(projectDir, "detected_gui.txt").GetFullPath();
    wxString detectedDE;
    if (wxFileExists(detectedGuiPath)) {
        wxFile(detectedGuiPath).ReadAll(&detectedDE);
        detectedDE.Trim();
        wxLogDebug("CustomizeTab::LoadWallpaper - Detected desktop environment: %s", detectedDE);
    } else {
        detectedDE = "Unknown";
        wxLogDebug("CustomizeTab::LoadWallpaper - detected_gui.txt not found, defaulting to: %s", detectedDE);
    }

    // Prepare local wallpaper directory
    wxString localWallpaperDir = wxFileName(projectDir, "wallpapers").GetFullPath();
    if (!wxDirExists(localWallpaperDir)) {
        wxMkdir(localWallpaperDir);
        wxLogDebug("CustomizeTab::LoadWallpaper - Created local wallpaper directory: %s", localWallpaperDir);
    } else {
        // Clear existing files if necessary
        wxDir dir(localWallpaperDir);
        wxString filename;
        bool cont = dir.GetFirst(&filename);
        while (cont) {
            wxRemoveFile(wxFileName(localWallpaperDir, filename).GetFullPath());
            cont = dir.GetNext(&filename);
        }
        wxLogDebug("CustomizeTab::LoadWallpaper - Cleared existing files in %s", localWallpaperDir);
    }

    // Get the wallpaper directory from DE-specific command
    wxString wallpaperDir = GetWallpaperDirFromCommand(containerId, detectedDE);
    if (!wallpaperDir.IsEmpty()) {
        // Construct and log the docker cp command
        wxString cpCommand = wxString::Format("docker cp %s:%s \"%s\"", containerId, wallpaperDir, localWallpaperDir);
        wxLogDebug("CustomizeTab::LoadWallpaper - Executing command: %s", cpCommand);

        // Execute the command and capture output for debugging
        wxArrayString output, errors;
        int cpExitCode = wxExecute(cpCommand, output, errors, wxEXEC_SYNC);
        if (cpExitCode == 0 || wxDirExists(localWallpaperDir) && wxDir(localWallpaperDir).HasFiles()) {
            wxLogDebug("CustomizeTab::LoadWallpaper - Wallpaper directory copied successfully from %s to %s", wallpaperDir, localWallpaperDir);
            if (!output.IsEmpty()) wxLogDebug("CustomizeTab::LoadWallpaper - Command output: %s", output[0]);
            if (!errors.IsEmpty()) wxLogDebug("CustomizeTab::LoadWallpaper - Command errors: %s", errors[0]);

            // Recursive function to process all files and subdirectories
            auto processDirectory = [&](wxDir& dir, const wxString& currentPath, auto& processDir) -> void {
                wxString filename;
                bool cont = dir.GetFirst(&filename, wxEmptyString, wxDIR_FILES | wxDIR_DIRS);
                while (cont) {
                    wxString filepath = wxFileName(currentPath, filename).GetFullPath();
                    if (wxDirExists(filepath)) {
                        wxDir subDir(filepath);
                        if (subDir.IsOpened()) {
                            processDir(subDir, filepath, processDir);  // Recursively process subdirectories
                        }
                    } else {
                        wxString ext = filename.Lower().AfterLast('.');
                        if (ext == "jpg" || ext == "jpeg" || ext == "png") {
                            wxLogDebug("CustomizeTab::LoadWallpaper - Processing image file: %s", filepath);
                            wxBitmap bitmap;
                            if (ext == "png") {
                                bitmap.LoadFile(filepath, wxBITMAP_TYPE_PNG);
                            } else {
                                bitmap.LoadFile(filepath, wxBITMAP_TYPE_JPEG);
                            }

                            if (bitmap.IsOk()) {
                                wxImage img = bitmap.ConvertToImage();
                                img.Rescale(100, 100, wxIMAGE_QUALITY_HIGH);
                                wxBitmap scaledBitmap(img);
                                wxStaticBitmap* wallpaperImage = new wxStaticBitmap(wallpaperPanel, wxID_ANY, scaledBitmap);
                                wallpaperPanel->GetSizer()->Add(wallpaperImage, 0, wxALL, 5);  // Add to grid
                                wxLogDebug("CustomizeTab::LoadWallpaper - Added wallpaper: %s", filepath);
                            } else {
                                wxLogDebug("CustomizeTab::LoadWallpaper - Failed to load image: %s", filepath);
                            }
                        }
                    }
                    cont = dir.GetNext(&filename);
                }
            };

            // Start processing the local wallpaper directory
            wxDir dir(localWallpaperDir);
            if (dir.IsOpened()) {
                processDirectory(dir, localWallpaperDir, processDirectory);
            } else {
                wxLogDebug("CustomizeTab::LoadWallpaper - Failed to open local wallpaper directory: %s", localWallpaperDir);
            }
        } else {
            wxLogDebug("CustomizeTab::LoadWallpaper - Failed to copy wallpaper directory, exit code: %d", cpExitCode);
            if (!output.IsEmpty()) wxLogDebug("CustomizeTab::LoadWallpaper - Command output: %s", output[0]);
            if (!errors.IsEmpty()) wxLogDebug("CustomizeTab::LoadWallpaper - Command errors: %s", errors[0]);
        }
    } else {
        wxLogDebug("CustomizeTab::LoadWallpaper - No wallpaper directory detected from command");
    }

    // Finalize layout
    wallpaperPanel->Layout();
    wallpaperPanel->FitInside();
    wxLogDebug("CustomizeTab::LoadWallpaper - Wallpaper load process completed");
}