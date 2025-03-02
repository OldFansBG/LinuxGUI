#include "DesktopTab.h"
#include <wx/filename.h>
#include <array>
#include <wx/textfile.h>
#include "SecondWindow.h" // Include here in the .cpp file instead of .h
#include <wx/file.h>
#include <wx/wfstream.h>
#include <wx/txtstrm.h>

// Custom event declaration
wxDEFINE_EVENT(FILE_COPY_COMPLETE_EVENT, wxCommandEvent);

DesktopTab::DesktopTab(wxWindow* parent)
    : wxPanel(parent), m_gridSizer(nullptr)
{
    CreateDesktopTab();
    Bind(wxEVT_SIZE, &DesktopTab::OnSize, this);
    Bind(FILE_COPY_COMPLETE_EVENT, &DesktopTab::OnFileCopyComplete, this);
}

wxBitmap DesktopTab::LoadImage(const wxString& imageName) {
    wxFileName path(wxFileName::GetCwd(), imageName);
    path.AppendDir("desktopenvs");
    if (path.FileExists()) {
        return wxBitmap(path.GetFullPath(), wxBITMAP_TYPE_PNG);
    }
    return wxNullBitmap;
}

void DesktopTab::OnSize(wxSizeEvent& event) {
    wxSize size = event.GetSize();
    RecalculateLayout(size.GetWidth());
    event.Skip();
}

void DesktopTab::RecalculateLayout(int windowWidth) {
    if (!m_gridSizer) return;

    // Account for margins
    int availableWidth = windowWidth - 40; // 20px margin on each side

    // Calculate number of columns based on available width
    // Minimum card width is 200px, with 20px spacing between cards
    int cardWithSpacing = 220; // 200px + 20px spacing
    int columns = std::max(1, availableWidth / cardWithSpacing);

    // Update grid columns if changed
    if (m_gridSizer->GetCols() != columns) {
        m_gridSizer->SetCols(columns);

        // Clear growable columns only if they exist
        for (int i = 0; i < m_gridSizer->GetCols(); i++) {
            if (m_gridSizer->IsColGrowable(i)) {
                m_gridSizer->RemoveGrowableCol(i);
            }
        }

        // Set all columns as growable
        for (int i = 0; i < columns; i++) {
            m_gridSizer->AddGrowableCol(i, 1);
        }

        Layout();
        Refresh();
    }
}

