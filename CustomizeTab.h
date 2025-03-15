// CustomizeTab.h
#ifndef CUSTOMIZETAB_H
#define CUSTOMIZETAB_H

#include <wx/wx.h>
#include <atomic>

class CustomizeTab : public wxPanel
{
public:
    CustomizeTab(wxWindow *parent);
    ~CustomizeTab();
    void CreateContent();
    void LoadWallpaper();
    wxString GetProjectDir();
    wxString GetWallpaperDirFromCommand(const wxString &containerId, const wxString &detectedDE);
    wxString ExecuteCommandOnMainThread(const wxString& command, wxArrayString* output = nullptr);

private:
    wxScrolledWindow *wallpaperPanel;
    wxStaticText *m_loadingText;
    std::atomic<bool> m_stopWallpaperLoad;
    bool m_isDestroyed; // Flag to track destruction

    void OnWallpaperReady(wxCommandEvent &event);
    void OnWallpaperLoadComplete(wxCommandEvent &event);

    DECLARE_EVENT_TABLE()
};

wxDECLARE_EVENT(wxEVT_WALLPAPER_READY, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_WALLPAPER_LOAD_COMPLETE, wxCommandEvent);

struct WallpaperImageData
{
    wxImage image;
};

class WallpaperLoadThread : public wxThread
{
public:
    WallpaperLoadThread(CustomizeTab *tab, std::atomic<bool> &stopFlag)
        : wxThread(wxTHREAD_DETACHED), m_tab(tab), m_stopFlag(stopFlag) {}

protected:
    virtual ExitCode Entry() override;

private:
    CustomizeTab *m_tab;
    std::atomic<bool> &m_stopFlag;
};

#endif // CUSTOMIZETAB_H