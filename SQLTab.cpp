#include "SQLTab.h"

wxBEGIN_EVENT_TABLE(SQLTab, wxPanel)
    EVT_BUTTON(ID_SQL_DESKTOP, SQLTab::OnSQLTabChanged)
    EVT_BUTTON(ID_SQL_APPS, SQLTab::OnSQLTabChanged)
    EVT_BUTTON(ID_SQL_SYSTEM, SQLTab::OnSQLTabChanged)
    EVT_BUTTON(ID_SQL_CUSTOMIZE, SQLTab::OnSQLTabChanged)
    EVT_BUTTON(ID_SQL_HARDWARE, SQLTab::OnSQLTabChanged)
wxEND_EVENT_TABLE()

SQLTab::SQLTab(wxWindow* parent) : wxPanel(parent) {
    wxBoxSizer* contentSizer = new wxBoxSizer(wxVERTICAL);

    wxPanel* tabPanel = new wxPanel(this);
    tabPanel->SetBackgroundColour(wxColour(26, 26, 26));
    wxBoxSizer* tabSizer = new wxBoxSizer(wxHORIZONTAL);

    const struct {
        wxString label;
        int id;
    } tabs[] = {
        {"Desktop", ID_SQL_DESKTOP},
        {"Applications", ID_SQL_APPS},
        {"System", ID_SQL_SYSTEM},
        {"Customize", ID_SQL_CUSTOMIZE},
        {"Hardware", ID_SQL_HARDWARE}
    };

    m_sqlTabButtons.clear();

    for (const auto& tab : tabs) {
        wxButton* tabButton = new wxButton(tabPanel, tab.id, tab.label,
                                           wxDefaultPosition, wxSize(120, 50),
                                           wxBORDER_NONE);
        tabButton->SetBackgroundColour(wxColour(26, 26, 26));
        tabButton->SetForegroundColour(wxColour(156, 163, 175));
        tabSizer->Add(tabButton, 0, wxEXPAND);

        tabButton->Bind(wxEVT_BUTTON, &SQLTab::OnSQLTabChanged, this);
        m_sqlTabButtons.push_back(tabButton);
    }

    tabPanel->SetSizer(tabSizer);
    contentSizer->Add(tabPanel, 0, wxEXPAND);

    m_sqlContent = new wxPanel(this);
    m_sqlContent->SetBackgroundColour(wxColour(17, 24, 39));
    contentSizer->Add(m_sqlContent, 1, wxEXPAND | wxALL, 10);

    this->SetSizer(contentSizer);

    m_currentSqlTab = ID_SQL_DESKTOP;
    CreateDesktopTab();
}

void SQLTab::ShowTab(int tabId) {
    wxWindowList children = m_sqlContent->GetChildren();
    for (wxWindowList::iterator it = children.begin(); it != children.end(); ++it) {
        (*it)->Destroy();
    }

    if (m_sqlContent->GetSizer()) {
        m_sqlContent->GetSizer()->Clear(true);
        m_sqlContent->SetSizer(nullptr);
    }

    m_currentSqlTab = tabId;

    switch (tabId) {
        case ID_SQL_DESKTOP:
            CreateDesktopTab();
            break;
        case ID_SQL_APPS:
            CreateAppsTab();
            break;
        case ID_SQL_SYSTEM:
            CreateSystemTab();
            break;
        case ID_SQL_CUSTOMIZE:
            CreateCustomizeTab();
            break;
        case ID_SQL_HARDWARE:
            CreateHardwareTab();
            break;
    }

    m_sqlContent->Layout();
}

void SQLTab::OnSQLTabChanged(wxCommandEvent& event) {
    for (wxButton* btn : m_sqlTabButtons) {
        if (btn->GetId() == event.GetId()) {
            btn->SetBackgroundColour(wxColour(44, 49, 58));
            btn->SetForegroundColour(*wxWHITE);
        } else {
            btn->SetBackgroundColour(wxColour(26, 26, 26));
            btn->SetForegroundColour(wxColour(156, 163, 175));
        }
        btn->Refresh();
    }

    ShowTab(event.GetId());
}

