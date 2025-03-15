#include "CustomizeTab.h"
#include "SecondWindow.h"
#include <wx/filename.h>
#include <wx/wfstream.h>
#include <wx/dir.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/log.h>
#include <wx/colordlg.h>
wxDEFINE_EVENT(wxEVT_WALLPAPER_READY, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_WALLPAPER_LOAD_COMPLETE, wxCommandEvent);
wxBEGIN_EVENT_TABLE(CustomizeTab, wxPanel)
    EVT_COMMAND(wxID_ANY, wxEVT_WALLPAPER_READY, CustomizeTab::OnWallpaperReady)
        EVT_COMMAND(wxID_ANY, wxEVT_WALLPAPER_LOAD_COMPLETE, CustomizeTab::OnWallpaperLoadComplete)
            wxEND_EVENT_TABLE()
                CustomizeTab::CustomizeTab(wxWindow *parent)
    : wxPanel(parent),
      wallpaperPanel(nullptr),
      m_loadingText(nullptr),
      m_stopWallpaperLoad(false),
      m_isDestroyed(false)
{
    CreateContent();
}
CustomizeTab::~CustomizeTab()
{
    m_stopWallpaperLoad = true;
    m_isDestroyed = true;
    wxMilliSleep(100);
}

void CustomizeTab::CreateContent()
{
    // Modern color palette
    wxColour bgColor(18, 18, 27);
    wxColour cardBgColor(30, 30, 46);
    wxColour accentColor(137, 180, 250);
    wxColour textColor(205, 214, 244);
    wxColour secondaryTextColor(166, 173, 200);

    SetBackgroundColour(bgColor);

    // Create a horizontal split layout
    wxBoxSizer *mainSizer = new wxBoxSizer(wxHORIZONTAL);

    // --- LEFT SIDEBAR (Controls Panel) ---
    wxPanel *controlPanel = new wxPanel(this, wxID_ANY);
    controlPanel->SetBackgroundColour(cardBgColor);

    wxBoxSizer *controlSizer = new wxBoxSizer(wxVERTICAL);
    controlSizer->AddSpacer(25);

    // App title/header
    wxStaticText *appTitle = new wxStaticText(controlPanel, wxID_ANY, "Customization");
    wxFont titleFont = appTitle->GetFont();
    titleFont.SetPointSize(titleFont.GetPointSize() + 3);
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);
    appTitle->SetFont(titleFont);
    appTitle->SetForegroundColour(accentColor);

    controlSizer->Add(appTitle, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT, 25);
    controlSizer->AddSpacer(30);

    // Theme controls section
    wxStaticText *themeHeader = new wxStaticText(controlPanel, wxID_ANY, "THEME SETTINGS");
    wxFont sectionFont = themeHeader->GetFont();
    sectionFont.SetPointSize(sectionFont.GetPointSize() - 1);
    sectionFont.SetWeight(wxFONTWEIGHT_BOLD);
    themeHeader->SetFont(sectionFont);
    themeHeader->SetForegroundColour(secondaryTextColor);

    controlSizer->Add(themeHeader, 0, wxLEFT | wxRIGHT, 25);
    controlSizer->AddSpacer(15);

    // Theme dropdown with label
    wxStaticText *themeLabel = new wxStaticText(controlPanel, wxID_ANY, "Global Theme");
    themeLabel->SetForegroundColour(textColor);

    controlSizer->Add(themeLabel, 0, wxLEFT | wxRIGHT, 25);
    controlSizer->AddSpacer(8);

    wxChoice *theme = new wxChoice(controlPanel, wxID_ANY);
    theme->Append(std::vector<wxString>{"Default", "Nord", "Dracula", "Gruvbox", "Tokyo Night", "Catppuccin"});
    theme->SetSelection(0);

    controlSizer->Add(theme, 0, wxEXPAND | wxLEFT | wxRIGHT, 25);
    controlSizer->AddSpacer(20);

    // Accent color with preview and button
    wxStaticText *accentLabel = new wxStaticText(controlPanel, wxID_ANY, "Accent Color");
    accentLabel->SetForegroundColour(textColor);

    controlSizer->Add(accentLabel, 0, wxLEFT | wxRIGHT, 25);
    controlSizer->AddSpacer(8);

    wxBoxSizer *colorSizer = new wxBoxSizer(wxHORIZONTAL);

    wxPanel *colorPanel = new wxPanel(controlPanel, wxID_ANY, wxDefaultPosition, wxSize(30, 30));
    colorPanel->SetBackgroundColour(accentColor);

    wxButton *changeColorBtn = new wxButton(controlPanel, wxID_ANY, "Choose Color");
    changeColorBtn->SetForegroundColour(textColor);
    changeColorBtn->SetBackgroundColour(cardBgColor.ChangeLightness(110));

    colorSizer->Add(colorPanel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    colorSizer->Add(changeColorBtn, 1, wxALIGN_CENTER_VERTICAL);

    controlSizer->Add(colorSizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 25);
    controlSizer->AddSpacer(30);

    // Wallpaper controls section
    wxStaticText *wpHeader = new wxStaticText(controlPanel, wxID_ANY, "WALLPAPER");
    wpHeader->SetFont(sectionFont);
    wpHeader->SetForegroundColour(secondaryTextColor);

    controlSizer->Add(wpHeader, 0, wxLEFT | wxRIGHT, 25);
    controlSizer->AddSpacer(15);

    wxButton *addWallpaperBtn = new wxButton(controlPanel, wxID_ANY, "Upload New Wallpaper");
    addWallpaperBtn->SetForegroundColour(textColor);
    addWallpaperBtn->SetBackgroundColour(accentColor);

    controlSizer->Add(addWallpaperBtn, 0, wxEXPAND | wxLEFT | wxRIGHT, 25);
    controlSizer->AddSpacer(10);

    wxButton *refreshBtn = new wxButton(controlPanel, wxID_ANY, "Refresh Gallery");
    refreshBtn->SetForegroundColour(textColor);
    refreshBtn->SetBackgroundColour(cardBgColor.ChangeLightness(110));

    controlSizer->Add(refreshBtn, 0, wxEXPAND | wxLEFT | wxRIGHT, 25);

    // Push everything up with a stretcher at the bottom
    controlSizer->AddStretchSpacer(1);

    // Bottom info text
    wxStaticText *infoText = new wxStaticText(controlPanel, wxID_ANY,
                                              "Select any wallpaper to apply it\nas your desktop background.");
    infoText->SetForegroundColour(secondaryTextColor);

    controlSizer->Add(infoText, 0, wxALIGN_CENTER | wxALL, 25);

    controlPanel->SetSizer(controlSizer);

    // --- RIGHT CONTENT AREA (Wallpaper Gallery) ---
    wxPanel *galleryContainer = new wxPanel(this, wxID_ANY);
    galleryContainer->SetBackgroundColour(bgColor);

    wxBoxSizer *gallerySizer = new wxBoxSizer(wxVERTICAL);

    // Gallery heading
    wxBoxSizer *galleryHeaderSizer = new wxBoxSizer(wxHORIZONTAL);

    wxStaticText *galleryTitle = new wxStaticText(galleryContainer, wxID_ANY, "Wallpaper Gallery");
    galleryTitle->SetFont(titleFont);
    galleryTitle->SetForegroundColour(textColor);

    galleryHeaderSizer->Add(galleryTitle, 1, wxALIGN_BOTTOM);

    // Counter for wallpapers (optional info)
    wxStaticText *counterText = new wxStaticText(galleryContainer, wxID_ANY, "");
    counterText->SetForegroundColour(secondaryTextColor);

    galleryHeaderSizer->Add(counterText, 0, wxALIGN_BOTTOM);

    gallerySizer->Add(galleryHeaderSizer, 0, wxEXPAND | wxALL, 20);

    // Scrollable wallpaper container with auto-resizing grid
    this->wallpaperPanel = new wxScrolledWindow(galleryContainer, wxID_ANY);
    this->wallpaperPanel->SetScrollRate(0, 10);
    this->wallpaperPanel->SetBackgroundColour(bgColor);

    // We'll use a dynamic grid created in OnSize
    gallerySizer->Add(this->wallpaperPanel, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 20);

    // Loading text centered in the gallery
    m_loadingText = new wxStaticText(this->wallpaperPanel, wxID_ANY, "Loading wallpapers...");
    m_loadingText->SetForegroundColour(textColor);
    wxFont loadingFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    m_loadingText->SetFont(loadingFont);

    wxBoxSizer *loadingSizer = new wxBoxSizer(wxVERTICAL);
    loadingSizer->AddStretchSpacer(1);
    loadingSizer->Add(m_loadingText, 0, wxALIGN_CENTER);
    loadingSizer->AddStretchSpacer(1);

    this->wallpaperPanel->SetSizer(loadingSizer);
    galleryContainer->SetSizer(gallerySizer);

    // Add the two main panels to the horizontal layout
    mainSizer->Add(controlPanel, 0, wxEXPAND);
    mainSizer->Add(galleryContainer, 1, wxEXPAND);

    SetSizer(mainSizer);

    // Bind events
    refreshBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent &)
                     { LoadWallpaper(); });

    // Fix this event binding - changed to wxEVT_BUTTON from wxEVT_LEFT_DOWN
    addWallpaperBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent &event)
                          { OnAddWallpaper(event); });

    changeColorBtn->Bind(wxEVT_BUTTON, [this, colorPanel](wxCommandEvent &)
                         {
        wxColourData data;
        data.SetColour(colorPanel->GetBackgroundColour());
        wxColourDialog dialog(this, &data);
        if (dialog.ShowModal() == wxID_OK) {
            wxColour color = dialog.GetColourData().GetColour();
            colorPanel->SetBackgroundColour(color);
            colorPanel->Refresh();
        } });

    // Bind size event to handle responsive layout
    this->wallpaperPanel->Bind(wxEVT_SIZE, &CustomizeTab::OnWallpaperPanelSize, this);
}