void DesktopTab::CreateDesktopTab() {
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Create scrolled window for vertical scrolling only
    m_scrolledWindow = new wxScrolledWindow(this, wxID_ANY);
    m_scrolledWindow->SetScrollRate(0, 10); // Only vertical scrolling
    m_scrolledWindow->SetBackgroundColour(wxColour(18, 18, 18));

    wxBoxSizer* scrolledSizer = new wxBoxSizer(wxVERTICAL);
    m_scrolledWindow->SetSizer(scrolledSizer);

    // Add a horizontal sizer for the text and reload button
    wxBoxSizer* headerSizer = new wxBoxSizer(wxHORIZONTAL);

    // Add a wxStaticText to display the text from the file
    m_textDisplay = new wxStaticText(m_scrolledWindow, wxID_ANY, "GUI Environment: Not detected");
    m_textDisplay->SetForegroundColour(*wxWHITE);
    m_textDisplay->SetFont(wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    headerSizer->Add(m_textDisplay, 1, wxALIGN_LEFT | wxALL, 10);

    // Add a reload button
    m_reloadButton = new wxButton(m_scrolledWindow, wxID_ANY, "Reload UI", 
                                 wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    m_reloadButton->SetForegroundColour(*wxWHITE);
    m_reloadButton->SetBackgroundColour(wxColour(60, 60, 60));
    headerSizer->Add(m_reloadButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);

    // Bind event for reload button
    m_reloadButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        UpdateTextFromFile();
    });

    scrolledSizer->Add(headerSizer, 0, wxEXPAND);

    const std::array<wxString, 9> environments = {
        "GNOME", "KDE Plasma", "XFCE",
        "Cinnamon", "MATE", "LXQt",
        "Budgie", "Deepin", "Pantheon"
    };

    const std::array<wxString, 9> imageFiles = {
        "GNOME.png", "KDE.png", "Xfce.png",
        "Cinnamon.png", "MATE.png", "LXQt.png",
        "Budgie.png", "Deepin.png", "Pantheon.png"
    };

    // Initialize grid sizer with default 3 columns
    m_gridSizer = new wxFlexGridSizer(3, 20, 20);
    m_gridSizer->SetFlexibleDirection(wxBOTH);

    for (size_t i = 0; i < environments.size(); ++i) {
        wxPanel* card = new wxPanel(m_scrolledWindow);
        card->SetBackgroundColour(wxColour(32, 32, 32));
        card->SetMinSize(wxSize(200, 300)); // Minimum card size

        wxBoxSizer* cardSizer = new wxBoxSizer(wxVERTICAL);

        // Image container
        wxPanel* imageContainer = new wxPanel(card, wxID_ANY);
        imageContainer->SetMinSize(wxSize(180, 240));
        imageContainer->SetBackgroundColour(wxColour(32, 32, 32));

        wxBitmap bitmap = LoadImage(imageFiles[i]);
        if (bitmap.IsOk()) {
            imageContainer->Bind(wxEVT_PAINT, [bitmap](wxPaintEvent& evt) {
                wxPanel* panel = dynamic_cast<wxPanel*>(evt.GetEventObject());
                wxPaintDC dc(panel);

                wxSize panelSize = panel->GetSize();
                wxSize bmpSize = bitmap.GetSize();

                double scale = std::min(
                    static_cast<double>(panelSize.GetWidth()) / bmpSize.GetWidth(),
                    static_cast<double>(panelSize.GetHeight()) / bmpSize.GetHeight()
                );

                int newWidth = bmpSize.GetWidth() * scale;
                int newHeight = bmpSize.GetHeight() * scale;

                int x = (panelSize.GetWidth() - newWidth) / 2;
                int y = (panelSize.GetHeight() - newHeight) / 2;

                wxImage img = bitmap.ConvertToImage();
                dc.DrawBitmap(
                    wxBitmap(img.Scale(newWidth, newHeight, wxIMAGE_QUALITY_HIGH)),
                    x, y
                );
            });
        }

        cardSizer->Add(imageContainer, 1, wxEXPAND | wxALL, 10);

        // Title
        wxStaticText* title = new wxStaticText(card, wxID_ANY, environments[i]);
        title->SetForegroundColour(*wxWHITE);
        title->SetFont(wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
        cardSizer->Add(title, 0, wxALIGN_LEFT | wxLEFT | wxRIGHT, 10);

        // Achievements
        wxStaticText* info = new wxStaticText(card, wxID_ANY, "0/100 Achievements");
        info->SetForegroundColour(wxColour(180, 180, 180));
        info->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        cardSizer->Add(info, 0, wxALIGN_LEFT | wxLEFT | wxRIGHT | wxTOP, 10);

        // Install button
        wxButton* installButton = new wxButton(card, wxID_ANY, "Install");
        installButton->SetMinSize(wxSize(80, 30));
        installButton->SetBackgroundColour(wxColour(0, 120, 215));
        installButton->SetForegroundColour(*wxWHITE);
        cardSizer->Add(installButton, 0, wxALIGN_LEFT | wxALL, 10);

        card->SetSizer(cardSizer);
        m_gridSizer->Add(card, 1, wxEXPAND);
    }

    scrolledSizer->Add(m_gridSizer, 1, wxEXPAND | wxALL, 20);
    mainSizer->Add(m_scrolledWindow, 1, wxEXPAND);
    SetSizer(mainSizer);

    // Initial layout calculation
    CallAfter(&DesktopTab::RecalculateLayout, GetSize().GetWidth());
}

void DesktopTab::UpdateGUILabel(const wxString& guiName) {
    wxLogDebug("DesktopTab::UpdateGUILabel - Setting label with GUI name: %s", guiName);
    wxString displayText = "GUI Environment: " + guiName;
    m_textDisplay->SetLabel(displayText);
    
    // Force refresh and layout updates
    m_textDisplay->Refresh();
    m_scrolledWindow->Layout();
    
    wxLogDebug("DesktopTab::UpdateGUILabel - Label updated to: %s", m_textDisplay->GetLabel());
    
    // Force more aggressive UI updates
    Refresh(true);
    Update();
    GetParent()->Refresh(true);
    GetParent()->Update();
}

void DesktopTab::OnFileCopyComplete(wxCommandEvent& event) {
    wxString guiName = event.GetString();
    wxLogDebug("DesktopTab::OnFileCopyComplete - Event received with GUI name: %s", guiName);
    
    guiName.Trim(true).Trim(false); // Clean again for safety

    if (guiName.IsEmpty()) {
        wxLogDebug("DesktopTab::OnFileCopyComplete - GUI name is empty, setting to 'Not detected'");
        UpdateGUILabel("Not detected");
    } else {
        wxLogDebug("DesktopTab::OnFileCopyComplete - Updating GUI label to: %s", guiName);
        UpdateGUILabel(guiName);
    }
}

void DesktopTab::UpdateTextFromFile() {
    wxLogDebug("Reload UI button clicked. Initiating UI reload.");
    
    // First try to get from SecondWindow parent if possible
    wxString guiFilePath;
    wxWindow* parent = GetParent();
    while (parent) {
        // Try to cast to SecondWindow
        SecondWindow* secondWindow = wxDynamicCast(parent, SecondWindow);
        if (secondWindow) {
            guiFilePath = wxFileName(secondWindow->GetProjectDir(), "detected_gui.txt").GetFullPath();
            wxLogDebug("Found SecondWindow parent, using project dir: %s", secondWindow->GetProjectDir());
            break;
        }
        parent = parent->GetParent();
    }
    
    // If we couldn't get from SecondWindow, try current working directory
    if (guiFilePath.IsEmpty()) {
        guiFilePath = wxFileName(wxGetCwd(), "detected_gui.txt").GetFullPath();
        wxLogDebug("No SecondWindow parent found, using current dir: %s", wxGetCwd());
    }
    
    wxLogDebug("DesktopTab::UpdateTextFromFile - Looking for file: %s", guiFilePath);
    
    if (wxFileExists(guiFilePath)) {
        wxFile file(guiFilePath);
        wxString content;
        if (file.ReadAll(&content)) {
            content.Trim(true).Trim(false);
            wxLogDebug("DesktopTab::UpdateTextFromFile - Read content: %s", content);
            
            UpdateGUILabel(content);
        } else {
            wxLogDebug("DesktopTab::UpdateTextFromFile - Failed to read file contents");
            UpdateGUILabel("Not detected");
        }
    } else {
        wxLogDebug("DesktopTab::UpdateTextFromFile - File not found: %s", guiFilePath);
        UpdateGUILabel("Not detected");
    }
}