void SQLTab::CreateDesktopTab() {
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    wxStaticBox* box = new wxStaticBox(m_sqlContent, wxID_ANY, "Desktop Environment");
    box->SetForegroundColour(*wxWHITE);
    wxStaticBoxSizer* envSizer = new wxStaticBoxSizer(box, wxVERTICAL);

    wxGridSizer* gridSizer = new wxGridSizer(3, 3, 10, 10);

    const wxString environments[] = {
        "GNOME", "KDE Plasma", "XFCE",
        "Cinnamon", "MATE", "LXQt",
        "Budgie", "Deepin", "Pantheon"
    };

    for (const auto& env : environments) {
        wxPanel* card = new wxPanel(m_sqlContent);
        card->SetBackgroundColour(wxColour(31, 41, 55));

        wxBoxSizer* cardSizer = new wxBoxSizer(wxVERTICAL);

        wxPanel* preview = new wxPanel(card, wxID_ANY, wxDefaultPosition, wxSize(-1, 100));
        preview->SetBackgroundColour(wxColour(55, 65, 81));

        wxStaticText* label = new wxStaticText(card, wxID_ANY, env);
        label->SetForegroundColour(*wxWHITE);

        cardSizer->Add(preview, 1, wxEXPAND | wxALL, 5);
        cardSizer->Add(label, 0, wxALL, 5);

        card->SetSizer(cardSizer);
        gridSizer->Add(card, 1, wxEXPAND);
    }

    envSizer->Add(gridSizer, 1, wxEXPAND | wxALL, 5);
    sizer->Add(envSizer, 1, wxEXPAND | wxALL, 10);

    m_sqlContent->SetSizer(sizer);
}

void SQLTab::CreateAppsTab() {
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    wxBoxSizer* searchSizer = new wxBoxSizer(wxHORIZONTAL);

    wxTextCtrl* search = new wxTextCtrl(m_sqlContent, wxID_ANY, "Search applications...");
    search->SetBackgroundColour(wxColour(31, 41, 55));
    search->SetForegroundColour(*wxWHITE);

    wxChoice* category = new wxChoice(m_sqlContent, wxID_ANY);
    category->Append({"All Categories", "Internet", "Graphics", "Development", "Games"});
    category->SetSelection(0);

    searchSizer->Add(search, 1, wxRIGHT, 5);
    searchSizer->Add(category, 0);

    wxStaticBox* box = new wxStaticBox(m_sqlContent, wxID_ANY, "Featured Applications");
    box->SetForegroundColour(*wxWHITE);
    wxStaticBoxSizer* appsSizer = new wxStaticBoxSizer(box, wxVERTICAL);

    wxGridSizer* grid = new wxGridSizer(2, 4, 10, 10);

    const wxString apps[] = {
        "Firefox", "VS Code", "GIMP", "Steam",
        "Blender", "VLC", "Discord", "OBS Studio"
    };

    for (const auto& app : apps) {
        wxPanel* card = new wxPanel(m_sqlContent);
        card->SetBackgroundColour(wxColour(31, 41, 55));

        wxBoxSizer* cardSizer = new wxBoxSizer(wxVERTICAL);

        wxPanel* icon = new wxPanel(card, wxID_ANY, wxDefaultPosition, wxSize(48, 48));
        icon->SetBackgroundColour(wxColour(55, 65, 81));

        wxStaticText* name = new wxStaticText(card, wxID_ANY, app);
        name->SetForegroundColour(*wxWHITE);

        wxButton* install = new wxButton(card, wxID_ANY, "Install");
        install->SetBackgroundColour(wxColour(37, 99, 235));
        install->SetForegroundColour(*wxWHITE);

        cardSizer->Add(icon, 0, wxALIGN_CENTER | wxALL, 5);
        cardSizer->Add(name, 0, wxALL, 5);
        cardSizer->Add(install, 0, wxEXPAND | wxALL, 5);

        card->SetSizer(cardSizer);
        grid->Add(card, 1, wxEXPAND);
    }

    appsSizer->Add(grid, 1, wxEXPAND | wxALL, 5);

    sizer->Add(searchSizer, 0, wxEXPAND | wxALL, 10);
    sizer->Add(appsSizer, 1, wxEXPAND | wxALL, 10);

    m_sqlContent->SetSizer(sizer);
}

