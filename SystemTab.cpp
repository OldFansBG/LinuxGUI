#include "SystemTab.h"
#include <vector> // For std::vector

// Define an ID for the lock root checkbox
const int ID_LOCK_ROOT_CHECKBOX = wxID_HIGHEST + 500;

wxBEGIN_EVENT_TABLE(SystemTab, wxPanel)
    EVT_CHECKBOX(ID_LOCK_ROOT_CHECKBOX, SystemTab::OnLockRootCheck)
        wxEND_EVENT_TABLE()

            SystemTab::SystemTab(wxWindow *parent)
    : wxPanel(parent),
      m_hostnameInput(nullptr), m_languageChoice(nullptr), m_keyboardLayoutChoice(nullptr),
      m_timezoneChoice(nullptr), m_usernameInput(nullptr), m_fullNameInput(nullptr),
      m_userPasswordInput(nullptr), m_confirmUserPasswordInput(nullptr), m_adminCheckbox(nullptr),
      m_lockRootCheckbox(nullptr), m_rootPasswordLabel(nullptr), m_rootPasswordInput(nullptr),
      m_confirmRootPasswordLabel(nullptr), m_confirmRootPasswordInput(nullptr), m_rootPasswordSizer(nullptr)
{
    // Initialize theme colors (adapt these or use ThemeConfig if available)
    m_bgColor = wxColour(18, 18, 27);         // Dark background for the whole tab
    m_panelBgColor = wxColour(30, 30, 46);    // Background for static boxes/cards
    m_textColor = wxColour(205, 214, 244);    // Main text color
    m_inputBgColor = wxColour(49, 50, 68);    // Background for inputs
    m_inputFgColor = wxColour(205, 214, 244); // Text color for inputs
    m_labelColor = wxColour(166, 173, 200);   // Lighter text for labels

    SetBackgroundColour(m_bgColor); // Set main background
    CreateControls();
    ApplyThemeColors(); // Apply colors after creation
}

