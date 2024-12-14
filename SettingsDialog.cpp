#include "SettingsDialog.h"
#include "CustomTitleBar.h"
BEGIN_EVENT_TABLE(SettingsDialog, wxDialog)
    EVT_BUTTON(wxID_OK, SettingsDialog::OnOK)
    EVT_BUTTON(wxID_CANCEL, SettingsDialog::OnCancel)
    EVT_CHOICE(wxID_ANY, SettingsDialog::OnThemeSelect)
    EVT_CHECKBOX(wxID_ANY, SettingsDialog::OnAnimationToggle)
END_EVENT_TABLE()

SettingsDialog::SettingsDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Settings", 
              wxDefaultPosition, wxSize(500, 400),
              wxDEFAULT_DIALOG_STYLE)
{
    CreateControls();
    Centre();
}

void SettingsDialog::CreateControls() {
    m_mainPanel = new wxPanel(this);
    
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    auto* themeSizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Theme selection
    auto* themeLabel = new wxStaticText(m_mainPanel, wxID_ANY, "Theme:");
    wxArrayString themes;
    themes.Add("Light");
    themes.Add("Dark");
    themes.Add("High Contrast Light");
    themes.Add("High Contrast Dark");
    
    m_themeChoice = new wxChoice(m_mainPanel, wxID_ANY, 
                                wxDefaultPosition, wxDefaultSize, themes);
    
    m_themeChoice->SetSelection(0);  // Default to first theme
    
    themeSizer->Add(themeLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    themeSizer->Add(m_themeChoice, 1, wxEXPAND);

    // Animations
    auto* animSizer = new wxBoxSizer(wxHORIZONTAL);
    m_animationsCheckbox = new wxCheckBox(m_mainPanel, wxID_ANY, 
                                        "Enable theme transition animations");
    m_animationsCheckbox->SetValue(true);
    
    animSizer->Add(m_animationsCheckbox, 0, wxALIGN_CENTER_VERTICAL);

    // Transition duration
    auto* transSizer = new wxBoxSizer(wxHORIZONTAL);
    auto* transLabel = new wxStaticText(m_mainPanel, wxID_ANY, "Transition Duration:");
    m_transitionSlider = new wxSlider(m_mainPanel, wxID_ANY, 
                                     200, 100, 500, wxDefaultPosition, wxDefaultSize,
                                     wxSL_HORIZONTAL | wxSL_LABELS);
    
    transSizer->Add(transLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    transSizer->Add(m_transitionSlider, 1, wxEXPAND);

    // Preview panel
    m_previewPanel = new wxPanel(m_mainPanel, wxID_ANY, 
                                wxDefaultPosition, wxSize(-1, 100));
    
    // Buttons
    auto* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    auto* okButton = new wxButton(m_mainPanel, wxID_OK, "OK");
    auto* cancelButton = new wxButton(m_mainPanel, wxID_CANCEL, "Cancel");
    
    buttonSizer->Add(okButton, 0, wxRIGHT, 5);
    buttonSizer->Add(cancelButton);

    mainSizer->Add(themeSizer, 0, wxEXPAND | wxALL, 10);
    mainSizer->Add(animSizer, 0, wxEXPAND | wxALL, 10);
    mainSizer->Add(transSizer, 0, wxEXPAND | wxALL, 10);
    mainSizer->Add(m_previewPanel, 1, wxEXPAND | wxALL, 10);
    mainSizer->AddSpacer(20);
    mainSizer->Add(buttonSizer, 0, wxALIGN_RIGHT | wxALL, 10);

    m_mainPanel->SetSizer(mainSizer);

    auto* dialogSizer = new wxBoxSizer(wxVERTICAL);
    dialogSizer->Add(m_mainPanel, 1, wxEXPAND);
    SetSizer(dialogSizer);
}

void SettingsDialog::OnThemeSelect(wxCommandEvent& event) {
    // Basic theme preview, just change background
    wxColour colors[] = {
        *wxWHITE,  // Light
        wxColour(50, 50, 50),  // Dark
        *wxWHITE,  // High Contrast Light
        *wxBLACK   // High Contrast Dark
    };
    
    int selection = m_themeChoice->GetSelection();
    m_previewPanel->SetBackgroundColour(colors[selection]);
    m_previewPanel->Refresh();
}

void SettingsDialog::OnAnimationToggle(wxCommandEvent& event) {
    m_transitionSlider->Enable(m_animationsCheckbox->IsChecked());
}

void SettingsDialog::OnOK(wxCommandEvent& event) {
    wxString selectedTheme = m_themeChoice->GetStringSelection().Lower();
    wxLogMessage("Selected theme: %s", selectedTheme);  // Debug print
    
    ThemeConfig::Get().LoadThemes();
    
    // Find the title bar by traversing the window hierarchy
    wxFrame* mainFrame = wxDynamicCast(GetParent(), wxFrame);
    if (mainFrame) {
        wxWindowList& children = mainFrame->GetChildren();
        for (wxWindow* child : children) {
            if (CustomTitleBar* titleBar = dynamic_cast<CustomTitleBar*>(child)) {
                ThemeConfig::Get().ApplyTheme(titleBar, selectedTheme);
                wxLogMessage("Found and updated titlebar");  // Debug print
                break;
            }
        }
    }
    
    EndModal(wxID_OK);
}

void SettingsDialog::OnCancel(wxCommandEvent& event) {
    EndModal(wxID_CANCEL);
}