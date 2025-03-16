#ifndef FILESYSTEM_SELECTION_DIALOG_H
#define FILESYSTEM_SELECTION_DIALOG_H

#include <wx/wx.h>
#include <wx/listbox.h>  // Use wxListBox for better styling
#include "ThemeConfig.h" // Required for applying themes

class FilesystemSelectionDialog : public wxDialog
{
public:
    FilesystemSelectionDialog(wxWindow *parent, const wxString &title, const wxArrayString &choices);
    wxString GetSelectedFilesystem() const;

private:
    void OnOK(wxCommandEvent &event);
    void OnCancel(wxCommandEvent &event);
    void ApplyTheme();

    wxListBox *m_listBox;
    wxString m_selectedFilesystem;

    wxDECLARE_EVENT_TABLE();
};

#endif // FILESYSTEM_SELECTION_DIALOG_H