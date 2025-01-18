#ifndef DESKTOPTAB_H
#define DESKTOPTAB_H

#include <wx/wx.h>

class DesktopTab : public wxPanel {
public:
    DesktopTab(wxWindow* parent);

    // Make RecalculateLayout public
    void RecalculateLayout(int windowWidth);

private:
    void CreateDesktopTab();
    wxBitmap LoadImage(const wxString& imageName);
    void OnSize(wxSizeEvent& event);

    wxFlexGridSizer* m_gridSizer;
    wxScrolledWindow* m_scrolledWindow;
};

#endif // DESKTOPTAB_H