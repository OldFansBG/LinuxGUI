#ifndef SQLTAB_H
#define SQLTAB_H

#include <wx/wx.h>
#include <vector>
#include "FlatpakStore.h"
#include "DesktopTab.h"
#include "CustomizeTab.h"  // Add the new CustomizeTab header

class SQLTab : public wxPanel {
public:
    SQLTab(wxWindow* parent, const wxString& workDir); // Add workDir parameter
    void ShowTab(int tabId);
    void SetContainerId(const wxString& containerId);

private:
    void CreateDesktopTab();
    void CreateAppsTab();
    void CreateSystemTab();
    void CreateCustomizeTab();  // Keep declaration
    void OnConfigTabChanged(wxCommandEvent& event);

    wxString m_containerId; // Container ID for FlatpakStore
    wxString m_workDir;     // Working directory for FlatpakStore

    wxPanel* m_configContent;  // Content panel for tabs
    int m_currentConfigTab;    // Currently active tab
    std::vector<wxButton*> m_configTabButtons; // Buttons for tab switching

    // Tab IDs
    enum {
        ID_CONFIG_DESKTOP = wxID_HIGHEST + 1, // Start from wxID_HIGHEST + 1 to avoid conflicts
        ID_CONFIG_APPS,
        ID_CONFIG_SYSTEM,
        ID_CONFIG_CUSTOMIZE
    };

    DECLARE_EVENT_TABLE()
};

#endif // SQLTAB_H
