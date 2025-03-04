#include "CustomizeTab.h"

wxBEGIN_EVENT_TABLE(CustomizeTab, wxPanel)
// No events yet, but keeping the table for future use
wxEND_EVENT_TABLE()

CustomizeTab::CustomizeTab(wxWindow* parent) 
    : wxPanel(parent)
{
    CreateContent();
}

void CustomizeTab::CreateContent() {
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    wxStaticBox* themeBox = new wxStaticBox(this, wxID_ANY, "Theme");
    themeBox->SetForegroundColour(*wxWHITE);
    wxStaticBoxSizer* themeSizer = new wxStaticBoxSizer(themeBox, wxVERTICAL);

    wxFlexGridSizer* themeGrid = new wxFlexGridSizer(2, 2, 10, 10);
    themeGrid->AddGrowableCol(1, 1);

    auto themeLabel = new wxStaticText(this, wxID_ANY, "Global Theme");
    themeLabel->SetForegroundColour(*wxWHITE);
    wxChoice* theme = new wxChoice(this, wxID_ANY);
    theme->Append({"Default", "Nord", "Dracula", "Gruvbox", "Tokyo Night", "Catppuccin"});
    theme->SetSelection(0);

    themeGrid->Add(themeLabel, 0, wxALIGN_CENTER_VERTICAL);
    themeGrid->Add(theme, 1, wxEXPAND);

    auto accentLabel = new wxStaticText(this, wxID_ANY, "Accent Color");
    accentLabel->SetForegroundColour(*wxWHITE);
    wxPanel* colorPanel = new wxPanel(this);
    colorPanel->SetBackgroundColour(wxColour(37, 99, 235));

    themeGrid->Add(accentLabel, 0, wxALIGN_CENTER_VERTICAL);
    themeGrid->Add(colorPanel, 1, wxEXPAND);

    themeSizer->Add(themeGrid, 1, wxEXPAND | wxALL, 5);
    sizer->Add(themeSizer, 0, wxEXPAND | wxALL, 10);

    this->SetSizer(sizer);
    this->Layout();
}