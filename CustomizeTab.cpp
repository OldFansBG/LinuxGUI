#include "CustomizeTab.h"
#include <wx/filename.h>
#include <wx/wfstream.h>
#include <wx/dir.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/log.h>
#include <wx/colordlg.h>
#include <wx/textfile.h>
#include <wx/thread.h>
#include <wx/utils.h> // For wxLaunchDefaultApplication

// Critical section for thread safety
wxCriticalSection gThreadCS;

// The hardcoded wallpaper directory
const wxString WALLPAPER_DIRECTORY = "C:\\Users\\PC\\Documents\\houdini20.5\\linuxmint";

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
    LoadWallpaper();
}

CustomizeTab::~CustomizeTab()
{
    m_stopWallpaperLoad = true;
    m_isDestroyed = true;
    wxMilliSleep(100);
}

void CustomizeTab::CreateContent()
{
    wxColour bgColor(18, 18, 27);
    wxColour cardBgColor(30, 30, 46);
    wxColour accentColor(137, 180, 250);
    wxColour textColor(205, 214, 244);
    wxColour secondaryTextColor(166, 173, 200);

    SetBackgroundColour(bgColor);

    wxBoxSizer *mainSizer = new wxBoxSizer(wxHORIZONTAL);

    wxPanel *controlPanel = new wxPanel(this, wxID_ANY);
    controlPanel->SetBackgroundColour(cardBgColor);

    wxBoxSizer *controlSizer = new wxBoxSizer(wxVERTICAL);
    controlSizer->AddSpacer(25);

    wxStaticText *appTitle = new wxStaticText(controlPanel, wxID_ANY, "Customization");
    wxFont titleFont = appTitle->GetFont();
    titleFont.SetPointSize(titleFont.GetPointSize() + 3);
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);
    appTitle->SetFont(titleFont);
    appTitle->SetForegroundColour(accentColor);
    controlSizer->Add(appTitle, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT, 25);
    controlSizer->AddSpacer(30);

    wxStaticText *themeHeader = new wxStaticText(controlPanel, wxID_ANY, "THEME SETTINGS");
    wxFont sectionFont = themeHeader->GetFont();
    sectionFont.SetPointSize(sectionFont.GetPointSize() - 1);
    sectionFont.SetWeight(wxFONTWEIGHT_BOLD);
    themeHeader->SetFont(sectionFont);
    themeHeader->SetForegroundColour(secondaryTextColor);
    controlSizer->Add(themeHeader, 0, wxLEFT | wxRIGHT, 25);
    controlSizer->AddSpacer(15);

    wxStaticText *themeLabel = new wxStaticText(controlPanel, wxID_ANY, "Global Theme");
    themeLabel->SetForegroundColour(textColor);
    controlSizer->Add(themeLabel, 0, wxLEFT | wxRIGHT, 25);
    controlSizer->AddSpacer(8);

    wxChoice *theme = new wxChoice(controlPanel, wxID_ANY);
    theme->Append(std::vector<wxString>{"Default", "Nord", "Dracula", "Gruvbox", "Tokyo Night", "Catppuccin"});
    theme->SetSelection(0);
    controlSizer->Add(theme, 0, wxEXPAND | wxLEFT | wxRIGHT, 25);
    controlSizer->AddSpacer(20);

    wxStaticText *accentLabel = new wxStaticText(controlPanel, wxID_ANY, "Accent Color");
    accentLabel->SetForegroundColour(textColor);
    controlSizer->Add(accentLabel, 0, wxLEFT | wxRIGHT, 25);
    controlSizer->AddSpacer(8);

    wxBoxSizer *colorSizer = new wxBoxSizer(wxHORIZONTAL);
    wxPanel *colorPanelDisplay = new wxPanel(controlPanel, wxID_ANY, wxDefaultPosition, wxSize(30, 30)); // Renamed
    colorPanelDisplay->SetBackgroundColour(accentColor);
    wxButton *changeColorBtn = new wxButton(controlPanel, wxID_ANY, "Choose Color");
    changeColorBtn->SetForegroundColour(textColor);
    changeColorBtn->SetBackgroundColour(cardBgColor.ChangeLightness(110));

    colorSizer->Add(colorPanelDisplay, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    // For a horizontal sizer, wxEXPAND makes it expand vertically.
    // wxALIGN_CENTER_VERTICAL conflicts with vertical wxEXPAND.
    // If you want the button to take available horizontal space and be centered vertically:
    // colorSizer->Add(changeColorBtn, 1, wxEXPAND); // This expands vertically. Not what we want for a button.
    // If you want button to take its natural height and be centered vertically, and expand horizontally
    colorSizer->Add(changeColorBtn, 1, wxALIGN_CENTER_VERTICAL); // Proportion 1 for horizontal expansion, wxALIGN_CENTER_VERTICAL for vertical alignment.

    controlSizer->Add(colorSizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 25);
    controlSizer->AddSpacer(30);

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

    wxButton *openFolderBtn = new wxButton(controlPanel, wxID_ANY, "Open Wallpaper Folder");
    openFolderBtn->SetForegroundColour(textColor);
    openFolderBtn->SetBackgroundColour(cardBgColor.ChangeLightness(110));
    controlSizer->Add(openFolderBtn, 0, wxEXPAND | wxLEFT | wxRIGHT, 25);
    controlSizer->AddSpacer(10);

    wxButton *refreshBtn = new wxButton(controlPanel, wxID_ANY, "Refresh Gallery");
    refreshBtn->SetForegroundColour(textColor);
    refreshBtn->SetBackgroundColour(cardBgColor.ChangeLightness(110));
    controlSizer->Add(refreshBtn, 0, wxEXPAND | wxLEFT | wxRIGHT, 25);
    controlSizer->AddStretchSpacer(1);

    wxStaticText *infoText = new wxStaticText(controlPanel, wxID_ANY,
                                              "Select any wallpaper to apply it\nas your desktop background.");
    infoText->SetForegroundColour(secondaryTextColor);
    controlSizer->Add(infoText, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 25);
    controlPanel->SetSizer(controlSizer);

    wxPanel *galleryContainer = new wxPanel(this, wxID_ANY);
    galleryContainer->SetBackgroundColour(bgColor);

    wxBoxSizer *gallerySizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *galleryHeaderSizer = new wxBoxSizer(wxHORIZONTAL);

    wxStaticText *galleryTitleText = new wxStaticText(galleryContainer, wxID_ANY, "Wallpaper Gallery"); // Renamed
    galleryTitleText->SetFont(titleFont);
    galleryTitleText->SetForegroundColour(textColor);
    // For a horizontal sizer, wxEXPAND makes it expand vertically.
    // wxALIGN_CENTER_VERTICAL conflicts. If you want it to take full height and be centered (if text is short), use wxEXPAND.
    // If you just want it aligned to the bottom or center of the available vertical space without expanding its own height:
    galleryHeaderSizer->Add(galleryTitleText, 1, wxALIGN_CENTER_VERTICAL | wxLEFT); // Proportion 1 for horizontal expansion. Align vertical.

    gallerySizer->Add(galleryHeaderSizer, 0, wxEXPAND | wxALL, 20);

    this->wallpaperPanel = new wxScrolledWindow(galleryContainer, wxID_ANY);
    this->wallpaperPanel->SetScrollRate(0, 10);
    this->wallpaperPanel->SetBackgroundColour(bgColor);
    gallerySizer->Add(this->wallpaperPanel, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 20);

    // m_loadingText will be parented to wallpaperPanel. Its sizer is set in LoadWallpaper/UpdateGrid.
    // Create it here so it's always available.
    m_loadingText = new wxStaticText(wallpaperPanel, wxID_ANY, "Loading wallpapers...");
    m_loadingText->SetForegroundColour(textColor);
    wxFont loadingFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    m_loadingText->SetFont(loadingFont);
    m_loadingText->Hide(); // Initially hidden, shown by LoadWallpaper

    galleryContainer->SetSizer(gallerySizer);

    mainSizer->Add(controlPanel, 0, wxEXPAND | wxALL, 0);
    mainSizer->Add(galleryContainer, 1, wxEXPAND | wxALL, 0);
    SetSizer(mainSizer);

    refreshBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent &)
                     { LoadWallpaper(); });
    addWallpaperBtn->Bind(wxEVT_BUTTON, &CustomizeTab::OnAddWallpaper, this);
    openFolderBtn->Bind(wxEVT_BUTTON, &CustomizeTab::OnOpenWallpaperFolder, this);

    changeColorBtn->Bind(wxEVT_BUTTON, [this, colorPanelDisplay](wxCommandEvent &) { // Use renamed panel
        wxColourData data;
        data.SetColour(colorPanelDisplay->GetBackgroundColour());
        wxColourDialog dialog(this, &data);
        if (dialog.ShowModal() == wxID_OK)
        {
            wxColour color = dialog.GetColourData().GetColour();
            colorPanelDisplay->SetBackgroundColour(color);
            colorPanelDisplay->Refresh();
        }
    });
    this->wallpaperPanel->Bind(wxEVT_SIZE, &CustomizeTab::OnWallpaperPanelSize, this);
}

