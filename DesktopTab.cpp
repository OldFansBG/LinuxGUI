#include "DesktopTab.h"
#include <wx/filename.h>
#include <array>

DesktopTab::DesktopTab(wxWindow* parent) : wxPanel(parent) {
    CreateDesktopTab();
    Bind(wxEVT_SIZE, &DesktopTab::OnSize, this);
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