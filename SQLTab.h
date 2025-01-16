#ifndef SQLTAB_H
#define SQLTAB_H

#include <wx/wx.h>
#include <vector>
#include "FlatpakStore.h"  // Updated include

class SQLTab : public wxPanel {
public:
    SQLTab(wxWindow* parent);
    void ShowTab(int tabId);
    void SetContainerId(const wxString& containerId); // Add this line

private:
    void CreateDesktopTab();
    void CreateAppsTab();
    void CreateSystemTab();
    void CreateCustomizeTab();
    void CreateHardwareTab();
    void OnSQLTabChanged(wxCommandEvent& event);
    wxString m_containerId; // Add this line

    wxPanel* m_sqlContent;
    int m_currentSqlTab;
    std::vector<wxButton*> m_sqlTabButtons;
    enum {
        ID_SQL_DESKTOP = wxID_HIGHEST + 1,
        ID_SQL_APPS,
        ID_SQL_SYSTEM,
        ID_SQL_CUSTOMIZE,
        ID_SQL_HARDWARE
    };

    DECLARE_EVENT_TABLE()
};

#endif // SQLTAB_H