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
#include <wx/utils.h>
#include <wx/stdpaths.h>       // wxStandardPaths за папката с тапети
#include "ContainerCommand.h"  // Команди в chroot средата на Docker контейнера // For wxLaunchDefaultApplication

// Critical section for thread safety
wxCriticalSection gThreadCS;

// The hardcoded wallpaper directory

wxDEFINE_EVENT(wxEVT_WALLPAPER_READY, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_WALLPAPER_LOAD_COMPLETE, wxCommandEvent);

wxBEGIN_EVENT_TABLE(CustomizeTab, wxPanel)
    EVT_COMMAND(wxID_ANY, wxEVT_WALLPAPER_READY, CustomizeTab::OnWallpaperReady)
        EVT_COMMAND(wxID_ANY, wxEVT_WALLPAPER_LOAD_COMPLETE, CustomizeTab::OnWallpaperLoadComplete)
            wxEND_EVENT_TABLE()

                CustomizeTab::CustomizeTab(wxWindow *parent, const wxString &projectDir)
    : wxPanel(parent),
      wallpaperPanel(nullptr),
      m_loadingText(nullptr),
      m_stopWallpaperLoad(false),
      m_isDestroyed(false),
      m_loadThread(nullptr),
      m_loadGeneration(0),
      m_projectDir(projectDir),
      m_wallpaperDir(wxStandardPaths::Get().GetUserDataDir() + wxFILE_SEP_PATH + "wallpapers"),
      m_applyWallpaperButton(nullptr)
{
    CreateContent();
    LoadWallpaper();
}

CustomizeTab::~CustomizeTab()
{
    m_isDestroyed = true;
    StopWallpaperThread(); // Нишката трябва да приключи, преди обектът да бъде освободен
}