void CustomizeTab::OnWallpaperPanelSize(wxSizeEvent &event)
{
    // Skip if we're still loading or being destroyed
    if (m_isDestroyed || IsBeingDeleted() || !wallpaperPanel)
    {
        event.Skip();
        return;
    }

    // Only update the grid if we have wallpapers to show
    if (!m_loadingText->IsShown())
    {
        UpdateWallpaperGrid();
    }

    event.Skip();
}

void CustomizeTab::UpdateWallpaperGrid()
{
    if (!wallpaperPanel || m_isDestroyed)
        return;

    // Get panel width and calculate optimal number of columns
    int panelWidth = wallpaperPanel->GetClientSize().GetWidth();

    // Determine optimal thumbnail size and column count
    // Minimum size 120px, maximum 220px, with 16px margin between items
    const int minThumbSize = 120;
    const int maxThumbSize = 220;
    const int margin = 16;

    // Calculate how many columns we can fit
    int optimalColumns = std::max(1, panelWidth / (minThumbSize + margin));

    // Calculate the actual thumbnail size based on available space
    int thumbSize = std::min(maxThumbSize,
                             (panelWidth - (margin * (optimalColumns + 1))) / optimalColumns);

    // Store the current wallpaper data to recreate the grid
    std::vector<WallpaperImageData *> wallpaperData;
    bool hasExistingWallpapers = false;

    if (wallpaperPanel->GetSizer() && wallpaperPanel->GetSizer()->GetItemCount() > 1)
    {
        hasExistingWallpapers = true;

        // Save existing wallpaper data
        wxSizerItemList items = wallpaperPanel->GetSizer()->GetChildren();
        for (wxSizerItemList::iterator it = items.begin(); it != items.end(); ++it)
        {
            wxSizerItem *item = *it;
            wxWindow *window = item->GetWindow();

            if (window && window != m_loadingText)
            {
                wxStaticBitmap *bitmap = wxDynamicCast(window->FindWindow(wxID_ANY), wxStaticBitmap);
                if (bitmap && bitmap->GetBitmap().IsOk())
                {
                    WallpaperImageData *data = new WallpaperImageData;
                    data->image = bitmap->GetBitmap().ConvertToImage();
                    wxWindow *parent = bitmap->GetParent();
                    if (parent)
                    {
                        data->isSelected = parent->GetBackgroundColour() == wxColour(137, 180, 250);
                    }
                    wallpaperData.push_back(data);
                }
            }
        }
    }

    // Create new sizer for wallpapers
    wxBoxSizer *wallpaperSizer = new wxBoxSizer(wxVERTICAL);

    if (hasExistingWallpapers)
    {
        // Create a new grid with the calculated number of columns
        wxFlexGridSizer *gridSizer = new wxFlexGridSizer(0, optimalColumns, margin, margin);

        // Add wallpapers back to the grid with the new size
        for (WallpaperImageData *data : wallpaperData)
        {
            // Resize image to new thumbnail size
            wxImage resizedImage = data->image.Copy();
            resizedImage.Rescale(thumbSize, thumbSize, wxIMAGE_QUALITY_HIGH);

            // Create container panel
            wxPanel *container = new wxPanel(wallpaperPanel, wxID_ANY);
            container->SetBackgroundColour(data->isSelected ? wxColour(137, 180, 250) : wxColour(49, 50, 68));

            wxBoxSizer *containerSizer = new wxBoxSizer(wxVERTICAL);
            container->SetSizer(containerSizer);

            // Create bitmap display
            wxBitmap bitmap(resizedImage);
            wxStaticBitmap *wallpaperImage = new wxStaticBitmap(container, wxID_ANY, bitmap);

            containerSizer->Add(wallpaperImage, 0, wxALL, 6);

            // Bind selection event
            container->Bind(wxEVT_LEFT_DOWN, [this, container](wxMouseEvent &)
                            {
                // Deselect all other wallpapers
                wxSizerItemList items = wallpaperPanel->GetSizer()->GetChildren();
                for (wxSizerItemList::iterator it = items.begin(); it != items.end(); ++it) {
                    wxSizerItem *item = *it;
                    wxWindow *window = item->GetWindow();
                    if (window && window != container && window != m_loadingText) {
                        window->SetBackgroundColour(wxColour(49, 50, 68));
                        window->Refresh();
                    }
                }
                
                // Highlight selected wallpaper
                container->SetBackgroundColour(wxColour(137, 180, 250));
                container->Refresh(); });

            gridSizer->Add(container, 0, wxALL, margin / 2);
        }

        wallpaperSizer->AddSpacer(margin);
        wallpaperSizer->Add(gridSizer, 1, wxALIGN_CENTER | wxLEFT | wxRIGHT, margin);

        // Clean up the temporary data
        for (WallpaperImageData *data : wallpaperData)
        {
            delete data;
        }
    }
    else
    {
        // If no wallpapers yet, show loading text
        wallpaperSizer->AddStretchSpacer(1);
        wallpaperSizer->Add(m_loadingText, 0, wxALIGN_CENTER);
        wallpaperSizer->AddStretchSpacer(1);
    }

    // Replace existing sizer
    wallpaperPanel->SetSizer(wallpaperSizer);
    wallpaperPanel->Layout();
}

