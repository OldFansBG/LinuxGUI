#include "SecondWindow.h"
#include <wx/notebook.h>
#include <wx/statline.h>

wxBEGIN_EVENT_TABLE(SecondWindow, wxFrame)
    EVT_CLOSE(SecondWindow::OnClose)
    EVT_BUTTON(ID_NEXT_BUTTON, SecondWindow::OnNext)
    EVT_BUTTON(ID_TERMINAL_TAB, SecondWindow::OnTabChanged)
    EVT_BUTTON(ID_SQL_TAB, SecondWindow::OnTabChanged)
wxEND_EVENT_TABLE()

SecondWindow::SecondWindow(wxWindow* parent, const wxString& title, const wxString& isoPath)
    : wxFrame(parent, wxID_ANY, title, wxDefaultPosition, wxSize(800, 650)),
      m_isoPath(isoPath), m_cmdPanel(nullptr), m_terminalPanel(nullptr)
{
    CreateControls();
    Centre();
    SetBackgroundColour(wxColour(40, 44, 52));
}

SecondWindow::~SecondWindow()
{
    UnbindSQLEvents();
}

void SecondWindow::UnbindSQLEvents()
{
    for(wxButton* btn : m_sqlTabButtons) {
        if(btn) {
            btn->Unbind(wxEVT_BUTTON, &SecondWindow::OnSQLTabChanged, this);
        }
    }
}

