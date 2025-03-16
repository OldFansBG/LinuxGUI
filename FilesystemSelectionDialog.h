#ifndef FILESYSTEM_SELECTION_DIALOG_H
#define FILESYSTEM_SELECTION_DIALOG_H

#include <wx/wx.h>
#include <wx/panel.h>
#include <wx/stattext.h>
#include <wx/checkbox.h> // Use wxCheckBox to simulate radio behavior
#include <wx/scrolwin.h> // Add this include
#include "ThemeConfig.h"

class FilesystemSelectionDialog : public wxDialog
{
public:
    FilesystemSelectionDialog(wxWindow *parent, const wxString &title, const wxArrayString &choices);
    wxString GetSelectedFilesystem() const;

private:
    void OnOK(wxCommandEvent &event);
    void OnCancel(wxCommandEvent &event);
    void ApplyTheme();
    void CreateItemList(const wxArrayString &choices);
    void OnItemCheckbox(wxCommandEvent &event); // Handler for checkbox clicks
    void OnPaint(wxPaintEvent &event);          // Add this line

    wxScrolledWindow *m_itemsPanel; // Change Panel to ScrolledWindow
    wxString m_selectedFilesystem;
    std::vector<wxCheckBox *> m_checkboxes; // Store checkboxes for single selection logic

    wxColour m_normalItemColor; // Base color for items
    wxColour m_hoverItemColor;  // Hover color for items

    void SetupColors();                                                     // Setup theme-aware colors
    wxPanel *CreateItemPanel(wxWindow *parent, const wxString &choiceText); // Helper to create item panels

    wxDECLARE_EVENT_TABLE();
};

#endif // FILESYSTEM_SELECTION_DIALOG_H