void CustomizeTab::OnWallpaperReady(wxCommandEvent &event)
{
    if (m_isDestroyed || IsBeingDeleted())
    {
        delete static_cast<WallpaperImageData *>(event.GetClientData());
        return;
    }

    WallpaperImageData *data = static_cast<WallpaperImageData *>(event.GetClientData());
    if (data && data->image.IsOk() && wallpaperPanel)
    {
        // Hide loading text if it's visible
        if (m_loadingText->IsShown())
        {
            m_loadingText->Hide();

            // Replace loading sizer with grid sizer
            int panelWidth = wallpaperPanel->GetClientSize().GetWidth();
            const int minThumbSize = 120;
            const int margin = 16;
            int optimalColumns = std::max(1, panelWidth / (minThumbSize + margin));

            wxFlexGridSizer *gridSizer = new wxFlexGridSizer(0, optimalColumns, margin, margin);
            wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
            mainSizer->AddSpacer(margin);
            mainSizer->Add(gridSizer, 1, wxALIGN_CENTER | wxLEFT | wxRIGHT, margin);

            wallpaperPanel->SetSizer(mainSizer);
        }

        // Get the current grid sizer
        wxSizer *panelSizer = wallpaperPanel->GetSizer();
        if (panelSizer && panelSizer->GetItemCount() > 0)
        {
            wxSizerItem *item = panelSizer->GetItem((size_t)0);
            if (item && item->IsSpacer())
                item = panelSizer->GetItem((size_t)1);

            wxFlexGridSizer *gridSizer = wxDynamicCast(
                item ? item->GetSizer() : nullptr,
                wxFlexGridSizer);

            if (gridSizer)
            {
                // Determine current thumbnail size based on column count
                int cols = gridSizer->GetCols();
                int panelWidth = wallpaperPanel->GetClientSize().GetWidth();
                const int margin = 16;
                const int maxThumbSize = 220;

                int thumbSize = std::min(maxThumbSize,
                                         (panelWidth - (margin * (cols + 1))) / cols);

                // Resize the image to fit the thumbnail
                wxImage resizedImage = data->image.Copy();
                resizedImage.Rescale(thumbSize, thumbSize, wxIMAGE_QUALITY_HIGH);

                // Create the container panel
                wxPanel *container = new wxPanel(wallpaperPanel, wxID_ANY);
                container->SetBackgroundColour(wxColour(49, 50, 68));

                wxBoxSizer *containerSizer = new wxBoxSizer(wxVERTICAL);
                container->SetSizer(containerSizer);

                // Add the bitmap
                wxBitmap bitmap(resizedImage);
                wxStaticBitmap *wallpaperImage = new wxStaticBitmap(container, wxID_ANY, bitmap);

                containerSizer->Add(wallpaperImage, 0, wxALL, 6);

                // Add selection behavior
                container->Bind(wxEVT_LEFT_DOWN, [this, container](wxMouseEvent &)
                                {
                    // Deselect all other wallpapers
                    wxSizerItemList items = wallpaperPanel->GetSizer()->GetChildren();
                    for (auto& item : items) {
                        if (item->IsSizer()) {
                            wxSizerItemList gridItems = item->GetSizer()->GetChildren();
                            for (auto& gridItem : gridItems) {
                                wxWindow *window = gridItem->GetWindow();
                                if (window && window != container) {
                                    window->SetBackgroundColour(wxColour(49, 50, 68));
                                    window->Refresh();
                                }
                            }
                        }
                    }
                    
                    // Highlight selected wallpaper
                    container->SetBackgroundColour(wxColour(137, 180, 250));
                    container->Refresh(); });

                // Add to the grid
                gridSizer->Add(container, 0, wxALL, margin / 2);
                wallpaperPanel->Layout();
            }
        }
    }

    delete data;
}

