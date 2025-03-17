#include "SettingsDialog.h"
#include "CustomTitleBar.h"

    BEGIN_EVENT_TABLE(SettingsDialog, wxDialog)
        EVT_BUTTON(wxID_OK, SettingsDialog::OnOK)
            EVT_BUTTON(wxID_CANCEL, SettingsDialog::OnCancel)
                EVT_CHOICE(wxID_ANY, SettingsDialog::OnThemeSelect)
                    END_EVENT_TABLE()

                        SettingsDialog::SettingsDialog(wxWindow *parent)
    : wxDialog(parent, wxID_ANY, "Settings",
               wxDefaultPosition, wxSize(500, 450),
               wxDEFAULT_DIALOG_STYLE)
{
    CreateControls();
    Centre();
}

void SettingsDialog::CreateControls()
{
    m_mainPanel = new wxPanel(this);

    auto *mainSizer = new wxBoxSizer(wxVERTICAL);

    // Theme selection section
    auto *themeSizer = new wxBoxSizer(wxHORIZONTAL);
    auto *themeLabel = new wxStaticText(m_mainPanel, wxID_ANY, "Theme:");
    wxArrayString themes;
    themes.Add("Light");
    themes.Add("Dark");
    themes.Add("High Contrast Light");
    themes.Add("High Contrast Dark");

    m_themeChoice = new wxChoice(m_mainPanel, wxID_ANY,
                                 wxDefaultPosition, wxDefaultSize, themes);
    m_themeChoice->SetSelection(0);

    themeSizer->Add(themeLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    themeSizer->Add(m_themeChoice, 1, wxEXPAND);

    // Container Options section
    auto *containerBox = new wxStaticBox(m_mainPanel, wxID_ANY, "Container Options");
    auto *containerSizer = new wxStaticBoxSizer(containerBox, wxVERTICAL);

    m_dockerCheckbox = new wxCheckBox(containerBox, wxID_ANY, "Use Docker (Recommended)");
    m_dockerCheckbox->SetValue(true); // Docker is default
    containerSizer->Add(m_dockerCheckbox, 0, wxALL, 5);

// Only show WSL option on Windows
#ifdef __WXMSW__
    m_wslCheckbox = new wxCheckBox(containerBox, wxID_ANY, "Use Windows Subsystem for Linux (WSL)");
    m_wslCheckbox->SetValue(false);
    containerSizer->Add(m_wslCheckbox, 0, wxALL, 5);

    // Bind events for container type changes
    m_dockerCheckbox->Bind(wxEVT_CHECKBOX, &SettingsDialog::OnContainerTypeChanged, this);
    m_wslCheckbox->Bind(wxEVT_CHECKBOX, &SettingsDialog::OnContainerTypeChanged, this);
#else
    m_wslCheckbox = nullptr;
#endif

    // Preview panel
    m_previewPanel = new wxPanel(m_mainPanel, wxID_ANY,
                                 wxDefaultPosition, wxSize(-1, 100));

    // Buttons
    auto *buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    auto *okButton = new wxButton(m_mainPanel, wxID_OK, "OK");
    auto *cancelButton = new wxButton(m_mainPanel, wxID_CANCEL, "Cancel");

    buttonSizer->Add(okButton, 0, wxRIGHT, 5);
    buttonSizer->Add(cancelButton);

    // Add all sections to main sizer
    mainSizer->Add(themeSizer, 0, wxEXPAND | wxALL, 10);
    mainSizer->Add(containerSizer, 0, wxEXPAND | wxALL, 10);
    mainSizer->Add(m_previewPanel, 1, wxEXPAND | wxALL, 10);
    mainSizer->AddSpacer(20);
    mainSizer->Add(buttonSizer, 0, wxALIGN_RIGHT | wxALL, 10);

    m_mainPanel->SetSizer(mainSizer);

    auto *dialogSizer = new wxBoxSizer(wxVERTICAL);
    dialogSizer->Add(m_mainPanel, 1, wxEXPAND);
    SetSizer(dialogSizer);
}

void SettingsDialog::OnContainerTypeChanged(wxCommandEvent &event)
{
#ifdef __WXMSW__
    if (event.GetEventObject() == m_dockerCheckbox && m_dockerCheckbox->GetValue())
    {
        m_wslCheckbox->SetValue(false);
    }
    else if (event.GetEventObject() == m_wslCheckbox && m_wslCheckbox->GetValue())
    {
        m_dockerCheckbox->SetValue(false);
    }

    // Ensure at least one option is selected
    if (!m_dockerCheckbox->GetValue() && !m_wslCheckbox->GetValue())
    {
        m_dockerCheckbox->SetValue(true);
    }
#endif
}

void SettingsDialog::OnThemeSelect(wxCommandEvent &event)
{
    // Basic theme preview, just change background
    wxColour colors[] = {
        *wxWHITE,             // Light
        wxColour(50, 50, 50), // Dark
        *wxWHITE,             // High Contrast Light
        *wxBLACK              // High Contrast Dark
    };

    int selection = m_themeChoice->GetSelection();
    m_previewPanel->SetBackgroundColour(colors[selection]);
    m_previewPanel->Refresh();
}

void SettingsDialog::OnOK(wxCommandEvent &event)
{
    wxString selectedTheme = m_themeChoice->GetStringSelection().Lower();

    // Save theme settings
    ThemeConfig::Get().LoadThemes();

    // Find and update the title bar
    wxFrame *mainFrame = wxDynamicCast(GetParent(), wxFrame);
    if (mainFrame)
    {
        wxWindowList &children = mainFrame->GetChildren();
        for (wxWindow *child : children)
        {
            if (CustomTitleBar *titleBar = dynamic_cast<CustomTitleBar *>(child))
            {
                ThemeConfig::Get().ApplyTheme(titleBar, selectedTheme);
                break;
            }
        }
    }

    // Save container preferences
    // In a real application, you would typically save these to a configuration file
    // For now, we just return them via the UseDocker() and UseWSL() methods

    EndModal(wxID_OK);
}

void SettingsDialog::OnCancel(wxCommandEvent &event)
{
    EndModal(wxID_CANCEL);
}