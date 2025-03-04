#include "SQLTab.h"
#include "FlatpakStore.h"
#include "DesktopTab.h"
#include "CustomizeTab.h" // Add new header
#include "SecondWindow.h"  // Added to provide full definition of SecondWindow

wxBEGIN_EVENT_TABLE(SQLTab, wxPanel)
    EVT_BUTTON(ID_SQL_DESKTOP, SQLTab::OnSQLTabChanged)
    EVT_BUTTON(ID_SQL_APPS, SQLTab::OnSQLTabChanged)
    EVT_BUTTON(ID_SQL_SYSTEM, SQLTab::OnSQLTabChanged)
    EVT_BUTTON(ID_SQL_CUSTOMIZE, SQLTab::OnSQLTabChanged)
wxEND_EVENT_TABLE()

SQLTab::SQLTab(wxWindow* parent, const wxString& workDir) 
    : wxPanel(parent), m_workDir(workDir)
{
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
        {"Customize", ID_SQL_CUSTOMIZE}
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
    // Destroy all existing children of m_sqlContent to prevent conflicts
    m_sqlContent->DestroyChildren();

    // Clear and delete the old sizer, if any, to avoid sizer reuse issues
    if (m_sqlContent->GetSizer()) {
        m_sqlContent->SetSizer(nullptr, true); // true deletes the old sizer
    }

    // Update the current tab ID
    m_currentSqlTab = tabId;

    // Switch to the appropriate tab and create its content
    switch (tabId) {
        case ID_SQL_DESKTOP:
            CreateDesktopTab();
            // Trigger a size event to force layout recalculation
            if (!m_sqlContent->GetChildren().IsEmpty()) {
                wxWindow* desktopTab = m_sqlContent->GetChildren().GetFirst()->GetData();
                wxSizeEvent event(desktopTab->GetSize());
                desktopTab->GetEventHandler()->ProcessEvent(event);
            }
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

        default:
            wxLogDebug("Unknown tab ID: %d", tabId);
            break;
    }

    // Ensure the layout is updated after adding new content
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
    DesktopTab* desktopTab = new DesktopTab(m_sqlContent);
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(desktopTab, 1, wxEXPAND);
    m_sqlContent->SetSizer(sizer);
    
    // Force immediate layout calculation
    m_sqlContent->Layout();
    
    // Use CallAfter to ensure proper sizing after layout
    CallAfter([this, desktopTab]() {
        desktopTab->RecalculateLayout(m_sqlContent->GetSize().GetWidth());
        m_sqlContent->Layout();
    });
}

void SQLTab::CreateAppsTab() {
    // Create a new vertical sizer for the tab content
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    // Create a new FlatpakStore instance with m_sqlContent as its parent
    FlatpakStore* flatpakStore = new FlatpakStore(m_sqlContent, m_workDir);
    flatpakStore->SetContainerId(m_containerId); // Apply the container ID
    wxLogDebug("Created new FlatpakStore for Apps tab with container ID: %s", m_containerId);

    // Add the FlatpakStore panel to the sizer with expansion and padding
    sizer->Add(flatpakStore, 1, wxEXPAND | wxALL, 10);

    // Set the sizer to m_sqlContent and ensure proper layout
    m_sqlContent->SetSizer(sizer);
    m_sqlContent->Layout();

    // Optional: Verify the parent hierarchy for debugging
    wxLogDebug("Parent of FlatpakStore: %p, m_sqlContent: %p", flatpakStore->GetParent(), m_sqlContent);
}

void SQLTab::SetContainerId(const wxString& containerId) {
    m_containerId = containerId;
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
    CustomizeTab* customizeTab = new CustomizeTab(m_sqlContent);
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(customizeTab, 1, wxEXPAND);
    m_sqlContent->SetSizer(sizer);
    m_sqlContent->Layout();
}