void SystemTab::CreateControls()
{
    wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->AddSpacer(20); // Top padding

    // --- Localization Section ---
    wxStaticBox *localeBox = new wxStaticBox(this, wxID_ANY, "Localization");
    wxStaticBoxSizer *localeSizer = new wxStaticBoxSizer(localeBox, wxVERTICAL);
    wxPanel *localePanel = new wxPanel(localeBox); // Panel inside the box for padding
    wxBoxSizer *localePanelSizer = new wxBoxSizer(wxVERTICAL);

    wxFlexGridSizer *localeGrid = new wxFlexGridSizer(4, 2, 15, 25); // Rows, Cols, VGap, HGap
    localeGrid->AddGrowableCol(1, 1);                                // Make the input column growable

    // Hostname
    auto hostnameLabel = new wxStaticText(localePanel, wxID_ANY, "Hostname:");
    m_hostnameInput = new wxTextCtrl(localePanel, wxID_ANY, "custom-linux");
    localeGrid->Add(hostnameLabel, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    localeGrid->Add(m_hostnameInput, 1, wxEXPAND);

    // Language
    auto langLabel = new wxStaticText(localePanel, wxID_ANY, "Language:");
    m_languageChoice = new wxChoice(localePanel, wxID_ANY);
    std::vector<wxString> languages = {"en_US.UTF-8 (English - US)", "en_GB.UTF-8 (English - UK)", "es_ES.UTF-8 (Spanish)", "fr_FR.UTF-8 (French)", "de_DE.UTF-8 (German)"};
    m_languageChoice->Append(languages);
    m_languageChoice->SetSelection(0);
    localeGrid->Add(langLabel, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    localeGrid->Add(m_languageChoice, 1, wxEXPAND);

    // Keyboard Layout
    auto kbdLabel = new wxStaticText(localePanel, wxID_ANY, "Keyboard Layout:");
    m_keyboardLayoutChoice = new wxChoice(localePanel, wxID_ANY);
    std::vector<wxString> layouts = {"us (US English)", "gb (UK English)", "es (Spanish)", "fr (French)", "de (German)"};
    m_keyboardLayoutChoice->Append(layouts);
    m_keyboardLayoutChoice->SetSelection(0);
    localeGrid->Add(kbdLabel, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    localeGrid->Add(m_keyboardLayoutChoice, 1, wxEXPAND);

    // Timezone
    auto tzLabel = new wxStaticText(localePanel, wxID_ANY, "Timezone:");
    m_timezoneChoice = new wxChoice(localePanel, wxID_ANY);
    // Note: Populating this accurately is complex. Using placeholders.
    std::vector<wxString> timezones = {"UTC", "America/New_York", "Europe/London", "Europe/Paris", "Asia/Tokyo"};
    m_timezoneChoice->Append(timezones);
    m_timezoneChoice->SetSelection(0);
    localeGrid->Add(tzLabel, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    localeGrid->Add(m_timezoneChoice, 1, wxEXPAND);

    localePanelSizer->Add(localeGrid, 1, wxEXPAND | wxALL, 15); // Padding inside the panel
    localePanel->SetSizer(localePanelSizer);
    localeSizer->Add(localePanel, 1, wxEXPAND);
    mainSizer->Add(localeSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 20);

    // --- User Account Section ---
    wxStaticBox *userBox = new wxStaticBox(this, wxID_ANY, "Default User Account");
    wxStaticBoxSizer *userSizer = new wxStaticBoxSizer(userBox, wxVERTICAL);
    wxPanel *userPanel = new wxPanel(userBox);
    wxBoxSizer *userPanelSizer = new wxBoxSizer(wxVERTICAL);

    wxFlexGridSizer *userGrid = new wxFlexGridSizer(5, 2, 15, 25);
    userGrid->AddGrowableCol(1, 1);

    // Username
    auto userLabel = new wxStaticText(userPanel, wxID_ANY, "Username:");
    m_usernameInput = new wxTextCtrl(userPanel, wxID_ANY, "user");
    userGrid->Add(userLabel, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    userGrid->Add(m_usernameInput, 1, wxEXPAND);

    // Full Name
    auto nameLabel = new wxStaticText(userPanel, wxID_ANY, "Full Name:");
    m_fullNameInput = new wxTextCtrl(userPanel, wxID_ANY, "Default User");
    userGrid->Add(nameLabel, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    userGrid->Add(m_fullNameInput, 1, wxEXPAND);

    // Password
    auto passLabel = new wxStaticText(userPanel, wxID_ANY, "Password:");
    m_userPasswordInput = new wxTextCtrl(userPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);
    userGrid->Add(passLabel, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    userGrid->Add(m_userPasswordInput, 1, wxEXPAND);

    // Confirm Password
    auto confirmPassLabel = new wxStaticText(userPanel, wxID_ANY, "Confirm Password:");
    m_confirmUserPasswordInput = new wxTextCtrl(userPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);
    userGrid->Add(confirmPassLabel, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    userGrid->Add(m_confirmUserPasswordInput, 1, wxEXPAND);

    // Administrator Checkbox (spans 2 columns)
    m_adminCheckbox = new wxCheckBox(userPanel, wxID_ANY, " Make this user an administrator (sudo access)");
    m_adminCheckbox->SetValue(true);
    userGrid->AddSpacer(0); // Placeholder for the label column
    userGrid->Add(m_adminCheckbox, 0, wxALIGN_CENTER_VERTICAL | wxTOP, 10);

    userPanelSizer->Add(userGrid, 1, wxEXPAND | wxALL, 15);
    userPanel->SetSizer(userPanelSizer);
    userSizer->Add(userPanel, 1, wxEXPAND);
    mainSizer->Add(userSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 20);

    // --- Root Account Section ---
    wxStaticBox *rootBox = new wxStaticBox(this, wxID_ANY, "Root Account");
    wxStaticBoxSizer *rootSizer = new wxStaticBoxSizer(rootBox, wxVERTICAL);
    wxPanel *rootPanel = new wxPanel(rootBox);
    wxBoxSizer *rootPanelSizer = new wxBoxSizer(wxVERTICAL);

    m_lockRootCheckbox = new wxCheckBox(rootPanel, ID_LOCK_ROOT_CHECKBOX, " Lock the root account (recommended)");
    m_lockRootCheckbox->SetValue(true); // Default to locked
    rootPanelSizer->Add(m_lockRootCheckbox, 0, wxEXPAND | wxALL, 15);

    // Sizer for root password fields (initially hidden)
    m_rootPasswordSizer = new wxFlexGridSizer(2, 2, 15, 25);
    m_rootPasswordSizer->AddGrowableCol(1, 1);

    m_rootPasswordLabel = new wxStaticText(rootPanel, wxID_ANY, "Root Password:");
    m_rootPasswordInput = new wxTextCtrl(rootPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);
    m_rootPasswordSizer->Add(m_rootPasswordLabel, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    m_rootPasswordSizer->Add(m_rootPasswordInput, 1, wxEXPAND);

    m_confirmRootPasswordLabel = new wxStaticText(rootPanel, wxID_ANY, "Confirm Root Password:");
    m_confirmRootPasswordInput = new wxTextCtrl(rootPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);
    m_rootPasswordSizer->Add(m_confirmRootPasswordLabel, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    m_rootPasswordSizer->Add(m_confirmRootPasswordInput, 1, wxEXPAND);

    rootPanelSizer->Add(m_rootPasswordSizer, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 15);
    m_rootPasswordSizer->Show(false); // Hide initially

    rootPanel->SetSizer(rootPanelSizer);
    rootSizer->Add(rootPanel, 1, wxEXPAND);
    mainSizer->Add(rootSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 20);

    mainSizer->AddStretchSpacer(1); // Pushes content up

    SetSizer(mainSizer);
}

void SystemTab::ApplyThemeColors()
{
    // Apply to StaticBoxes
    for (wxWindow *child : GetChildren())
    {
        if (wxStaticBox *box = wxDynamicCast(child, wxStaticBox))
        {
            box->SetForegroundColour(m_textColor);
            // Note: Background color of StaticBox itself is tricky,
            // so we style the panel inside it.
            wxWindow *panelInside = box->GetChildren().GetFirst() ? box->GetChildren().GetFirst()->GetData() : nullptr;
            if (wxPanel *p = wxDynamicCast(panelInside, wxPanel))
            {
                p->SetBackgroundColour(m_panelBgColor);
            }
        }
    }

    // Apply colors to direct children panels if needed (though StaticBox panels handle it)
    for (wxWindow *child : GetChildren())
    {
        wxPanel *panel = wxDynamicCast(child, wxPanel);
        if (panel && !wxDynamicCast(child->GetParent(), wxStaticBox))
        { // Avoid styling panels already inside static boxes
            panel->SetBackgroundColour(m_panelBgColor);
        }
    }

    // Helper lambda to style text controls and choices
    auto styleInput = [&](wxWindow *ctrl)
    {
        if (!ctrl)
            return;
        ctrl->SetBackgroundColour(m_inputBgColor);
        ctrl->SetForegroundColour(m_inputFgColor);
    };

    // Helper lambda to style labels
    auto styleLabel = [&](wxWindow *ctrl)
    {
        if (!ctrl)
            return;
        ctrl->SetForegroundColour(m_labelColor);
    };

    // Helper lambda to style checkboxes
    auto styleCheckbox = [&](wxCheckBox *cb)
    {
        if (!cb)
            return;
        cb->SetForegroundColour(m_textColor); // Checkbox text color
        // Background might be inherited or tricky to set reliably across platforms
    };

    // Apply styles to controls by traversing the sizers
    wxSizer *mainSizer = GetSizer();
    if (!mainSizer)
        return;

    wxSizerItemList &items = mainSizer->GetChildren();
    for (wxSizerItem *item : items)
    {
        wxSizer *sizer = item->GetSizer();
        if (wxStaticBoxSizer *boxSizer = wxDynamicCast(sizer, wxStaticBoxSizer))
        {
            wxStaticBox *box = boxSizer->GetStaticBox();
            box->SetForegroundColour(m_textColor); // Style the box label

            wxWindow *panelInside = box->GetChildren().GetFirst() ? box->GetChildren().GetFirst()->GetData() : nullptr;
            if (wxPanel *p = wxDynamicCast(panelInside, wxPanel))
            {
                p->SetBackgroundColour(m_panelBgColor); // Style inner panel background

                // Iterate through controls *inside* the panel within the static box
                wxSizerItemList &panelItems = p->GetSizer()->GetChildren();
                for (wxSizerItem *panelItem : panelItems)
                {
                    wxSizer *gridSizer = panelItem->GetSizer(); // Assuming FlexGridSizer is direct child
                    if (gridSizer)
                    {
                        wxSizerItemList &gridItems = gridSizer->GetChildren();
                        for (wxSizerItem *gridItem : gridItems)
                        {
                            wxWindow *widget = gridItem->GetWindow();
                            if (wxStaticText *label = wxDynamicCast(widget, wxStaticText))
                            {
                                styleLabel(label);
                            }
                            else if (wxTextCtrl *input = wxDynamicCast(widget, wxTextCtrl))
                            {
                                styleInput(input);
                            }
                            else if (wxChoice *choice = wxDynamicCast(widget, wxChoice))
                            {
                                styleInput(choice);
                            }
                            else if (wxCheckBox *cb = wxDynamicCast(widget, wxCheckBox))
                            {
                                styleCheckbox(cb);
                            }
                        }
                    }
                    // Handle the root password sizer separately if needed
                    else if (panelItem->GetSizer() == m_rootPasswordSizer)
                    {
                        wxSizerItemList &rootItems = m_rootPasswordSizer->GetChildren();
                        for (wxSizerItem *rootItem : rootItems)
                        {
                            wxWindow *widget = rootItem->GetWindow();
                            if (wxStaticText *label = wxDynamicCast(widget, wxStaticText))
                            {
                                styleLabel(label);
                            }
                            else if (wxTextCtrl *input = wxDynamicCast(widget, wxTextCtrl))
                            {
                                styleInput(input);
                            }
                        }
                    }
                    // Handle the initial checkbox in root section
                    else if (wxCheckBox *cb = wxDynamicCast(panelItem->GetWindow(), wxCheckBox))
                    {
                        styleCheckbox(cb);
                    }
                }
            }
        }
    }
    Layout(); // Re-layout after style changes
    Refresh();
}

void SystemTab::OnLockRootCheck(wxCommandEvent &event)
{
    if (m_rootPasswordSizer)
    {
        m_rootPasswordSizer->Show(!event.IsChecked()); // Show if checkbox is *not* checked
        // We might need to explicitly show/hide the labels as well
        if (m_rootPasswordLabel)
            m_rootPasswordLabel->Show(!event.IsChecked());
        if (m_rootPasswordInput)
            m_rootPasswordInput->Show(!event.IsChecked());
        if (m_confirmRootPasswordLabel)
            m_confirmRootPasswordLabel->Show(!event.IsChecked());
        if (m_confirmRootPasswordInput)
            m_confirmRootPasswordInput->Show(!event.IsChecked());

        this->Layout(); // Tell the panel to recalculate layout
    }
    event.Skip();
}