// Спира нишката за зареждане на тапети и изчаква да приключи
void CustomizeTab::StopWallpaperThread()
{
    if (!m_loadThread)
        return;
    m_stopWallpaperLoad = true;
    m_loadThread->Wait(); // Нишката проверява флага след всеки файл
    delete m_loadThread;
    m_loadThread = nullptr;
    m_stopWallpaperLoad = false;
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

    wxStaticText *themeLabel = new wxStaticText(controlPanel, wxID_ANY, "Global Theme (coming soon)");
    themeLabel->SetForegroundColour(textColor);
    controlSizer->Add(themeLabel, 0, wxLEFT | wxRIGHT, 25);
    controlSizer->AddSpacer(8);

    wxChoice *theme = new wxChoice(controlPanel, wxID_ANY);
    theme->Append(std::vector<wxString>{"Default", "Nord", "Dracula", "Gruvbox", "Tokyo Night", "Catppuccin"});
    theme->SetSelection(0);
    theme->Disable(); // Още не се прилага към ISO образа
    theme->SetToolTip("Coming soon");
    controlSizer->Add(theme, 0, wxEXPAND | wxLEFT | wxRIGHT, 25);
    controlSizer->AddSpacer(20);

    wxStaticText *accentLabel = new wxStaticText(controlPanel, wxID_ANY, "Accent Color (coming soon)");
    accentLabel->SetForegroundColour(textColor);
    controlSizer->Add(accentLabel, 0, wxLEFT | wxRIGHT, 25);
    controlSizer->AddSpacer(8);

    wxBoxSizer *colorSizer = new wxBoxSizer(wxHORIZONTAL);
    wxPanel *colorPanelDisplay = new wxPanel(controlPanel, wxID_ANY, wxDefaultPosition, wxSize(30, 30)); // Renamed
    colorPanelDisplay->SetBackgroundColour(accentColor);
    wxButton *changeColorBtn = new wxButton(controlPanel, wxID_ANY, "Choose Color");
    changeColorBtn->SetForegroundColour(textColor);
    changeColorBtn->SetBackgroundColour(cardBgColor.ChangeLightness(110));
    changeColorBtn->Disable(); // Още не се прилага към ISO образа
    changeColorBtn->SetToolTip("Coming soon");

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
    controlSizer->AddSpacer(10);
    m_applyWallpaperButton = new wxButton(controlPanel, wxID_ANY, "Apply Wallpaper to ISO");
    m_applyWallpaperButton->SetForegroundColour(textColor);
    m_applyWallpaperButton->SetBackgroundColour(accentColor);
    m_applyWallpaperButton->Disable(); // Активира се, когато е избран тапет
    controlSizer->Add(m_applyWallpaperButton, 0, wxEXPAND | wxLEFT | wxRIGHT, 25);
    controlSizer->AddStretchSpacer(1);

    wxStaticText *infoText = new wxStaticText(controlPanel, wxID_ANY,
                                              "Select a wallpaper and click\n\"Apply Wallpaper to ISO\" to make it\nthe default background\n(GNOME, Cinnamon, MATE).");
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
    m_applyWallpaperButton->Bind(wxEVT_BUTTON, &CustomizeTab::OnApplyWallpaper, this);

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
    if (!wxDirExists(m_wallpaperDir))
    {
        wxMessageBox("The wallpaper directory does not exist:\n" + m_wallpaperDir, "Error", wxOK | wxICON_ERROR, this);
        return;
    }
    if (!wxLaunchDefaultApplication(m_wallpaperDir))
    {
        wxMessageBox("Could not open the wallpaper folder.\nPlease navigate to it manually:\n" + m_wallpaperDir, "Error", wxOK | wxICON_INFORMATION, this);
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
                            data->originalPath = window->GetName();
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
            container->SetName(currentImgData->originalPath); // Пътят до файла се пази в името на контейнера

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
                                SelectWallpaper(container);
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
    if (imgData && event.GetInt() != m_loadGeneration) // Събитие от предишно зареждане
    {
        delete imgData;
        return;
    }
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
            container->SetName(imgData->originalPath); // Пътят до файла се пази в името на контейнера

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
                SelectWallpaper(container);
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
        }
        StopWallpaperThread(); // Предишното зареждане спира, за да няма дублирани тапети
        m_loadGeneration++;
        m_loadThread = new WallpaperLoadThread(this, m_stopWallpaperLoad, m_wallpaperDir, m_loadGeneration);
        if (m_loadThread->Run() != wxTHREAD_NO_ERROR) {
            delete m_loadThread;
            m_loadThread = nullptr;
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
        const wxString &targetDir = m_wallpaperDir;
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
        newEvent->SetInt(m_loadGeneration);
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

    const wxString &localWallpaperDir = m_dir;
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
                event->SetInt(m_generation);
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
// Маркира избрания тапет и активира бутона за прилагане
void CustomizeTab::SelectWallpaper(wxPanel *container)
{
    if (!m_applyWallpaperButton || !container)
        return;
    m_applyWallpaperButton->Enable(!container->GetName().IsEmpty() && wxFileExists(container->GetName()));
    m_applyWallpaperButton->SetClientObject(new wxStringClientData(container->GetName())); // Пътят на избрания файл
}

// Копира избрания тапет в ISO образа и го задава като фон по подразбиране (GNOME, Cinnamon, MATE)
void CustomizeTab::OnApplyWallpaper(wxCommandEvent &WXUNUSED(event))
{
    wxStringClientData *selected = dynamic_cast<wxStringClientData *>(m_applyWallpaperButton->GetClientObject());
    wxString sourcePath = selected ? selected->GetData() : wxString();
    if (sourcePath.IsEmpty() || !wxFileExists(sourcePath))
    {
        wxMessageBox("Select a wallpaper first.", "Apply Wallpaper", wxOK | wxICON_INFORMATION, this);
        return;
    }

    wxString containerId = ContainerCommand::ReadContainerId(m_projectDir);
    if (containerId.IsEmpty())
    {
        wxMessageBox("The Docker container is not ready yet. Wait until the initial setup has finished.",
                     "Container Not Ready", wxOK | wxICON_INFORMATION, this);
        return;
    }

    wxString ext = wxFileName(sourcePath).GetExt().Lower();
    wxString target = "/usr/share/backgrounds/linuxisopro/wallpaper." + ext; // Път вътре в ISO образа

    wxBusyCursor busy;
    wxArrayString output;
    if (ContainerCommand::RunInChroot(containerId, "mkdir -p /usr/share/backgrounds/linuxisopro && rm -f /usr/share/backgrounds/linuxisopro/wallpaper.*", &output) != 0)
    {
        wxMessageBox("Could not prepare the wallpaper folder in the ISO.", "Apply Wallpaper", wxOK | wxICON_ERROR, this);
        return;
    }

    wxString copyCommand = wxString::Format("docker cp \"%s\" %s:%s%s", sourcePath, containerId, ContainerCommand::CHROOT_PATH, target);
    wxArrayString copyOutput, copyErrors;
    if (wxExecute(copyCommand, copyOutput, copyErrors, wxEXEC_SYNC | wxEXEC_HIDE_CONSOLE) != 0)
    {
        wxMessageBox("Copying the wallpaper into the container failed:\n" + wxJoin(copyErrors, '\n'), "Apply Wallpaper", wxOK | wxICON_ERROR, this);
        return;
    }

    // GSettings override за всяка инсталирана среда, която поддържа тапет по подразбиране
    wxString script =
        "WP=" + ContainerCommand::ShellQuote(target) + "\n"
        "chmod 644 \"$WP\"\n"
        "S=/usr/share/glib-2.0/schemas\n"
        "OUT=$S/99_linuxisopro-wallpaper.gschema.override\n"
        ": > \"$OUT\"\n"
        "if [ -f $S/org.gnome.desktop.background.gschema.xml ]; then\n"
        "    printf \"[org.gnome.desktop.background]\\npicture-uri='file://%s'\\npicture-options='zoom'\\n\" \"$WP\" >> \"$OUT\"\n"
        "    grep -q picture-uri-dark $S/org.gnome.desktop.background.gschema.xml && printf \"picture-uri-dark='file://%s'\\n\" \"$WP\" >> \"$OUT\"\n"
        "    echo >> \"$OUT\"\n"
        "fi\n"
        "if [ -f $S/org.cinnamon.desktop.background.gschema.xml ]; then\n"
        "    printf \"[org.cinnamon.desktop.background]\\npicture-uri='file://%s'\\npicture-options='zoom'\\n\\n\" \"$WP\" >> \"$OUT\"\n"
        "fi\n"
        "if [ -f $S/org.mate.background.gschema.xml ]; then\n"
        "    printf \"[org.mate.background]\\npicture-filename='%s'\\npicture-options='zoom'\\n\\n\" \"$WP\" >> \"$OUT\"\n"
        "fi\n"
        "if [ ! -s \"$OUT\" ]; then\n"
        "    rm -f \"$OUT\"\n"
        "    echo 'No GNOME, Cinnamon or MATE desktop found in this image.'\n"
        "    exit 2\n"
        "fi\n"
        "glib-compile-schemas $S\n";

    output.Clear();
    long exitCode = ContainerCommand::RunInChroot(containerId, script, &output);
    if (exitCode == 0)
        wxMessageBox("The wallpaper is now the default background of the ISO.", "Apply Wallpaper", wxOK | wxICON_INFORMATION, this);
    else if (exitCode == 2)
        wxMessageBox("The wallpaper was copied to " + target + ", but no GNOME, Cinnamon or MATE desktop was found to set it as default.",
                     "Apply Wallpaper", wxOK | wxICON_WARNING, this);
    else
        wxMessageBox("Setting the wallpaper failed:\n" + wxJoin(output, '\n'), "Apply Wallpaper", wxOK | wxICON_ERROR, this);
}