void SQLTab::CreateSystemTab() {
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    wxStaticBox* box = new wxStaticBox(m_sqlContent, wxID_ANY, "System Configuration");
    box->SetForegroundColour(*wxWHITE);
    wxStaticBoxSizer* configSizer = new wxStaticBoxSizer(box, wxVERTICAL);

    wxFlexGridSizer* grid = new wxFlexGridSizer(2, 2, 10, 10);
    grid->AddGrowableCol(1, 1);

    auto hostnameLabel = new wxStaticText(m_sqlContent, wxID_ANY, "Hostname");
    hostnameLabel->SetForegroundColour(*wxWHITE);
    auto hostnameInput = new wxTextCtrl(m_sqlContent, wxID_ANY);
    hostnameInput->SetBackgroundColour(wxColour(31, 41, 55));
    hostnameInput->SetForegroundColour(*wxWHITE);

    grid->Add(hostnameLabel, 0, wxALIGN_CENTER_VERTICAL);
    grid->Add(hostnameInput, 1, wxEXPAND);

    auto langLabel = new wxStaticText(m_sqlContent, wxID_ANY, "Language");
    langLabel->SetForegroundColour(*wxWHITE);
    wxChoice* lang = new wxChoice(m_sqlContent, wxID_ANY);
    lang->Append({"English (US)", "English (UK)", "Spanish", "French"});
    lang->SetSelection(0);

    grid->Add(langLabel, 0, wxALIGN_CENTER_VERTICAL);
    grid->Add(lang, 1, wxEXPAND);

    configSizer->Add(grid, 1, wxEXPAND | wxALL, 5);
    sizer->Add(configSizer, 0, wxEXPAND | wxALL, 10);

    m_sqlContent->SetSizer(sizer);
}

void SQLTab::CreateCustomizeTab() {
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    wxStaticBox* themeBox = new wxStaticBox(m_sqlContent, wxID_ANY, "Theme");
    themeBox->SetForegroundColour(*wxWHITE);
    wxStaticBoxSizer* themeSizer = new wxStaticBoxSizer(themeBox, wxVERTICAL);

    wxFlexGridSizer* themeGrid = new wxFlexGridSizer(2, 2, 10, 10);
    themeGrid->AddGrowableCol(1, 1);

    auto themeLabel = new wxStaticText(m_sqlContent, wxID_ANY, "Global Theme");
    themeLabel->SetForegroundColour(*wxWHITE);
    wxChoice* theme = new wxChoice(m_sqlContent, wxID_ANY);
    theme->Append({"Default", "Nord", "Dracula", "Gruvbox", "Tokyo Night", "Catppuccin"});
    theme->SetSelection(0);

    themeGrid->Add(themeLabel, 0, wxALIGN_CENTER_VERTICAL);
    themeGrid->Add(theme, 1, wxEXPAND);

    auto accentLabel = new wxStaticText(m_sqlContent, wxID_ANY, "Accent Color");
    accentLabel->SetForegroundColour(*wxWHITE);
    wxPanel* colorPanel = new wxPanel(m_sqlContent);
    colorPanel->SetBackgroundColour(wxColour(37, 99, 235));

    themeGrid->Add(accentLabel, 0, wxALIGN_CENTER_VERTICAL);
    themeGrid->Add(colorPanel, 1, wxEXPAND);

    themeSizer->Add(themeGrid, 1, wxEXPAND | wxALL, 5);
    sizer->Add(themeSizer, 0, wxEXPAND | wxALL, 10);

    m_sqlContent->SetSizer(sizer);
}

void SQLTab::CreateHardwareTab() {
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    wxStaticBox* box = new wxStaticBox(m_sqlContent, wxID_ANY, "Display");
    box->SetForegroundColour(*wxWHITE);
    wxStaticBoxSizer* displaySizer = new wxStaticBoxSizer(box, wxVERTICAL);

    wxFlexGridSizer* grid = new wxFlexGridSizer(2, 2, 10, 10);
    grid->AddGrowableCol(1, 1);

    auto resLabel = new wxStaticText(m_sqlContent, wxID_ANY, "Resolution");
    resLabel->SetForegroundColour(*wxWHITE);
    wxChoice* res = new wxChoice(m_sqlContent, wxID_ANY);
    res->Append({"3840 x 2160", "2560 x 1440", "1920 x 1080"});
    res->SetSelection(2);

    grid->Add(resLabel, 0, wxALIGN_CENTER_VERTICAL);
    grid->Add(res, 1, wxEXPAND);

    auto rateLabel = new wxStaticText(m_sqlContent, wxID_ANY, "Refresh Rate");
    rateLabel->SetForegroundColour(*wxWHITE);
    wxChoice* rate = new wxChoice(m_sqlContent, wxID_ANY);
    rate->Append({"144 Hz", "120 Hz", "60 Hz"});
    rate->SetSelection(2);

    grid->Add(rateLabel, 0, wxALIGN_CENTER_VERTICAL);
    grid->Add(rate, 1, wxEXPAND);

    displaySizer->Add(grid, 1, wxEXPAND | wxALL, 5);
    sizer->Add(displaySizer, 0, wxEXPAND | wxALL, 10);

    m_sqlContent->SetSizer(sizer);
}