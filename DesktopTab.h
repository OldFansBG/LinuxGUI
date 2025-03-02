#ifndef DESKTOPTAB_H
#define DESKTOPTAB_H

#include <wx/wx.h>
#include "CustomEvents.h" // Include the shared header

// Forward declarations
class SecondWindow; // Forward declare instead of including

class DesktopTab : public wxPanel {
public:
    DesktopTab(wxWindow* parent);

    // Make RecalculateLayout public
    void RecalculateLayout(int windowWidth);

    // Add a method to update the UI with text from a file
    void UpdateTextFromFile();

    // Add this method
    void UpdateGUILabel(const wxString& guiName);

private:
    void CreateDesktopTab();
    wxBitmap LoadImage(const wxString& imageName);
    void OnSize(wxSizeEvent& event);
    void OnFileCopyComplete(wxCommandEvent& event); // Event handler
    
    // New methods for grayscale functionality
    wxBitmap ConvertToGrayscale(const wxBitmap& original);
    void ApplyGrayscaleToCards(const wxString& detectedEnv);

    wxFlexGridSizer* m_gridSizer;
    wxScrolledWindow* m_scrolledWindow;

    // Add a wxStaticText to display the text
    wxStaticText* m_textDisplay;
    
    // Add the reload button
    wxButton* m_reloadButton;
};

#endif // DESKTOPTAB_H
