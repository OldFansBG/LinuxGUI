#ifndef DESKTOPTAB_H
#define DESKTOPTAB_H

#include <wx/wx.h>
#include "CustomEvents.h" // Include the shared header

class DesktopTab : public wxPanel {
public:
    DesktopTab(wxWindow* parent);

    // Make RecalculateLayout public
    void RecalculateLayout(int windowWidth);

    // Add a method to update the UI with text from a file
    void UpdateTextFromFile();

private:
    void CreateDesktopTab();
    wxBitmap LoadImage(const wxString& imageName);
    void OnSize(wxSizeEvent& event);
    void OnFileCopyComplete(wxCommandEvent& event); // Event handler

    wxFlexGridSizer* m_gridSizer;
    wxScrolledWindow* m_scrolledWindow;

    // Add a wxStaticText to display the text
    wxStaticText* m_textDisplay;
};

#endif // DESKTOPTAB_H