#include "ConfigTab.h"
#include "FlatpakStore.h"
#include "DesktopTab.h"
#include "CustomizeTab.h" // Add new header
#include "SecondWindow.h" // Added to provide full definition of SecondWindow
#include "SystemTab.h" // Add this include at the top with other includes

wxBEGIN_EVENT_TABLE(SQLTab, wxPanel)
    EVT_BUTTON(ID_CONFIG_DESKTOP, SQLTab::OnConfigTabChanged)
        EVT_BUTTON(ID_CONFIG_APPS, SQLTab::OnConfigTabChanged)
            EVT_BUTTON(ID_CONFIG_SYSTEM, SQLTab::OnConfigTabChanged)
                EVT_BUTTON(ID_CONFIG_CUSTOMIZE, SQLTab::OnConfigTabChanged)
                    wxEND_EVENT_TABLE()

                        SQLTab::SQLTab(wxWindow *parent, const wxString &workDir)
    : wxPanel(parent), m_workDir(workDir)
{
    wxBoxSizer *contentSizer = new wxBoxSizer(wxVERTICAL);

    wxPanel *tabPanel = new wxPanel(this);
    tabPanel->SetBackgroundColour(wxColour(26, 26, 26));
    wxBoxSizer *tabSizer = new wxBoxSizer(wxHORIZONTAL);

    const struct
    {
        wxString label;
        int id;
    } tabs[] = {
        {"Desktop", ID_CONFIG_DESKTOP},
        {"Applications", ID_CONFIG_APPS},
        {"System", ID_CONFIG_SYSTEM},
        {"Customize", ID_CONFIG_CUSTOMIZE}};

    m_configTabButtons.clear();

    for (const auto &tab : tabs)
    {
        wxButton *tabButton = new wxButton(tabPanel, tab.id, tab.label,
                                           wxDefaultPosition, wxSize(120, 50),
                                           wxBORDER_NONE);
        tabButton->SetBackgroundColour(wxColour(26, 26, 26));
        tabButton->SetForegroundColour(wxColour(156, 163, 175));
        tabSizer->Add(tabButton, 0, wxEXPAND);

        tabButton->Bind(wxEVT_BUTTON, &SQLTab::OnConfigTabChanged, this);
        m_configTabButtons.push_back(tabButton);
    }

    tabPanel->SetSizer(tabSizer);
    contentSizer->Add(tabPanel, 0, wxEXPAND);

    m_configContent = new wxPanel(this);
    m_configContent->SetBackgroundColour(wxColour(17, 24, 39));
    contentSizer->Add(m_configContent, 1, wxEXPAND | wxALL, 10);

    this->SetSizer(contentSizer);

    m_currentConfigTab = ID_CONFIG_DESKTOP;
    CreateDesktopTab();
}

void SQLTab::ShowTab(int tabId)
{
    // Destroy all existing children of m_configContent to prevent conflicts
    m_configContent->DestroyChildren();

    // Clear and delete the old sizer, if any, to avoid sizer reuse issues
    if (m_configContent->GetSizer())
    {
        m_configContent->SetSizer(nullptr, true); // true deletes the old sizer
    }

    // Update the current tab ID
    m_currentConfigTab = tabId;

    // Switch to the appropriate tab and create its content
    switch (tabId)
    {
    case ID_CONFIG_DESKTOP:
        CreateDesktopTab();
        // Trigger a size event to force layout recalculation
        if (!m_configContent->GetChildren().IsEmpty())
        {
            wxWindow *desktopTab = m_configContent->GetChildren().GetFirst()->GetData();
            wxSizeEvent event(desktopTab->GetSize());
            desktopTab->GetEventHandler()->ProcessEvent(event);
        }
        break;

    case ID_CONFIG_APPS:
        CreateAppsTab();
        break;

    case ID_CONFIG_SYSTEM:
        CreateSystemTab();
        break;

    case ID_CONFIG_CUSTOMIZE:
        CreateCustomizeTab();
        break;

    default:
        wxLogDebug("Unknown tab ID: %d", tabId);
        break;
    }

    // Ensure the layout is updated after adding new content
    m_configContent->Layout();
}

void SQLTab::OnConfigTabChanged(wxCommandEvent &event)
{
    for (wxButton *btn : m_configTabButtons)
    {
        if (btn->GetId() == event.GetId())
        {
            btn->SetBackgroundColour(wxColour(44, 49, 58));
            btn->SetForegroundColour(*wxWHITE);
        }
        else
        {
            btn->SetBackgroundColour(wxColour(26, 26, 26));
            btn->SetForegroundColour(wxColour(156, 163, 175));
        }
        btn->Refresh();
    }

    ShowTab(event.GetId());
}

void SQLTab::CreateDesktopTab()
{
    DesktopTab *desktopTab = new DesktopTab(m_configContent);
    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(desktopTab, 1, wxEXPAND);
    m_configContent->SetSizer(sizer);

    // Force immediate layout calculation
    m_configContent->Layout();

    // Use CallAfter to ensure proper sizing after layout
    CallAfter([this, desktopTab]()
              {
        desktopTab->RecalculateLayout(m_configContent->GetSize().GetWidth());
        m_configContent->Layout(); });
}

void SQLTab::CreateAppsTab()
{
    // Create a new vertical sizer for the tab content
    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);

    // Create a new FlatpakStore instance with m_configContent as its parent
    FlatpakStore *flatpakStore = new FlatpakStore(m_configContent, m_workDir);
    flatpakStore->SetContainerId(m_containerId); // Apply the container ID
    wxLogDebug("Created new FlatpakStore for Apps tab with container ID: %s", m_containerId);

    // Add the FlatpakStore panel to the sizer with expansion and padding
    sizer->Add(flatpakStore, 1, wxEXPAND | wxALL, 10);

    // Set the sizer to m_configContent and ensure proper layout
    m_configContent->SetSizer(sizer);
    m_configContent->Layout();

    // Optional: Verify the parent hierarchy for debugging
    wxLogDebug("Parent of FlatpakStore: %p, m_configContent: %p", flatpakStore->GetParent(), m_configContent);
}

void SQLTab::SetContainerId(const wxString &containerId)
{
    m_containerId = containerId;
}

void SQLTab::CreateSystemTab()
{
    SystemTab* systemTab = new SystemTab(m_configContent);
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(systemTab, 1, wxEXPAND);
    m_configContent->SetSizer(sizer);
    m_configContent->Layout();
}

void SQLTab::CreateCustomizeTab()
{
    CustomizeTab *customizeTab = new CustomizeTab(m_configContent);
    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(customizeTab, 1, wxEXPAND);
    m_configContent->SetSizer(sizer);
    m_configContent->Layout();
    customizeTab->LoadWallpaper(); // Load the wallpaper after tab creation
}