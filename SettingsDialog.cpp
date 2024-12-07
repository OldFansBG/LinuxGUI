// SettingsDialog.cpp
#include "SettingsDialog.h"

BEGIN_EVENT_TABLE(SettingsDialog, wxDialog)
    EVT_BUTTON(wxID_OK, SettingsDialog::OnOK)
    EVT_BUTTON(wxID_CANCEL, SettingsDialog::OnCancel)
END_EVENT_TABLE()

SettingsDialog::SettingsDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Settings", 
              wxDefaultPosition, wxSize(400, 200),
              wxDEFAULT_DIALOG_STYLE)
{
    ThemeManager::Get().AddObserver(this);
    CreateControls();
    Centre();
}

void SettingsDialog::CreateControls()
{
    m_mainPanel = new wxPanel(this);
    
    wxStaticText* themeLabel = new wxStaticText(m_mainPanel, wxID_ANY, "Theme:");
    
    wxArrayString themes;
    themes.Add("Light");
    themes.Add("Dark");
    
    m_themeChoice = new wxChoice(m_mainPanel, wxID_ANY, 
                                wxDefaultPosition, wxDefaultSize, themes);
    m_themeChoice->SetSelection(ThemeManager::Get().IsDarkTheme() ? 1 : 0);
    
    wxButton* okButton = new wxButton(m_mainPanel, wxID_OK, "OK");
    wxButton* cancelButton = new wxButton(m_mainPanel, wxID_CANCEL, "Cancel");
    
    wxBoxSizer* themeSizer = new wxBoxSizer(wxHORIZONTAL);
    themeSizer->Add(themeLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    themeSizer->Add(m_themeChoice, 1, wxEXPAND);

    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->Add(okButton, 0, wxRIGHT, 5);
    buttonSizer->Add(cancelButton);

    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->Add(themeSizer, 0, wxEXPAND | wxALL, 10);
    mainSizer->AddStretchSpacer();
    mainSizer->Add(buttonSizer, 0, wxALIGN_RIGHT | wxALL, 10);

    m_mainPanel->SetSizer(mainSizer);

    wxBoxSizer* dialogSizer = new wxBoxSizer(wxVERTICAL);
    dialogSizer->Add(m_mainPanel, 1, wxEXPAND);
    SetSizer(dialogSizer);
}

void SettingsDialog::OnOK(wxCommandEvent& event)
{
    ThemeManager::Get().SetTheme(
        m_themeChoice->GetSelection() == 1 ? 
        ThemeManager::Theme::Dark : ThemeManager::Theme::Light
    );
    EndModal(wxID_OK);
}

void SettingsDialog::OnCancel(wxCommandEvent& event)
{
    EndModal(wxID_CANCEL);
}