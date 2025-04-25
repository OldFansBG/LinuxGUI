// MongoDBPanel.h
#ifndef MONGODBPANEL_H
#define MONGODBPANEL_H

#include <wx/wx.h>
#include <wx/panel.h>
#include <wx/listctrl.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <map>
#include <wx/filename.h> // <-- Add include for wxFileName
#include <wx/file.h>     // <-- Add include for wxFile
#include <wx/dir.h>      // <-- Add include for wxDir

// MongoDB specific includes
// These are kept commented out to prevent build errors unless really needed
// #include <mongocxx/uri.hpp>
// #include <mongocxx/client.hpp>
// #include <mongocxx/collection.hpp>
// #include <mongocxx/exception/exception.hpp>
// #include <bsoncxx/builder/stream/document.hpp>
// #include <bsoncxx/builder/basic/document.hpp>
// #include <bsoncxx/builder/basic/kvp.hpp>
// #include <bsoncxx/json.hpp>
// #include <bsoncxx/types.hpp>
// #include <bsoncxx/exception/exception.hpp>
// #include <bsoncxx/string/view_or_value.hpp>

// Forward Declarations
class wxListEvent; // Needed for OnItemSelected

// Define IDs used *within* MongoDBPanel here
enum
{
    ID_MONGODB_PANEL_INTERNAL_CLOSE_BTN = wxID_HIGHEST + 200,
    ID_MONGODB_UPLOAD_JSON,
    ID_MONGODB_REFRESH,
    ID_MONGODB_DOWNLOAD_SELECTED,
    ID_MONGODB_LOAD_INSTALL // <-- Add an ID for Load & Install button
};

class MongoDBPanel : public wxPanel
{
public:
    // ***** MODIFIED CONSTRUCTOR *****
    MongoDBPanel(wxWindow *parent, const wxString &projectDir); // Added projectDir
    virtual ~MongoDBPanel();
    void LoadMongoDBContent(); // Public method to load/refresh data

private:
    // --- Event Handlers ---
    void OnCloseClick(wxCommandEvent &event);
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
    std::map<long, wxString> m_documentIds;
    int m_selectedItemIndex;
    wxString m_selectedDocumentId;

    // --- Styling Helpers (optional) ---
    void ApplyThemeColors();
    wxColour m_bgColor;
    wxColour m_listBgColor;
    wxColour m_listFgColor;
    wxColour m_headerColor;
    wxColour m_buttonBgColor;
    wxColour m_buttonFgColor;
    wxColour m_uploadBtnBgColor;
    wxColour m_headerBgColor;
    wxColour m_borderColor;

    // --- Logic Helpers ---
    void ProcessInstallation(const wxString &configName, const wxString &appNames);
    wxString GetContainerIdFromProjectDir(); // <-- New helper function

    // --- Added JSON File Handling Helpers ---
    bool ParseJsonFile(const wxString &filePath, wxString &configName, wxString &appsList);
    void CreateSampleDBFiles(const wxString &dbDirPath);
    void LoadFromLocalFiles();                                                        // <-- Add fallback loading function
    void SaveToLocalFile(const wxString &sourcePath, const std::string &jsonContent); // <-- Add local saving function

    // ***** ADDED MEMBER *****
    wxString m_projectDir; // Store the project directory path

    // --- Event Table ---
    wxDECLARE_EVENT_TABLE();
};

#endif // MONGODBPANEL_H