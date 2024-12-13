#include "SettingsDialog.h"

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
    ThemeSystem::Get().RegisterControl(this);
    CreateControls();
    Centre();
}

SettingsDialog::~SettingsDialog() {
    ThemeSystem::Get().UnregisterControl(this);
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
    
    auto currentTheme = ThemeSystem::Get().GetCurrentTheme();
    m_themeChoice->SetSelection(static_cast<int>(currentTheme));
    
    themeSizer->Add(themeLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    themeSizer->Add(m_themeChoice, 1, wxEXPAND);

    // Animations
    auto* animSizer = new wxBoxSizer(wxHORIZONTAL);
    m_animationsCheckbox = new wxCheckBox(m_mainPanel, wxID_ANY, 
                                        "Enable theme transition animations");
    m_animationsCheckbox->SetValue(ThemeSystem::Get().AreAnimationsEnabled());
    
    animSizer->Add(m_animationsCheckbox, 0, wxALIGN_CENTER_VERTICAL);

    // Transition duration
    auto* transSizer = new wxBoxSizer(wxHORIZONTAL);
    auto* transLabel = new wxStaticText(m_mainPanel, wxID_ANY, "Transition Duration:");
    m_transitionSlider = new wxSlider(m_mainPanel, wxID_ANY, 
                                     ThemeSystem::Get().GetTransitionDuration(),
                                     100, 500, wxDefaultPosition, wxDefaultSize,
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

    // Initial preview
    PreviewTheme(currentTheme);
}

void SettingsDialog::OnThemeSelect(wxCommandEvent& event) {
    auto theme = static_cast<ThemeSystem::ThemeVariant>(m_themeChoice->GetSelection());
    PreviewTheme(theme);
}

void SettingsDialog::OnAnimationToggle(wxCommandEvent& event) {
    m_transitionSlider->Enable(m_animationsCheckbox->IsChecked());
}

void SettingsDialog::PreviewTheme(ThemeSystem::ThemeVariant theme) {
    ThemeColors previewColors;
    switch (theme) {
        case ThemeSystem::ThemeVariant::Dark:
            previewColors = ThemeSystem::Get().CreateDarkTheme();
            break;
        case ThemeSystem::ThemeVariant::Light:
            previewColors = ThemeSystem::Get().CreateLightTheme();
            break;
        case ThemeSystem::ThemeVariant::HighContrastDark:
            previewColors = ThemeSystem::Get().CreateHighContrastDarkTheme();
            break;
        case ThemeSystem::ThemeVariant::HighContrastLight:
            previewColors = ThemeSystem::Get().CreateHighContrastLightTheme();
            break;
        default:
            return;
    }

    m_previewPanel->SetBackgroundColour(previewColors.get(ColorRole::Background));
    m_previewPanel->Refresh();
}

void SettingsDialog::OnOK(wxCommandEvent& event) {
    auto theme = static_cast<ThemeSystem::ThemeVariant>(m_themeChoice->GetSelection());
    ThemeSystem::Get().EnableAnimations(m_animationsCheckbox->IsChecked());
    ThemeSystem::Get().SetTransitionDuration(m_transitionSlider->GetValue());
    ThemeSystem::Get().ApplyTheme(theme);
    EndModal(wxID_OK);
}

void SettingsDialog::OnCancel(wxCommandEvent& event) {
    EndModal(wxID_CANCEL);
}