#ifndef MONGODBPANEL_H
#define MONGODBPANEL_H

#include <wx/wx.h>
#include <wx/panel.h>
#include <wx/listctrl.h> // Needed for wxListCtrl
#include <wx/button.h>   // Needed for wxButton
#include <wx/sizer.h>    // Needed for wxBoxSizer

// Forward Declarations (if any complex types were used only as pointers/references)
// Not strictly needed here for basic wx types

// Define IDs used *within* MongoDBPanel here
enum
{
    ID_MONGODB_PANEL_INTERNAL_CLOSE_BTN = wxID_HIGHEST + 200, // Renamed to avoid clash if needed
    ID_MONGODB_UPLOAD_JSON,
    ID_MONGODB_REFRESH
};

class MongoDBPanel : public wxPanel
{
public:
    MongoDBPanel(wxWindow *parent);
    void LoadMongoDBContent(); // Public method to load/refresh data

private:
    // --- Event Handlers ---
    void OnCloseClick(wxCommandEvent &event); // Renamed handler for clarity
    void OnUploadJson(wxCommandEvent &event);
    void OnRefresh(wxCommandEvent &event);

    // --- UI Controls ---
    wxButton *m_closeButton;
    wxButton *m_uploadButton;
    wxButton *m_refreshButton;
    wxListCtrl *m_contentList;

    // --- Styling Helpers (optional) ---
    void ApplyThemeColors();
    wxColour m_bgColor;
    wxColour m_listBgColor;
    wxColour m_listFgColor;
    wxColour m_headerColor; // Note: Header styling is platform-dependent
    wxColour m_buttonBgColor;
    wxColour m_buttonFgColor;
    wxColour m_uploadBtnBgColor;

    // --- Event Table ---
    wxDECLARE_EVENT_TABLE();
};

#endif // MONGODBPANEL_H