void CustomizeTab::OnOpenWallpaperFolder(wxCommandEvent &WXUNUSED(event))
{
    if (m_isDestroyed || IsBeingDeleted())
        return;
    if (!wxDirExists(WALLPAPER_DIRECTORY))
    {
        wxMessageBox("The wallpaper directory does not exist:\n" + WALLPAPER_DIRECTORY, "Error", wxOK | wxICON_ERROR, this);
        return;
    }
    if (!wxLaunchDefaultApplication(WALLPAPER_DIRECTORY))
    {
        wxMessageBox("Could not open the wallpaper folder.\nPlease navigate to it manually:\n" + WALLPAPER_DIRECTORY, "Error", wxOK | wxICON_INFORMATION, this);
    }
}

void CustomizeTab::OnWallpaperPanelSize(wxSizeEvent &event)
{
    if (!IsWallpaperPanelReady())
    {
        event.Skip();
        return;
    }
    if (wallpaperPanel->GetSizer() && m_loadingText && m_loadingText->GetParent() && !m_loadingText->IsShown())
    {
        UpdateWallpaperGrid();
    }
    event.Skip();
}

void CustomizeTab::UpdateWallpaperGrid()
{
    if (!IsWallpaperPanelReady() || !m_loadingText || !m_loadingText->GetParent()) // Ensure m_loadingText is valid
        return;

    int panelWidth = wallpaperPanel->GetClientSize().GetWidth();
    if (panelWidth <= 0)
        panelWidth = 300;
    const int minThumbSize = 120;
    const int maxThumbSize = 220;
    const int margin = 16;
    int optimalColumns = std::max(1, panelWidth / (minThumbSize + margin));
    int thumbSize = std::min(maxThumbSize, (optimalColumns > 0 ? (panelWidth - (margin * (optimalColumns + 1))) / optimalColumns : minThumbSize));
    if (thumbSize <= 0)
        thumbSize = minThumbSize;

    std::vector<WallpaperImageData *> wallpaperData;
    bool hasExistingWallpapers = false;

    wxSizer *currentOuterSizer = wallpaperPanel->GetSizer();
    if (currentOuterSizer && currentOuterSizer->GetItemCount() > 0)
    {
        bool isLoadingSizer = (m_loadingText->IsShown() && currentOuterSizer->GetChildren().size() == 3 &&
                               currentOuterSizer->GetChildren().Item(1) &&
                               currentOuterSizer->GetChildren().Item(1)->GetData()->GetWindow() == m_loadingText);

        if (!isLoadingSizer)
        {
            wxFlexGridSizer *actualGridSizer = nullptr;
            for (wxSizerItem *item : currentOuterSizer->GetChildren())
            {
                if (item && item->IsSizer())
                {
                    actualGridSizer = wxDynamicCast(item->GetSizer(), wxFlexGridSizer);
                    if (actualGridSizer)
                        break;
                }
            }
            if (actualGridSizer)
            {
                hasExistingWallpapers = true;
                for (wxSizerItem *gridSizerItem : actualGridSizer->GetChildren())
                {
                    wxWindow *window = gridSizerItem->GetWindow();
                    if (window && window != m_loadingText)
                    { // Should not be m_loadingText here
                        wxStaticBitmap *bitmapControl = nullptr;
                        if (!window->GetChildren().IsEmpty())
                        {
                            bitmapControl = wxDynamicCast(window->GetChildren().GetFirst()->GetData(), wxStaticBitmap);
                        }
                        if (bitmapControl && bitmapControl->GetBitmap().IsOk())
                        {
                            WallpaperImageData *data = new WallpaperImageData;
                            data->image = bitmapControl->GetBitmap().ConvertToImage();
                            data->isSelected = window->GetBackgroundColour() == wxColour(137, 180, 250);
                            // How to get originalPath? It needs to be stored with the 'window' (container panel)
                            // For now, it will be empty or from a previous selection if not managed.
                            // wxStringClientData* pathData = dynamic_cast<wxStringClientData*>(window->GetClientObject());
                            // if (pathData) data->originalPath = pathData->GetData();

                            wallpaperData.push_back(data);
                        }
                    }
                }
            }
        }
    }

    wxBoxSizer *newOuterSizer = new wxBoxSizer(wxVERTICAL);

    if (hasExistingWallpapers && !wallpaperData.empty())
    {
        wxFlexGridSizer *newGridSizer = new wxFlexGridSizer(0, optimalColumns, margin, margin);
        for (WallpaperImageData *currentImgData : wallpaperData)
        {
            wxImage resizedImage = currentImgData->image.Copy();
            resizedImage.Rescale(thumbSize, thumbSize, wxIMAGE_QUALITY_HIGH);
            wxPanel *container = new wxPanel(wallpaperPanel, wxID_ANY);
            container->SetBackgroundColour(currentImgData->isSelected ? wxColour(137, 180, 250) : wxColour(49, 50, 68));
            // Store originalPath with the container
            // container->SetClientObject(new wxStringClientData(currentImgData->originalPath));

            wxBoxSizer *containerSizer = new wxBoxSizer(wxVERTICAL);
            wxBitmap bitmap(resizedImage);
            wxStaticBitmap *wallpaperImage = new wxStaticBitmap(container, wxID_ANY, bitmap);
            containerSizer->Add(wallpaperImage, 1, wxEXPAND | wxALL, 6); // Image expands in its panel
            container->SetSizer(containerSizer);

            container->Bind(wxEVT_LEFT_DOWN, [this, container, originalPath = currentImgData->originalPath](wxMouseEvent &)
                            {
                                if (!IsWallpaperPanelReady())
                                    return;
                                wxSizer *outerSizer = wallpaperPanel->GetSizer();
                                if (outerSizer)
                                {
                                    for (wxSizerItem *item : outerSizer->GetChildren())
                                    {
                                        if (item && item->IsSizer())
                                        {
                                            wxFlexGridSizer *flexGrid = wxDynamicCast(item->GetSizer(), wxFlexGridSizer);
                                            if (flexGrid)
                                            {
                                                for (wxSizerItem *gridItem : flexGrid->GetChildren())
                                                {
                                                    wxWindow *win = gridItem->GetWindow();
                                                    if (win && win != container)
                                                    {
                                                        win->SetBackgroundColour(wxColour(49, 50, 68));
                                                        win->Refresh();
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                                container->SetBackgroundColour(wxColour(137, 180, 250));
                                container->Refresh();
                                // wxLogMessage("UpdateWallpaperGrid Clicked: %s", originalPath);
                            });
            newGridSizer->Add(container, 1, wxEXPAND | wxALL, margin / 2); // Container expands in grid cell
        }
        newOuterSizer->AddSpacer(margin);
        newOuterSizer->Add(newGridSizer, 1, wxEXPAND | wxLEFT | wxRIGHT, margin);
        newOuterSizer->AddSpacer(margin);
        for (WallpaperImageData *imgDataToDel : wallpaperData)
        {
            delete imgDataToDel;
        }
    }
    else if (m_loadingText->IsShown())
    {
        if (m_loadingText->GetParent() != wallpaperPanel)
            m_loadingText->Reparent(wallpaperPanel);
        newOuterSizer->AddStretchSpacer(1);
        newOuterSizer->Add(m_loadingText, 0, wxALIGN_CENTER_HORIZONTAL);
        newOuterSizer->AddStretchSpacer(1);
    }
    else
    {
        wxStaticText *emptyGalleryText = new wxStaticText(wallpaperPanel, wxID_ANY, "No wallpapers found in gallery.");
        emptyGalleryText->SetForegroundColour(wxColour(166, 173, 200));
        newOuterSizer->AddStretchSpacer(1);
        newOuterSizer->Add(emptyGalleryText, 0, wxALIGN_CENTER_HORIZONTAL);
        newOuterSizer->AddStretchSpacer(1);
    }
    wallpaperPanel->SetSizer(newOuterSizer, true);
    wallpaperPanel->Layout();
    wallpaperPanel->FitInside();
    wallpaperPanel->Refresh();
}

bool CustomizeTab::IsWallpaperPanelReady() const
{
    wxCriticalSectionLocker lock(gThreadCS);
    if (!this || IsBeingDeleted() || m_isDestroyed)
        return false;
    if (!wallpaperPanel || wallpaperPanel->IsBeingDeleted())
        return false;
    return true;
}

void CustomizeTab::OnWallpaperReady(wxCommandEvent &event)
{
    wxCriticalSectionLocker lock(gThreadCS);
    if (m_isDestroyed || IsBeingDeleted() || !IsWallpaperPanelReady())
    {
        WallpaperImageData *data = static_cast<WallpaperImageData *>(event.GetClientData());
        if (data)
            delete data;
        return;
    }
    WallpaperImageData *imgData = static_cast<WallpaperImageData *>(event.GetClientData());
    if (!imgData || !imgData->image.IsOk())
    {
        if (imgData)
            delete imgData;
        return;
    }

    CallAfter([this, imgData]() mutable
              {
        if (!IsWallpaperPanelReady() || !imgData || !imgData->image.IsOk() || !m_loadingText || !m_loadingText->GetParent()) {
            if (imgData) delete imgData;
            return;
        }

        wxSizer *panelOuterSizer = wallpaperPanel->GetSizer();
        wxFlexGridSizer *actualGridSizer = nullptr;

        if (m_loadingText->IsShown()) {
            m_loadingText->Hide();
            // panelOuterSizer (the loading sizer) will be deleted by SetSizer below.
            // m_loadingText itself will be deleted if it was part of that sizer.
            // We create a new m_loadingText if its parent is gone after SetSizer.

            int panelWidth = wallpaperPanel->GetClientSize().GetWidth();
            if (panelWidth <=0) panelWidth = 300;
            const int minThumbSize = 120; const int margin = 16;
            int optimalColumns = std::max(1, panelWidth / (minThumbSize + margin));

            actualGridSizer = new wxFlexGridSizer(0, optimalColumns, margin, margin);
            wxBoxSizer *mainBoxSizerForGrid = new wxBoxSizer(wxVERTICAL);
            mainBoxSizerForGrid->AddSpacer(margin);
            mainBoxSizerForGrid->Add(actualGridSizer, 1, wxEXPAND | wxLEFT | wxRIGHT, margin);
            mainBoxSizerForGrid->AddSpacer(margin);
            wallpaperPanel->SetSizer(mainBoxSizerForGrid, true); // Delete old sizer (and m_loadingText if it was its child)
            panelOuterSizer = mainBoxSizerForGrid;

            // If m_loadingText was deleted, recreate it for future use (though it remains hidden for now)
            if (!m_loadingText->GetParent()) {
                 m_loadingText = new wxStaticText(wallpaperPanel, wxID_ANY, "Loading wallpapers...");
                 m_loadingText->SetForegroundColour(wxColour(205, 214, 244));
                 wxFont font(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
                 m_loadingText->SetFont(font);
                 m_loadingText->Hide();
            }
        } else {
            if (panelOuterSizer) {
                for (wxSizerItem *item : panelOuterSizer->GetChildren()) {
                    if (item && item->IsSizer()) {
                        actualGridSizer = wxDynamicCast(item->GetSizer(), wxFlexGridSizer);
                        if (actualGridSizer) break;
                    }
                }
            }
        }

        if (!actualGridSizer) {
            int panelWidth = wallpaperPanel->GetClientSize().GetWidth(); if (panelWidth <=0) panelWidth = 300;
            const int minThumbSize = 120; const int margin = 16;
            int optimalColumns = std::max(1, panelWidth / (minThumbSize + margin));
            actualGridSizer = new wxFlexGridSizer(0, optimalColumns, margin, margin);
            wxBoxSizer *mainBoxSizerForGrid = new wxBoxSizer(wxVERTICAL);
            mainBoxSizerForGrid->AddSpacer(margin);
            mainBoxSizerForGrid->Add(actualGridSizer, 1, wxEXPAND | wxLEFT | wxRIGHT, margin);
            mainBoxSizerForGrid->AddSpacer(margin);
            wallpaperPanel->SetSizer(mainBoxSizerForGrid, true);
        }

        if (actualGridSizer) {
            int cols = actualGridSizer->GetCols();
            if (cols == 0) {
                 int currentPanelWidth = wallpaperPanel->GetClientSize().GetWidth(); if (currentPanelWidth <= 0) currentPanelWidth = 300;
                 cols = std::max(1, currentPanelWidth / (120 + 16));
                 actualGridSizer->SetCols(cols);
            }
            int panelWidth = wallpaperPanel->GetClientSize().GetWidth(); if (panelWidth <=0) panelWidth = 300;
            const int margin = 16; const int maxThumbSize = 220; const int minThumbSize = 120;
            int thumbSize = std::min(maxThumbSize, (cols > 0 ? (panelWidth - (margin * (cols + 1))) / cols : minThumbSize));
            if (thumbSize <= 0) thumbSize = minThumbSize;

            wxImage resizedImage = imgData->image.Copy();
            resizedImage.Rescale(thumbSize, thumbSize, wxIMAGE_QUALITY_HIGH);
            wxPanel *container = new wxPanel(wallpaperPanel, wxID_ANY);
            // container->SetClientObject(new wxStringClientData(imgData->originalPath)); // Store path

            if (!container) { delete imgData; return; }
            container->SetBackgroundColour(wxColour(49, 50, 68));
            wxBoxSizer *containerSizer = new wxBoxSizer(wxVERTICAL);
            wxBitmap bitmap(resizedImage);
            if (!bitmap.IsOk()) { delete imgData; container->Destroy(); return; }
            wxStaticBitmap *wallpaperImage = new wxStaticBitmap(container, wxID_ANY, bitmap);
            containerSizer->Add(wallpaperImage, 1, wxEXPAND | wxALL, 6); // Image expands in its panel
            container->SetSizer(containerSizer);

            container->Bind(wxEVT_LEFT_DOWN, [this, container, originalPath = imgData->originalPath](wxMouseEvent &) {
                if (!IsWallpaperPanelReady()) return;
                wxSizer *outerSizer = wallpaperPanel->GetSizer();
                if (outerSizer) {
                     for (wxSizerItem *item : outerSizer->GetChildren()) {
                        if (item && item->IsSizer()) {
                            wxFlexGridSizer* flexGrid = wxDynamicCast(item->GetSizer(), wxFlexGridSizer);
                            if (flexGrid) {
                                for (wxSizerItem *gridItem : flexGrid->GetChildren()) {
                                    wxWindow *win = gridItem->GetWindow();
                                    if (win && win != container) {
                                        win->SetBackgroundColour(wxColour(49, 50, 68)); win->Refresh();
                                    }
                                }
                            }
                        }
                    }
                }
                container->SetBackgroundColour(wxColour(137, 180, 250)); container->Refresh();
                // wxLogMessage("OnWallpaperReady Clicked: %s", originalPath);
            });
            actualGridSizer->Add(container, 1, wxEXPAND | wxALL, margin / 2); // Cell expands
            wallpaperPanel->Layout(); wallpaperPanel->FitInside(); wallpaperPanel->Refresh();
        }
        delete imgData; });
}

void CustomizeTab::OnWallpaperLoadComplete(wxCommandEvent &WXUNUSED(event))
{
    wxCriticalSectionLocker lock(gThreadCS);
    if (!IsWallpaperPanelReady())
        return;
    CallAfter([this]()
              {
        if (!IsWallpaperPanelReady() || !m_loadingText || !m_loadingText->GetParent()) return;
        if (m_loadingText->IsShown()) {
            m_loadingText->Hide();
        }
        UpdateWallpaperGrid();
        if (wallpaperPanel) {
            wallpaperPanel->FitInside(); wallpaperPanel->Layout(); wallpaperPanel->Refresh();
        } });
}

void CustomizeTab::LoadWallpaper()
{
    CallAfter([this]()
              {
        if (m_isDestroyed || !wallpaperPanel) return;
        {
            wxCriticalSectionLocker lock(gThreadCS);
            // Ensure m_loadingText is valid and parented before checking its sizer
            if (!m_loadingText || !m_loadingText->GetParent()) {
                 m_loadingText = new wxStaticText(wallpaperPanel, wxID_ANY, "Loading...");
                 m_loadingText->SetForegroundColour(wxColour(205, 214, 244));
                 wxFont font(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
                 m_loadingText->SetFont(font);
            }
            m_loadingText->SetLabel("Loading wallpapers...");

            wxSizer* currentSizer = wallpaperPanel->GetSizer();
            bool currentIsLoadingSizer = false;
            if (currentSizer) {
                 currentIsLoadingSizer = (currentSizer->GetChildren().size() == 3 &&
                                       currentSizer->GetChildren().Item(1) &&
                                       currentSizer->GetChildren().Item(1)->GetData()->GetWindow() == m_loadingText);
            }

            if (!currentIsLoadingSizer) {
                if(currentSizer) currentSizer->Clear(true);
                wxBoxSizer *loadingSizer = new wxBoxSizer(wxVERTICAL);
                loadingSizer->AddStretchSpacer(1);
                loadingSizer->Add(m_loadingText, 0, wxALIGN_CENTER_HORIZONTAL);
                loadingSizer->AddStretchSpacer(1);
                wallpaperPanel->SetSizer(loadingSizer, true);
            }
            m_loadingText->Show();
            wallpaperPanel->Layout(); wallpaperPanel->Refresh();
            if (m_stopWallpaperLoad) m_stopWallpaperLoad = false;
        }
        WallpaperLoadThread *thread = new WallpaperLoadThread(this, m_stopWallpaperLoad);
        if (thread->Run() != wxTHREAD_NO_ERROR) {
            delete thread;
            wxCriticalSectionLocker lock2(gThreadCS);
            if (m_loadingText && m_loadingText->GetParent() && IsWallpaperPanelReady()) {
                m_loadingText->SetLabel("Failed to start wallpaper load");
                wallpaperPanel->Layout(); wallpaperPanel->Refresh();
            }
        } });
}

void CustomizeTab::OnAddWallpaper(wxCommandEvent &WXUNUSED(event))
{
    if (m_isDestroyed || IsBeingDeleted())
        return;
    wxFileDialog openFileDialog(this, "Choose an image", "", "",
                                "Image files (*.png;*.jpg;*.jpeg;*.bmp)|*.png;*.jpg;*.jpeg;*.bmp",
                                wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (openFileDialog.ShowModal() == wxID_CANCEL)
        return;
    wxString filePath = openFileDialog.GetPath();
    if (filePath.IsEmpty() || !wxFileExists(filePath))
    {
        wxMessageBox("Selected file does not exist or is invalid.", "Error", wxOK | wxICON_ERROR, this);
        return;
    }
    wxImage image;
    if (image.LoadFile(filePath))
    {
        const wxString &targetDir = WALLPAPER_DIRECTORY;
        if (!wxDirExists(targetDir))
        {
            if (!wxFileName::Mkdir(targetDir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL))
            {
                wxMessageBox("Failed to create dir:\n" + targetDir, "Error", wxOK | wxICON_ERROR, this);
                return;
            }
        }
        wxFileName fn(filePath);
        wxString fileName = fn.GetFullName();
        if (fileName.IsEmpty())
        {
            wxMessageBox("Invalid original filename.", "Error", wxOK | wxICON_ERROR, this);
            return;
        }
        wxString targetPath = wxFileName(targetDir, fileName).GetFullPath();
        wxString origName = fn.GetName();
        wxString ext = fn.GetExt();
        int counter = 1;
        while (wxFileExists(targetPath))
        {
            wxString newName = wxString::Format("%s_%d.%s", origName, counter++, ext);
            targetPath = wxFileName(targetDir, newName).GetFullPath();
            if (counter > 1000)
            {
                wxMessageBox("Could not create unique filename.", "Error", wxOK | wxICON_ERROR, this);
                return;
            }
        }
        if (!image.SaveFile(targetPath))
        {
            wxMessageBox("Failed to save image to:\n" + targetPath, "Error", wxOK | wxICON_ERROR, this);
            return;
        }
        WallpaperImageData *data = new WallpaperImageData;
        data->image = image.Copy();
        data->originalPath = targetPath;
        wxCommandEvent *newEvent = new wxCommandEvent(wxEVT_WALLPAPER_READY);
        newEvent->SetClientData(data);
        wxQueueEvent(this, newEvent);
    }
    else
    {
        wxMessageBox("Failed to load selected image.", "Error", wxOK | wxICON_ERROR, this);
    }
}

wxThread::ExitCode WallpaperLoadThread::Entry()
{ /* ... unchanged ... */
    wxMilliSleep(50);
    if (m_stopFlag || !m_tab || m_tab->IsBeingDeleted() || m_tab->m_isDestroyed)
    {
        return (ExitCode)1;
    }

    const wxString &localWallpaperDir = WALLPAPER_DIRECTORY;
    if (!wxDirExists(localWallpaperDir))
    {
        if (!wxFileName::Mkdir(localWallpaperDir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL))
        {
            wxLogError("WallpaperLoadThread: Wallpaper directory does not exist and cannot be created: %s", localWallpaperDir);
            wxCriticalSectionLocker lock(gThreadCS);
            if (m_tab && !m_tab->IsBeingDeleted() && !m_tab->m_isDestroyed && m_tab->IsWallpaperPanelReady())
            {
                wxCommandEvent *completeEvent = new wxCommandEvent(wxEVT_WALLPAPER_LOAD_COMPLETE);
                wxQueueEvent(m_tab, completeEvent);
            }
            return (ExitCode)1;
        }
    }

    wxDir dir(localWallpaperDir);
    if (!dir.IsOpened())
    {
        wxLogError("WallpaperLoadThread: Could not open wallpaper directory: %s", localWallpaperDir);
        wxCriticalSectionLocker lock(gThreadCS);
        if (m_tab && !m_tab->IsBeingDeleted() && !m_tab->m_isDestroyed && m_tab->IsWallpaperPanelReady())
        {
            wxCommandEvent *completeEvent = new wxCommandEvent(wxEVT_WALLPAPER_LOAD_COMPLETE);
            wxQueueEvent(m_tab, completeEvent);
        }
        return (ExitCode)1;
    }

    wxString filename;
    bool cont = dir.GetFirst(&filename, wxEmptyString, wxDIR_FILES | wxDIR_HIDDEN);
    while (cont && !m_stopFlag && m_tab && !m_tab->IsBeingDeleted() && !m_tab->m_isDestroyed)
    {
        wxString filepath = wxFileName(localWallpaperDir, filename).GetFullPath();
        wxFileName fn(filepath);
        wxString ext = fn.GetExt().Lower();

        if (ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "bmp")
        {
            wxImage image;
            if (image.LoadFile(filepath))
            {
                if (m_stopFlag || !m_tab || m_tab->IsBeingDeleted() || m_tab->m_isDestroyed)
                {
                    return (ExitCode)1;
                }
                WallpaperImageData *data = new WallpaperImageData;
                data->image = image.Copy();
                data->originalPath = filepath;
                wxCommandEvent *event = new wxCommandEvent(wxEVT_WALLPAPER_READY);
                event->SetClientData(data);
                {
                    wxCriticalSectionLocker lock(gThreadCS);
                    if (m_tab && !m_tab->IsBeingDeleted() && !m_tab->m_isDestroyed)
                    {
                        wxQueueEvent(m_tab, event);
                    }
                    else
                    {
                        delete data;
                        delete event;
                    }
                }
                wxMilliSleep(15);
            }
            else
            {
                wxLogWarning("WallpaperLoadThread: Failed to load image file: %s", filepath);
            }
        }
        cont = dir.GetNext(&filename);
    }

    {
        wxCriticalSectionLocker lock(gThreadCS);
        if (!m_stopFlag && m_tab && !m_tab->IsBeingDeleted() && !m_tab->m_isDestroyed)
        {
            wxCommandEvent *completeEvent = new wxCommandEvent(wxEVT_WALLPAPER_LOAD_COMPLETE);
            wxQueueEvent(m_tab, completeEvent);
        }
    }
    return (ExitCode)0;
}