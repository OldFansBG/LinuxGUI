#pragma once

#include <wx/wx.h>
#include <wx/thread.h>
#include <atomic>

class SecondWindow;
class WallpaperLoadThread; // Forward declaration

class CustomizeTab : public wxPanel
{
public:
    CustomizeTab(wxWindow *parent);
    ~CustomizeTab();
    void LoadWallpaper(); // Moved to public section

    // These methods need to be public or friend-accessible to WallpaperLoadThread
    wxString GetProjectDir();
    wxString GetWallpaperDirFromCommand(const wxString &containerId, const wxString &detectedDE);
    wxString ExecuteCommandOnMainThread(const wxString &command, wxArrayString *output = nullptr);

private:
    void CreateContent();

    // Fix function signature - should be wxCommandEvent not wxMouseEvent
    void OnAddWallpaper(wxCommandEvent &event);
    void OnWallpaperReady(wxCommandEvent &event);
    void OnWallpaperLoadComplete(wxCommandEvent &event);

    // New methods for responsive layout
    void OnWallpaperPanelSize(wxSizeEvent &event);
    void UpdateWallpaperGrid();

    wxScrolledWindow *wallpaperPanel;
    wxStaticText *m_loadingText;
    bool m_stopWallpaperLoad;
    bool m_isDestroyed;

    DECLARE_EVENT_TABLE()

    // Alternative approach: make WallpaperLoadThread a friend
    // friend class WallpaperLoadThread;
};

// Update the WallpaperImageData struct to include selection state
struct WallpaperImageData
{
    wxImage image;
    wxString originalPath;
    bool isSelected = false;
};

class WallpaperLoadThread : public wxThread
{
public:
    WallpaperLoadThread(CustomizeTab *tab, bool &stopFlag)
        : wxThread(wxTHREAD_DETACHED), m_tab(tab), m_stopFlag(stopFlag) {}

    virtual ExitCode Entry() override;

private:
    CustomizeTab *m_tab;
    bool &m_stopFlag;
};