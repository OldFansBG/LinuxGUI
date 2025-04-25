#ifndef MONGODBPANEL_H
#define MONGODBPANEL_H

#include <wx/wx.h>
#include <wx/panel.h>
#include <wx/listctrl.h> // Needed for wxListCtrl
#include <wx/button.h>   // Needed for wxButton
#include <wx/sizer.h>    // Needed for wxBoxSizer
#include <map>           // For storing document IDs

// Forward Declarations (if any complex types were used only as pointers/references)
// Not strictly needed here for basic wx types

// Define IDs used *within* MongoDBPanel here
enum
{
    ID_MONGODB_PANEL_INTERNAL_CLOSE_BTN = wxID_HIGHEST + 200, // Renamed to avoid clash if needed
    ID_MONGODB_UPLOAD_JSON,
    ID_MONGODB_REFRESH,
    ID_MONGODB_DOWNLOAD_SELECTED
};

class MongoDBPanel : public wxPanel
{
public:
    MongoDBPanel(wxWindow *parent);
    virtual ~MongoDBPanel();
    void LoadMongoDBContent(); // Public method to load/refresh data

private:
    // --- Event Handlers ---
    void OnCloseClick(wxCommandEvent &event); // Renamed handler for clarity
    void OnUploadJson(wxCommandEvent &event);
    void OnRefresh(wxCommandEvent &event);
    void OnLoadAndInstall(wxCommandEvent &event);
    void OnItemSelected(wxListEvent &event);
    void OnDownloadSelected(wxCommandEvent &event);

    // --- UI Controls ---
    wxButton *m_closeButton;
    wxButton *m_uploadButton;
    wxButton *m_refreshButton;
    wxButton *m_loadInstallButton;
    wxButton *m_downloadButton;
    wxListCtrl *m_contentList;

    // --- Document tracking ---
    std::map<long, wxString> m_documentIds; // Maps list item indices to MongoDB document IDs
    int m_selectedItemIndex;                // Currently selected item index
    wxString m_selectedDocumentId;          // Currently selected MongoDB document ID

    // --- Styling Helpers (optional) ---
    void ApplyThemeColors();
    wxColour m_bgColor;
    wxColour m_listBgColor;
    wxColour m_listFgColor;
    wxColour m_headerColor; // Note: Header styling is platform-dependent
    wxColour m_buttonBgColor;
    wxColour m_buttonFgColor;
    wxColour m_uploadBtnBgColor;
    wxColour m_headerBgColor;
    wxColour m_borderColor; // Added for subtle border color

    // --- Fake Installation ---
    void SimulateInstallation(const wxString &fileName, const wxString &jsonContent);
    void SimulateDownload(const wxString &documentId, const wxString &fileName);

    // --- Container ID handling ---
    wxString GetContainerId();

    // --- Event Table ---
    wxDECLARE_EVENT_TABLE();
};

#endif // MONGODBPANEL_H