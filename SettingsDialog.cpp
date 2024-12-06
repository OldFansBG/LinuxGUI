// SettingsDialog.cpp
#include <wx/wx.h>
#include "SettingsDialog.h"
#include "MainFrame.h"
#include "ThemeColors.h"

BEGIN_EVENT_TABLE(SettingsDialog, wxDialog)
    EVT_BUTTON(wxID_OK, SettingsDialog::OnOK)
    EVT_BUTTON(wxID_CANCEL, SettingsDialog::OnCancel)
END_EVENT_TABLE()

SettingsDialog::SettingsDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Settings", 
              wxDefaultPosition, wxSize(400, 200),
              wxDEFAULT_DIALOG_STYLE)
{
    // Get the current theme from parent window
    bool isDarkMode = parent->GetBackgroundColour().GetLuminance() < 0.5;
    m_isDark = isDarkMode;
    
    // Create main panel
    m_mainPanel = new wxPanel(this);
    
    // Apply initial theme immediately
    this->SetBackgroundColour(isDarkMode ? ThemeColors::DARK_BACKGROUND : ThemeColors::LIGHT_BACKGROUND);
    m_mainPanel->SetBackgroundColour(isDarkMode ? ThemeColors::DARK_BACKGROUND : ThemeColors::LIGHT_BACKGROUND);
    
    // Create controls
    wxStaticText* themeLabel = new wxStaticText(m_mainPanel, wxID_ANY, "Theme:");
    themeLabel->SetForegroundColour(isDarkMode ? ThemeColors::DARK_TEXT : ThemeColors::LIGHT_TEXT);
    
    wxArrayString themes;
    themes.Add("Light");
    themes.Add("Dark");
    
    m_themeChoice = new wxChoice(m_mainPanel, wxID_ANY, 
                                wxDefaultPosition, wxDefaultSize, themes);
    m_themeChoice->SetSelection(isDarkMode ? 1 : 0);
    m_themeChoice->SetBackgroundColour(isDarkMode ? ThemeColors::DARK_BUTTON_BG : ThemeColors::LIGHT_BUTTON_BG);
    m_themeChoice->SetForegroundColour(isDarkMode ? ThemeColors::DARK_TEXT : ThemeColors::LIGHT_TEXT);
    
    // Create sizers
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* themeSizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);

    wxButton* okButton = new wxButton(m_mainPanel, wxID_OK, "OK");
    wxButton* cancelButton = new wxButton(m_mainPanel, wxID_CANCEL, "Cancel");
    
    if (isDarkMode) {
        okButton->SetBackgroundColour(ThemeColors::DARK_BUTTON_BG);
        okButton->SetForegroundColour(ThemeColors::DARK_BUTTON_TEXT);
        cancelButton->SetBackgroundColour(ThemeColors::DARK_BUTTON_BG);
        cancelButton->SetForegroundColour(ThemeColors::DARK_BUTTON_TEXT);
    } else {
        okButton->SetBackgroundColour(ThemeColors::LIGHT_BUTTON_BG);
        okButton->SetForegroundColour(ThemeColors::LIGHT_BUTTON_TEXT);
        cancelButton->SetBackgroundColour(ThemeColors::LIGHT_BUTTON_BG);
        cancelButton->SetForegroundColour(ThemeColors::LIGHT_BUTTON_TEXT);
    }
    
    themeSizer->Add(themeLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    themeSizer->Add(m_themeChoice, 1, wxEXPAND);

    buttonSizer->Add(okButton, 0, wxRIGHT, 5);
    buttonSizer->Add(cancelButton);

    mainSizer->Add(themeSizer, 0, wxEXPAND | wxALL, 10);
    mainSizer->AddStretchSpacer();
    mainSizer->Add(buttonSizer, 0, wxALIGN_RIGHT | wxALL, 10);

    m_mainPanel->SetSizer(mainSizer);

    wxBoxSizer* dialogSizer = new wxBoxSizer(wxVERTICAL);
    dialogSizer->Add(m_mainPanel, 1, wxEXPAND);
    SetSizer(dialogSizer);

    Centre();
}

void SettingsDialog::OnOK(wxCommandEvent& event)
{
    bool isDark = (m_themeChoice->GetSelection() == 1);
    
    // Apply theme to Main Window first
    if (MainFrame* mainWindow = dynamic_cast<MainFrame*>(GetParent())) {
        mainWindow->ApplyTheme(isDark);
    }
    
    // Important: Store the selection
    m_isDark = isDark;
    
    // Close only the dialog with OK result
    EndModal(wxID_OK);
}

void SettingsDialog::ApplyTheme(bool isDark)
{
    // Apply to dialog itself
    this->SetBackgroundColour(isDark ? ThemeColors::DARK_BACKGROUND : ThemeColors::LIGHT_BACKGROUND);
    
    // Apply to main panel
    m_mainPanel->SetBackgroundColour(isDark ? ThemeColors::DARK_BACKGROUND : ThemeColors::LIGHT_BACKGROUND);
    
    // Apply to all controls recursively
    ApplyThemeToWindow(m_mainPanel, isDark);
    
    // Refresh everything
    m_mainPanel->Refresh();
    this->Refresh();
    Update();
}

void SettingsDialog::ApplyThemeToWindow(wxWindow* window, bool isDark)
{
    // Apply theme to current window
    window->SetBackgroundColour(isDark ? ThemeColors::DARK_BACKGROUND : ThemeColors::LIGHT_BACKGROUND);
    
    // Apply specific control styling
    if (auto* text = wxDynamicCast(window, wxStaticText)) {
        text->SetForegroundColour(isDark ? ThemeColors::DARK_TEXT : ThemeColors::LIGHT_TEXT);
    }
    else if (auto* button = wxDynamicCast(window, wxButton)) {
        button->SetBackgroundColour(isDark ? ThemeColors::DARK_BUTTON_BG : ThemeColors::LIGHT_BUTTON_BG);
        button->SetForegroundColour(isDark ? ThemeColors::DARK_BUTTON_TEXT : ThemeColors::LIGHT_BUTTON_TEXT);
    }
    else if (auto* choice = wxDynamicCast(window, wxChoice)) {
        choice->SetBackgroundColour(isDark ? ThemeColors::DARK_BUTTON_BG : ThemeColors::LIGHT_BUTTON_BG);
        choice->SetForegroundColour(isDark ? ThemeColors::DARK_TEXT : ThemeColors::LIGHT_TEXT);
    }
    
    // Recursively apply to all children
    for (wxWindow* child : window->GetChildren()) {
        ApplyThemeToWindow(child, isDark);
    }
}

void SettingsDialog::OnCancel(wxCommandEvent& event)
{
    EndModal(wxID_CANCEL);
}