#include "FilesystemSelectionDialog.h"
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/stattext.h>

wxBEGIN_EVENT_TABLE(FilesystemSelectionDialog, wxDialog)
    EVT_BUTTON(wxID_OK, FilesystemSelectionDialog::OnOK)
        EVT_BUTTON(wxID_CANCEL, FilesystemSelectionDialog::OnCancel)
            wxEND_EVENT_TABLE()

                FilesystemSelectionDialog::FilesystemSelectionDialog(wxWindow *parent, const wxString &title, const wxArrayString &choices)
    : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);

    // ListBox to display filesystem choices
    m_listBox = new wxListBox(this, wxID_ANY, wxDefaultPosition, wxSize(400, 200), choices);
    mainSizer->Add(m_listBox, 1, wxEXPAND | wxALL, 10);

    // OK and Cancel buttons
    wxBoxSizer *buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton *okButton = new wxButton(this, wxID_OK, "OK");
    wxButton *cancelButton = new wxButton(this, wxID_CANCEL, "Cancel");
    buttonSizer->Add(okButton, 0, wxRIGHT, 5);
    buttonSizer->Add(cancelButton, 0);
    mainSizer->Add(buttonSizer, 0, wxALIGN_RIGHT | wxALL, 10);

    SetSizerAndFit(mainSizer);
    ApplyTheme(); // Apply theme on creation
    CentreOnParent();
}

wxString FilesystemSelectionDialog::GetSelectedFilesystem() const
{
    return m_selectedFilesystem;
}

void FilesystemSelectionDialog::OnOK(wxCommandEvent &event)
{
    if (m_listBox->GetSelection() != wxNOT_FOUND)
    {
        m_selectedFilesystem = m_listBox->GetStringSelection();

        // Remove description if it's still present. This is a good safety check.
        int descPos = m_selectedFilesystem.Find(" (");
        if (descPos != wxNOT_FOUND)
            m_selectedFilesystem = m_selectedFilesystem.Left(descPos);

        EndModal(wxID_OK);
    }
    else
    {
        wxMessageBox("Please select a filesystem.", "Selection Required", wxOK | wxICON_WARNING, this);
    }
}

void FilesystemSelectionDialog::OnCancel(wxCommandEvent &event)
{
    EndModal(wxID_CANCEL);
}

void FilesystemSelectionDialog::ApplyTheme()
{
    ThemeConfig &themeConfig = ThemeConfig::Get();
    const ThemeColors &colors = themeConfig.GetThemeColors(themeConfig.GetCurrentTheme());

    SetBackgroundColour(colors.panel.background);

    // Apply colors to child controls
    wxWindowList &children = GetChildren();
    for (wxWindow *child : children)
    {
        if (wxButton *button = wxDynamicCast(child, wxButton))
        {
            button->SetBackgroundColour(colors.button.background);
            button->SetForegroundColour(colors.button.text);
        }
        else if (wxListBox *listBox = wxDynamicCast(child, wxListBox))
        {
            listBox->SetBackgroundColour(colors.input.background);
            listBox->SetForegroundColour(colors.input.text);
        }
        else if (wxStaticText *staticText = wxDynamicCast(child, wxStaticText))
        {
            staticText->SetForegroundColour(colors.text);
        }
    }

    Refresh(); // Important: Refresh to apply changes immediately
}