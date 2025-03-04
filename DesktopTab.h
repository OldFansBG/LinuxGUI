#ifndef DESKTOPTAB_H
#define DESKTOPTAB_H

#include <wx/wx.h>
#include "CustomEvents.h" // Include the shared header

// Forward declarations
class SecondWindow;

class DesktopTab : public wxPanel {
public:
    DesktopTab(wxWindow* parent);
    void RecalculateLayout(int windowWidth);
    void UpdateTextFromFile();
    void UpdateGUILabel(const wxString& guiName);
    wxString GetGuiFilePath();

private:
    void CreateDesktopTab();
    wxBitmap LoadImage(const wxString& imageName);
    void OnSize(wxSizeEvent& event);
    void OnFileCopyComplete(wxCommandEvent& event);
    wxBitmap ConvertToGrayscale(const wxBitmap& original);
    void ApplyGrayscaleToCards(const wxString& detectedEnv);

    wxFlexGridSizer* m_gridSizer;
    wxScrolledWindow* m_scrolledWindow;
    wxStaticText* m_textDisplay;
    wxButton* m_reloadButton;
};

#endif // DESKTOPTAB_H