void SecondWindow::CreateControls()
{
    m_mainPanel = new wxPanel(this);
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    if (!HasFlag(wxFRAME_NO_TASKBAR)) {
        wxPanel* titleBar = new wxPanel(m_mainPanel);
        titleBar->SetBackgroundColour(wxColour(26, 26, 26));
        wxBoxSizer* titleSizer = new wxBoxSizer(wxHORIZONTAL);
        wxStaticText* titleText = new wxStaticText(titleBar, wxID_ANY, "Terminal");
        titleText->SetForegroundColour(wxColour(229, 229, 229));
        titleSizer->Add(titleText, 0, wxALL | wxALIGN_CENTER_VERTICAL, 10);
        titleBar->SetSizer(titleSizer);
        mainSizer->Add(titleBar, 0, wxEXPAND);
    }

    wxPanel* tabPanel = new wxPanel(m_mainPanel);
    tabPanel->SetBackgroundColour(wxColour(30, 30, 30));
    wxBoxSizer* tabSizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* centeredTabSizer = new wxBoxSizer(wxHORIZONTAL);
    
    wxButton* terminalButton = new wxButton(tabPanel, ID_TERMINAL_TAB, "TERMINAL",
                                          wxDefaultPosition, wxSize(100, 40),
                                          wxBORDER_NONE);
    terminalButton->SetBackgroundColour(wxColour(44, 49, 58));
    terminalButton->SetForegroundColour(*wxWHITE);

    wxStaticLine* separator = new wxStaticLine(tabPanel, wxID_ANY, 
                                             wxDefaultPosition, wxDefaultSize,
                                             wxLI_VERTICAL);
    separator->SetBackgroundColour(wxColour(64, 64, 64));

    wxButton* sqlButton = new wxButton(tabPanel, ID_SQL_TAB, "SQL",
                                     wxDefaultPosition, wxSize(100, 40),
                                     wxBORDER_NONE);
    sqlButton->SetBackgroundColour(wxColour(30, 30, 30));
    sqlButton->SetForegroundColour(wxColour(128, 128, 128));

    centeredTabSizer->Add(terminalButton, 0, wxEXPAND);
    centeredTabSizer->Add(separator, 0, wxEXPAND | wxTOP | wxBOTTOM, 5);
    centeredTabSizer->Add(sqlButton, 0, wxEXPAND);

    tabSizer->AddStretchSpacer(1);
    tabSizer->Add(centeredTabSizer, 0, wxALIGN_CENTER);
    tabSizer->AddStretchSpacer(1);

    tabPanel->SetSizer(tabSizer);

    m_terminalTab = new wxPanel(m_mainPanel);
    m_sqlTab = new wxPanel(m_mainPanel);
    
    m_terminalSizer = new wxBoxSizer(wxVERTICAL);
    m_sqlSizer = new wxBoxSizer(wxVERTICAL);

    m_terminalTab->SetBackgroundColour(wxColour(30, 30, 30));
    m_sqlTab->SetBackgroundColour(wxColour(30, 30, 30));

    OSDetector::OS currentOS = m_osDetector.GetCurrentOS();
    if (currentOS == OSDetector::OS::Windows) {
        m_cmdPanel = new WindowsCmdPanel(m_terminalTab);
        m_cmdPanel->SetISOPath(m_isoPath);
        m_terminalSizer->Add(m_cmdPanel, 1, wxEXPAND | wxALL, 5);
    } else {
        m_terminalPanel = new LinuxTerminalPanel(m_terminalTab);
        m_terminalPanel->SetMinSize(wxSize(680, 400));
        m_terminalSizer->Add(m_terminalPanel, 1, wxEXPAND | wxALL, 5);
    }

    CreateSQLPanel();

    m_terminalTab->SetSizer(m_terminalSizer);
    m_sqlTab->SetSizer(m_sqlSizer);

    m_sqlTab->Hide();
    m_terminalTab->Show();

    wxPanel* buttonPanel = new wxPanel(m_mainPanel);
    buttonPanel->SetBackgroundColour(wxColour(30, 30, 30));
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    
    wxButton* nextButton = new wxButton(buttonPanel, ID_NEXT_BUTTON, "Next",
                                      wxDefaultPosition, wxSize(120, 35));
    nextButton->SetBackgroundColour(wxColour(189, 147, 249));
    nextButton->SetForegroundColour(*wxWHITE);

    buttonSizer->AddStretchSpacer(1);
    buttonSizer->Add(nextButton, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    buttonPanel->SetSizer(buttonSizer);

    mainSizer->Add(tabPanel, 0, wxEXPAND);
    mainSizer->Add(m_terminalTab, 1, wxEXPAND);
    mainSizer->Add(m_sqlTab, 1, wxEXPAND);
    mainSizer->Add(buttonPanel, 0, wxEXPAND);

    m_mainPanel->SetSizer(mainSizer);
    SetMinSize(wxSize(800, 600));
}
void SecondWindow::OnClose(wxCloseEvent& event)
{
    if (GetParent()) {
        GetParent()->Show();
    }
    Destroy();
}

void SecondWindow::OnTabChanged(wxCommandEvent& event)
{
    if (event.GetId() == ID_TERMINAL_TAB) {
        m_terminalTab->Show();
        m_sqlTab->Hide();
        
        wxButton* terminalButton = wxDynamicCast(FindWindow(ID_TERMINAL_TAB), wxButton);
        wxButton* sqlButton = wxDynamicCast(FindWindow(ID_SQL_TAB), wxButton);
        
        if(terminalButton && sqlButton) {
            terminalButton->SetBackgroundColour(wxColour(44, 49, 58));
            terminalButton->SetForegroundColour(*wxWHITE);
            sqlButton->SetBackgroundColour(wxColour(30, 30, 30));
            sqlButton->SetForegroundColour(wxColour(128, 128, 128));
        }
    } else {
        m_terminalTab->Hide();
        m_sqlTab->Show();
        
        wxButton* terminalButton = wxDynamicCast(FindWindow(ID_TERMINAL_TAB), wxButton);
        wxButton* sqlButton = wxDynamicCast(FindWindow(ID_SQL_TAB), wxButton);
        
        if(terminalButton && sqlButton) {
            terminalButton->SetBackgroundColour(wxColour(30, 30, 30));
            terminalButton->SetForegroundColour(wxColour(128, 128, 128));
            sqlButton->SetBackgroundColour(wxColour(44, 49, 58));
            sqlButton->SetForegroundColour(*wxWHITE);
        }
    }
    
    m_mainPanel->Layout();
}

void SecondWindow::OnNext(wxCommandEvent& event)
{
    wxString currentTab = m_terminalTab->IsShown() ? "Terminal" : "SQL";
    wxMessageBox(wxString::Format("Moving to next step from %s tab...", currentTab),
                "Next Step", wxOK | wxICON_INFORMATION);
}

void SecondWindow::CreateSQLPanel() 
{
    wxPanel* sqlContent = new wxPanel(m_sqlTab);
    sqlContent->SetBackgroundColour(wxColour(26, 26, 26));
    
    wxBoxSizer* contentSizer = new wxBoxSizer(wxVERTICAL);
    
    wxPanel* tabPanel = new wxPanel(sqlContent);
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
    
    for(const auto& tab : tabs) {
        wxButton* tabButton = new wxButton(tabPanel, tab.id, tab.label,
                                         wxDefaultPosition, wxSize(120, 50),
                                         wxBORDER_NONE);
        tabButton->SetBackgroundColour(wxColour(26, 26, 26));
        tabButton->SetForegroundColour(wxColour(156, 163, 175));
        tabSizer->Add(tabButton, 0, wxEXPAND);
        
        tabButton->Bind(wxEVT_BUTTON, &SecondWindow::OnSQLTabChanged, this);
        m_sqlTabButtons.push_back(tabButton);
    }
    
    tabPanel->SetSizer(tabSizer);
    contentSizer->Add(tabPanel, 0, wxEXPAND);
    
    m_sqlContent = new wxPanel(sqlContent);
    m_sqlContent->SetBackgroundColour(wxColour(17, 24, 39));
    contentSizer->Add(m_sqlContent, 1, wxEXPAND | wxALL, 10);
    
    sqlContent->SetSizer(contentSizer);
    m_sqlSizer->Add(sqlContent, 1, wxEXPAND);

    m_currentSqlTab = ID_SQL_DESKTOP;
    CreateDesktopTab();
}

void SecondWindow::OnSQLTabChanged(wxCommandEvent& event) 
{
    // Update button colors
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

    ShowSQLTab(event.GetId());
}

void SecondWindow::ShowSQLTab(int tabId) 
{
    // Destroy all existing children
    wxWindowList children = m_sqlContent->GetChildren();
    for (wxWindowList::iterator it = children.begin(); it != children.end(); ++it) {
        (*it)->Destroy();
    }
    
    // Clear any existing sizer
    if (m_sqlContent->GetSizer()) {
        m_sqlContent->GetSizer()->Clear(true);
        m_sqlContent->SetSizer(nullptr);
    }

    m_currentSqlTab = tabId;
    
    switch(tabId) {
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

void SecondWindow::CreateDesktopTab() 
{
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

    for(const auto& env : environments) {
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

void SecondWindow::CreateAppsTab() 
{
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
    
    for(const auto& app : apps) {
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

void SecondWindow::CreateSystemTab() 
{
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

void SecondWindow::CreateCustomizeTab() 
{
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

void SecondWindow::CreateHardwareTab() 
{
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