void CustomizeTab::OnWallpaperLoadComplete(wxCommandEvent &event)
{
    if (m_isDestroyed || IsBeingDeleted())
    {
        return;
    }

    if (m_loadingText)
    {
        m_loadingText->Hide();
    }

    if (wallpaperPanel)
    {
        // Update and optimize the wallpaper grid
        UpdateWallpaperGrid();
        wallpaperPanel->FitInside();
        wallpaperPanel->Layout();
    }
}

void CustomizeTab::OnAddWallpaper(wxCommandEvent &event)
{
    wxFileDialog openFileDialog(this, "Choose an image", "", "",
                                "Image files (*.png;*.jpg;*.jpeg;*.bmp)|*.png;*.jpg;*.jpeg;*.bmp",
                                wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (openFileDialog.ShowModal() == wxID_CANCEL)
        return;
    wxString filePath = openFileDialog.GetPath();
    wxImage image;
    if (image.LoadFile(filePath))
    {
        image.Rescale(200, 200, wxIMAGE_QUALITY_HIGH); // Using larger thumbnail for better quality
        WallpaperImageData *data = new WallpaperImageData{image};
        data->originalPath = filePath;
        wxCommandEvent newEvent(wxEVT_WALLPAPER_READY);
        newEvent.SetClientData(data);
        OnWallpaperReady(newEvent);
    }
    else
    {
        wxMessageBox("Failed to load the selected image.", "Error", wxOK | wxICON_ERROR);
    }
}
void CustomizeTab::LoadWallpaper()
{
    CallAfter([this]()
              {
        if (m_isDestroyed || !wallpaperPanel || !wallpaperPanel->GetSizer()) {
            return;
        }
        wallpaperPanel->GetSizer()->Clear(true);
        m_loadingText = new wxStaticText(wallpaperPanel, wxID_ANY, "Loading wallpapers...");
        m_loadingText->SetForegroundColour(*wxWHITE);
        wxFont font(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
        m_loadingText->SetFont(font);
        wallpaperPanel->GetSizer()->Add(m_loadingText, 0, wxALIGN_CENTER | wxTOP, 20);
        wallpaperPanel->Layout();
        if (m_stopWallpaperLoad)
        {
            m_stopWallpaperLoad = false;
        }
        WallpaperLoadThread *thread = new WallpaperLoadThread(this, m_stopWallpaperLoad);
        if (thread->Run() != wxTHREAD_NO_ERROR)
        {
            delete thread;
            if (m_loadingText)
            {
                m_loadingText->SetLabel("Failed to load wallpapers");
            }
            return;
        } });
}
wxString CustomizeTab::GetProjectDir()
{
    wxWindow *parent = GetParent();
    while (parent)
    {
        SecondWindow *secondWindow = wxDynamicCast(parent, SecondWindow);
        if (secondWindow)
        {
            wxString projectDir = secondWindow->GetProjectDir();
            return projectDir;
        }
        parent = parent->GetParent();
    }
    wxString cwd = wxGetCwd();
    return cwd;
}
wxString CustomizeTab::ExecuteCommandOnMainThread(const wxString &command, wxArrayString *output)
{
    wxString result;
    wxSemaphore semaphore(0, 1);
    CallAfter([this, command, output, &result, &semaphore]()
              {
        int exitCode;
        if (output) {
            exitCode = wxExecute(command, *output);
        } else {
            exitCode = wxExecute(command, wxEXEC_SYNC);
        }
        if (exitCode == 0) {
            if (output && !output->IsEmpty()) {
                result = (*output)[0];
            } else {
                result = "SUCCESS";
            }
        }
        semaphore.Post(); });
    semaphore.Wait();
    return result;
}
wxString CustomizeTab::GetWallpaperDirFromCommand(const wxString &containerId, const wxString &detectedDE)
{
    wxString command;
    if (detectedDE == "GNOME")
    {
        command = "gsettings get org.gnome.desktop.background picture-uri";
    }
    else if (detectedDE == "Cinnamon")
    {
        command = "gsettings get org.cinnamon.desktop.background picture-uri";
    }
    else if (detectedDE == "KDE Plasma")
    {
        command = "qdbus org.kde.plasma.desktop /Wallpaper org.kde.plasma.Wallpaper.image";
    }
    else if (detectedDE == "XFCE")
    {
        command = "xfconf-query -c xfce4-desktop -p /backdrop/screen0/monitor0/image-path";
    }
    else
    {
        return wxEmptyString;
    }
    wxString fullCommand = wxString::Format(
        "docker exec %s /bin/bash -c \"chroot /root/custom_iso/squashfs-root /bin/bash -c '%s'\"",
        containerId, command);
    wxArrayString output;
    wxString result = ExecuteCommandOnMainThread(fullCommand, &output);
    if (!result.IsEmpty() && !output.IsEmpty())
    {
        wxString wallpaperPath = output[0].Trim();
        if (wallpaperPath.StartsWith("'file://"))
        {
            wallpaperPath = wallpaperPath.Mid(8).BeforeLast('\'');
        }
        else if (wallpaperPath.StartsWith("file://"))
        {
            wallpaperPath = wallpaperPath.Mid(7);
        }
        wxString fullDirPath = "/root/custom_iso/squashfs-root" + wallpaperPath.BeforeLast('/');
        if (!fullDirPath.EndsWith("/"))
        {
            fullDirPath += "/";
        }
        return fullDirPath;
    }
    return wxEmptyString;
}
wxThread::ExitCode WallpaperLoadThread::Entry()
{
    wxString projectDir = m_tab->GetProjectDir();
    if (projectDir.IsEmpty())
    {
        return (ExitCode)1;
    }
    wxString containerIdPath = wxFileName(projectDir, "container_id.txt").GetFullPath();
    wxString containerId;
    if (wxFileExists(containerIdPath))
    {
        wxFile(containerIdPath).ReadAll(&containerId);
        containerId.Trim();
    }
    else
    {
        return (ExitCode)1;
    }
    wxString detectedGuiPath = wxFileName(projectDir, "detected_gui.txt").GetFullPath();
    wxString detectedDE;
    if (wxFileExists(detectedGuiPath))
    {
        wxFile(detectedGuiPath).ReadAll(&detectedDE);
        detectedDE.Trim();
    }
    else
    {
        detectedDE = "Unknown";
    }
    wxString wallpaperDir = m_tab->GetWallpaperDirFromCommand(containerId, detectedDE);
    if (wallpaperDir.IsEmpty())
    {
        return (ExitCode)1;
    }
    wxString localWallpaperDir = wxFileName(projectDir, "wallpapers").GetFullPath();
    if (!wxDirExists(localWallpaperDir))
    {
        wxMkdir(localWallpaperDir);
    }
    else
    {
        wxDir dir(localWallpaperDir);
        wxString filename;
        bool cont = dir.GetFirst(&filename);
        while (cont)
        {
            wxRemoveFile(wxFileName(localWallpaperDir, filename).GetFullPath());
            cont = dir.GetNext(&filename);
        }
    }
    wxString cpCommand = wxString::Format("docker cp %s:%s \"%s\"", containerId, wallpaperDir, localWallpaperDir);
    wxString result = m_tab->ExecuteCommandOnMainThread(cpCommand);
    if (result != "SUCCESS")
    {
        return (ExitCode)1;
    }
    auto processDirectory = [&](wxDir &dir, const wxString &currentPath, auto &processDir) -> void
    {
        wxString filename;
        bool cont = dir.GetFirst(&filename, wxEmptyString, wxDIR_FILES | wxDIR_DIRS);
        while (cont && !m_stopFlag)
        {
            wxString filepath = wxFileName(currentPath, filename).GetFullPath();
            if (wxDirExists(filepath))
            {
                wxDir subDir(filepath);
                if (subDir.IsOpened())
                {
                    processDir(subDir, filepath, processDir);
                }
            }
            else
            {
                wxString ext = filename.Lower().AfterLast('.');
                if (ext == "jpg" || ext == "jpeg" || ext == "png")
                {
                    wxImage image;
                    if (image.LoadFile(filepath))
                    {
                        image.Rescale(100, 100, wxIMAGE_QUALITY_HIGH);
                        WallpaperImageData *data = new WallpaperImageData{image};
                        wxCommandEvent *event = new wxCommandEvent(wxEVT_WALLPAPER_READY);
                        event->SetClientData(data);
                        wxQueueEvent(m_tab, event);
                        wxMilliSleep(10);
                    }
                }
            }
            cont = dir.GetNext(&filename);
        }
    };
    wxDir dir(localWallpaperDir);
    if (dir.IsOpened())
    {
        processDirectory(dir, localWallpaperDir, processDirectory);
    }
    if (!m_stopFlag)
    {
        wxCommandEvent *completeEvent = new wxCommandEvent(wxEVT_WALLPAPER_LOAD_COMPLETE);
        wxQueueEvent(m_tab, completeEvent);
    }
    return (ExitCode)0;
};