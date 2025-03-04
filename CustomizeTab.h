#ifndef CUSTOMIZETAB_H
#define CUSTOMIZETAB_H

#include <wx/wx.h>

class CustomizeTab : public wxPanel {
public:
    CustomizeTab(wxWindow* parent);
    void CreateContent();  // Method to create the tab content
    void LoadWallpaper();  // Method to load and display wallpapers
    wxString GetProjectDir();  // Method to get the project directory
    wxString GetWallpaperDirFromCommand(const wxString& containerId, const wxString& detectedDE);  // Helper to get wallpaper dir

private:
    wxScrolledWindow* wallpaperPanel;  // Scrollable panel for displaying wallpapers
    DECLARE_EVENT_TABLE()
};

#endif // CUSTOMIZETAB_H