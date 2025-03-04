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
    void OnSQLTabChanged(wxCommandEvent& event);

    wxString m_containerId; // Container ID for FlatpakStore
    wxString m_workDir;     // Working directory for FlatpakStore

    wxPanel* m_sqlContent;  // Content panel for tabs
    int m_currentSqlTab;    // Currently active tab
    std::vector<wxButton*> m_sqlTabButtons; // Buttons for tab switching

    // Tab IDs
    enum {
        ID_SQL_DESKTOP = wxID_HIGHEST + 1, // Start from wxID_HIGHEST + 1 to avoid conflicts
        ID_SQL_APPS,
        ID_SQL_SYSTEM,
        ID_SQL_CUSTOMIZE
    };

    DECLARE_EVENT_TABLE()
};

#endif // SQLTAB_H
