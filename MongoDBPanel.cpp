

#include "MongoDBPanel.h"
#include "WindowIDs.h" // Include the shared IDs header (Important!)

// wxWidgets includes
#include <wx/listctrl.h>
#include <wx/filedlg.h>
#include <wx/textfile.h>
#include <wx/msgdlg.h>
#include <wx/log.h>

// Standard library includes
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <string> // Include for std::string

// MongoDB specific includes
#include <mongocxx/uri.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/collection.hpp>
#include <mongocxx/exception/exception.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/types.hpp>
#include <bsoncxx/exception/exception.hpp>
#include <bsoncxx/string/view_or_value.hpp>

// --- Event Table for MongoDBPanel ---
wxBEGIN_EVENT_TABLE(MongoDBPanel, wxPanel)
    EVT_BUTTON(ID_MONGODB_PANEL_INTERNAL_CLOSE_BTN, MongoDBPanel::OnCloseClick) // Use internal ID
    EVT_BUTTON(ID_MONGODB_UPLOAD_JSON, MongoDBPanel::OnUploadJson)
        EVT_BUTTON(ID_MONGODB_REFRESH, MongoDBPanel::OnRefresh)
            wxEND_EVENT_TABLE()

    // --- MongoDBPanel Implementation ---

    MongoDBPanel::MongoDBPanel(wxWindow *parent)
    : wxPanel(parent, wxID_ANY), // Added wxID_ANY
      m_closeButton(nullptr), m_uploadButton(nullptr),
      m_refreshButton(nullptr), m_contentList(nullptr)
{
    // Basic Theme Colors (Replace with ThemeConfig if integrated)
    m_bgColor = wxColour(30, 30, 46);        // Panel background
    m_listBgColor = wxColour(49, 50, 68);    // List background
    m_listFgColor = wxColour(205, 214, 244); // List text
    m_headerColor = wxColour(166, 173, 200); // List header text (styling might vary)
    m_buttonBgColor = wxColour(75, 85, 99);  // Default button bg
    m_buttonFgColor = *wxWHITE;
    m_uploadBtnBgColor = wxColour(79, 70, 229); // Accent for upload

    SetBackgroundColour(m_bgColor);

    wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);

    // --- Top Bar with Buttons ---
    wxPanel *topBar = new wxPanel(this, wxID_ANY); // Added wxID_ANY
    topBar->SetBackgroundColour(m_bgColor);        // Match panel bg
    wxBoxSizer *topBarSizer = new wxBoxSizer(wxHORIZONTAL);

    m_uploadButton = new wxButton(topBar, ID_MONGODB_UPLOAD_JSON, "Upload JSON", wxDefaultPosition, wxSize(120, 30));
    m_refreshButton = new wxButton(topBar, ID_MONGODB_REFRESH, "Refresh", wxDefaultPosition, wxSize(100, 30));
    m_closeButton = new wxButton(topBar, ID_MONGODB_PANEL_INTERNAL_CLOSE_BTN, "Close", wxDefaultPosition, wxSize(80, 30)); // Use internal ID

    topBarSizer->Add(m_uploadButton, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
    topBarSizer->Add(m_refreshButton, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
    topBarSizer->AddStretchSpacer(1); // Push close button to the right
    topBarSizer->Add(m_closeButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    topBar->SetSizer(topBarSizer);

    // --- Content List ---
    m_contentList = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                   wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES | wxLC_VRULES | wxBORDER_NONE); // Report style, no border

    // Define Columns
    m_contentList->InsertColumn(0, "ID", wxLIST_FORMAT_LEFT, 220); // MongoDB _id (wider)
    m_contentList->InsertColumn(1, "Filename", wxLIST_FORMAT_LEFT, 180);
    m_contentList->InsertColumn(2, "Upload Time", wxLIST_FORMAT_LEFT, 160);
    m_contentList->InsertColumn(3, "Content Snippet", wxLIST_FORMAT_LEFT, 350); // Wider snippet

    // --- Assemble Main Sizer ---
    mainSizer->Add(topBar, 0, wxEXPAND | wxTOP | wxBOTTOM, 5);
    mainSizer->Add(m_contentList, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10); // Add list control with padding

    SetSizer(mainSizer);
    ApplyThemeColors(); // Apply colors
}

void MongoDBPanel::ApplyThemeColors()
{
    m_contentList->SetBackgroundColour(m_listBgColor);
    m_contentList->SetForegroundColour(m_listFgColor);
    // Note: Setting header text color reliably might require owner-drawn ListCtrl or platform specifics.

    m_uploadButton->SetBackgroundColour(m_uploadBtnBgColor);
    m_uploadButton->SetForegroundColour(m_buttonFgColor);
    m_refreshButton->SetBackgroundColour(m_buttonBgColor);
    m_refreshButton->SetForegroundColour(m_buttonFgColor);
    m_closeButton->SetBackgroundColour(m_buttonBgColor); // Close button style
    m_closeButton->SetForegroundColour(m_buttonFgColor);

    // Font styling (optional)
    wxFont listFont = m_contentList->GetFont();
    listFont.SetPointSize(10); // Slightly larger list font
    m_contentList->SetFont(listFont);

    wxFont buttonFont = m_uploadButton->GetFont();
    buttonFont.SetWeight(wxFONTWEIGHT_BOLD);
    m_uploadButton->SetFont(buttonFont);
    m_refreshButton->SetFont(buttonFont);
    m_closeButton->SetFont(buttonFont);

    Refresh();
}

void MongoDBPanel::LoadMongoDBContent()
{
    m_contentList->DeleteAllItems(); // Clear previous content
    wxLogDebug("MongoDBPanel::LoadMongoDBContent - List cleared.");

    try
    {
        // Connection URI - IMPORTANT: Replace with your actual connection string
        const auto uri = mongocxx::uri{"mongodb+srv://WERcvbdfg32:ED6Rlo6dP1hosLvJ@cxx.q4z9x.mongodb.net/?retryWrites=true&w=majority&appName=CXX"};
        mongocxx::options::client client_options;
        const auto api = mongocxx::options::server_api{mongocxx::options::server_api::version::k_version_1};
        client_options.server_api_opts(api);

        mongocxx::client client(uri, client_options);
        mongocxx::database db = client["testdb"];       // Your database name
        mongocxx::collection coll = db["config_files"]; // Collection for JSON files
        wxLogDebug("MongoDBPanel::LoadMongoDBContent - Connected to db: %s, collection: %s",
                   std::string(db.name()),    // Convert view_or_value
                   std::string(coll.name())); // Convert view_or_value

        mongocxx::cursor cursor = coll.find({}); // Find all documents
        wxLogDebug("MongoDBPanel::LoadMongoDBContent - Executed find query.");

        long itemIndex = 0;
        for (auto &&doc_view : cursor)
        {
            // --- Extract Data for Columns ---
            wxString docIdStr = "N/A";
            if (doc_view["_id"] && doc_view["_id"].type() == bsoncxx::type::k_oid)
            {
                docIdStr = doc_view["_id"].get_oid().value.to_string();
            }

            wxString filenameStr = "(no filename)"; // Default
            if (doc_view["original_filename"] && doc_view["original_filename"].type() == bsoncxx::type::k_string)
            {                                                                                              // Use k_string
                filenameStr = wxString::FromUTF8(doc_view["original_filename"].get_string().value.data()); // Use get_string()
            }

            wxString timestampStr = "(no timestamp)"; // Default
            if (doc_view["upload_timestamp"] && doc_view["upload_timestamp"].type() == bsoncxx::type::k_date)
            {
                auto time_point_ms = doc_view["upload_timestamp"].get_date();          // Get the b_date wrapper
                std::chrono::system_clock::time_point time_point{time_point_ms.value}; // Construct time_point from ms duration
                std::time_t time_c = std::chrono::system_clock::to_time_t(time_point);
                std::tm tm_buf;
// Use platform-specific safe function for localtime
#ifdef _WIN32
                localtime_s(&tm_buf, &time_c);
#else
                localtime_r(&time_c, &tm_buf);
#endif
                std::stringstream ss;
                ss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S"); // ISO 8601 like
                timestampStr = ss.str();
            }
            else if (doc_view["upload_timestamp"] && doc_view["upload_timestamp"].type() == bsoncxx::type::k_string)
            {                                                                                              // Use k_string
                timestampStr = wxString::FromUTF8(doc_view["upload_timestamp"].get_string().value.data()); // Use get_string()
            }

            wxString contentSnippet = "(No Content/Invalid)";
            try
            {
                // Preferentially get snippet from "config_data" if it exists
                std::string key_to_check = "config_data"; // The key where the original JSON is stored
                if (doc_view[key_to_check])
                {
                    if (doc_view[key_to_check].type() == bsoncxx::type::k_document)
                    {
                        contentSnippet = bsoncxx::to_json(doc_view[key_to_check].get_document().view());
                    }
                    else
                    {
                        // Attempt to convert the entire document if config_data isn't a document
                        contentSnippet = bsoncxx::to_json(doc_view);
                    }
                }
                else
                {
                    // Fallback to whole document if "config_data" is not present
                    contentSnippet = bsoncxx::to_json(doc_view);
                }

                // Truncate the snippet
                if (contentSnippet.Length() > 150)
                { // Increased snippet length
                    contentSnippet = contentSnippet.Left(147) + "...";
                }
            }
            catch (const bsoncxx::exception &e)
            {
                wxLogWarning("Could not convert BSON to JSON for snippet (Doc ID: %s): %s", docIdStr, e.what());
                contentSnippet = "(Error converting)";
            }

            // --- Insert into List Control ---
            long actualIndex = m_contentList->InsertItem(itemIndex, docIdStr);
            m_contentList->SetItem(actualIndex, 1, filenameStr);
            m_contentList->SetItem(actualIndex, 2, timestampStr);
            m_contentList->SetItem(actualIndex, 3, contentSnippet);

            // Alternate row colors (optional but good for readability)
            if (itemIndex % 2 == 0)
            {
                m_contentList->SetItemBackgroundColour(actualIndex, m_listBgColor);
            }
            else
            {
                wxColour altColor = m_listBgColor.ChangeLightness(105); // Slightly lighter
                m_contentList->SetItemBackgroundColour(actualIndex, altColor);
            }
            m_contentList->SetItemTextColour(actualIndex, m_listFgColor); // Ensure text color

            itemIndex++;
        }

        wxLogDebug("MongoDBPanel::LoadMongoDBContent - Processed %ld documents.", itemIndex);

        if (itemIndex == 0)
        {
            // Display a message if the collection is empty
            long actualIndex = m_contentList->InsertItem(0, "No configuration files found in MongoDB.");
            m_contentList->SetItem(actualIndex, 1, "-");
            m_contentList->SetItem(actualIndex, 2, "-");
            m_contentList->SetItem(actualIndex, 3, "-");
            m_contentList->SetItemBackgroundColour(actualIndex, m_listBgColor);
            m_contentList->SetItemTextColour(actualIndex, m_listFgColor);
        }
    }
    catch (const mongocxx::exception &e)
    {
        wxLogError("MongoDB Error during Load: %s", e.what());
        wxMessageBox("Error connecting to or reading from MongoDB:\n" + wxString(e.what()),
                     "MongoDB Error", wxOK | wxICON_ERROR, this);
        long actualIndex = m_contentList->InsertItem(0, "Error loading data from MongoDB.");
        m_contentList->SetItem(actualIndex, 1, "Check Logs");
        m_contentList->SetItem(actualIndex, 2, "-");
        m_contentList->SetItem(actualIndex, 3, wxString(e.what()).Left(100)); // Show part of error
        m_contentList->SetItemBackgroundColour(actualIndex, *wxRED);          // Highlight error row
        m_contentList->SetItemTextColour(actualIndex, *wxWHITE);
    }
    catch (const std::exception &e)
    {
        wxLogError("Standard Exception during Load: %s", e.what());
        wxMessageBox("An unexpected error occurred while loading data: " + wxString(e.what()),
                     "Error", wxOK | wxICON_ERROR, this);
    }
    wxLogDebug("MongoDBPanel::LoadMongoDBContent - Finished loading.");
}

// This handler is for the 'Close' button *inside* the MongoDBPanel
void MongoDBPanel::OnCloseClick(wxCommandEvent &event) // Renamed handler
{
    wxLogDebug("MongoDBPanel::OnCloseClick - Close button clicked.");
    // Create an event with the ID that SecondWindow is listening for (ID_MONGODB_PANEL_CLOSE)
    // This ID needs to be defined where SecondWindow can see it (e.g., WindowIDs.h)
    wxCommandEvent closeEvent(wxEVT_BUTTON, ID_MONGODB_PANEL_CLOSE); // Use shared ID
    closeEvent.SetEventObject(this);                                 // Set the object originating the event
    ProcessWindowEvent(closeEvent);                                  // Send the event up the hierarchy to the parent (SecondWindow)
    wxLogDebug("MongoDBPanel::OnCloseClick - Sent close request event to parent.");
}

void MongoDBPanel::OnUploadJson(wxCommandEvent &event)
{
    wxLogDebug("MongoDBPanel::OnUploadJson - Upload button clicked.");
    wxFileDialog openFileDialog(this, "Select JSON file to upload", "", "",
                                "JSON files (*.json)|*.json", wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if (openFileDialog.ShowModal() == wxID_CANCEL)
    {
        wxLogDebug("MongoDBPanel::OnUploadJson - File selection cancelled.");
        return; // User cancelled
    }

    wxString filePath = openFileDialog.GetPath();
    wxString fileName = openFileDialog.GetFilename();
    wxLogDebug("MongoDBPanel::OnUploadJson - Selected file: %s", filePath);

    // --- Read JSON File Content ---
    std::ifstream fileStream(filePath.ToStdString());
    if (!fileStream.is_open())
    {
        wxLogError("MongoDBPanel::OnUploadJson - Failed to open file: %s", filePath);
        wxMessageBox("Failed to open file: " + filePath, "File Error", wxOK | wxICON_ERROR, this);
        return;
    }
    std::stringstream buffer;
    buffer << fileStream.rdbuf();
    std::string jsonContent = buffer.str();
    fileStream.close();

    if (jsonContent.empty())
    {
        wxLogWarning("MongoDBPanel::OnUploadJson - File is empty: %s", filePath);
        wxMessageBox("File appears to be empty: " + filePath, "File Warning", wxOK | wxICON_WARNING, this);
        // Allow upload of empty files? Decide based on requirements.
        // return; // Uncomment to prevent uploading empty files
    }
    wxLogDebug("MongoDBPanel::OnUploadJson - Read %zu bytes from file.", jsonContent.length());

    // --- Upload to MongoDB ---
    try
    {
        // Parse JSON content into a BSON document
        bsoncxx::document::value doc_value = bsoncxx::from_json(jsonContent);
        wxLogDebug("MongoDBPanel::OnUploadJson - Successfully parsed JSON content.");

        // Connect to MongoDB
        const auto uri = mongocxx::uri{"mongodb+srv://WERcvbdfg32:ED6Rlo6dP1hosLvJ@cxx.q4z9x.mongodb.net/?retryWrites=true&w=majority&appName=CXX"};
        mongocxx::options::client client_options;
        const auto api = mongocxx::options::server_api{mongocxx::options::server_api::version::k_version_1};
        client_options.server_api_opts(api);

        mongocxx::client client(uri, client_options);
        mongocxx::database db = client["testdb"];
        mongocxx::collection coll = db["config_files"];
        wxLogDebug("MongoDBPanel::OnUploadJson - Connected to MongoDB for upload.");

        // --- Add Metadata ---
        auto now = std::chrono::system_clock::now();
        bsoncxx::builder::basic::document builder{};

        // Append the actual parsed JSON data under a specific key
        builder.append(bsoncxx::builder::basic::kvp("config_data", doc_value.view()));

        // Append metadata fields
        builder.append(bsoncxx::builder::basic::kvp("original_filename", fileName.ToStdString()));
        builder.append(bsoncxx::builder::basic::kvp("upload_timestamp", bsoncxx::types::b_date{now}));
        wxLogDebug("MongoDBPanel::OnUploadJson - Built BSON document with metadata.");

        // Insert the document with metadata
        auto result = coll.insert_one(builder.view());
        wxLogDebug("MongoDBPanel::OnUploadJson - Executed insert_one.");

        if (result)
        {
            bsoncxx::oid oid = result->inserted_id().get_oid().value;
            wxLogDebug("MongoDBPanel::OnUploadJson - Insert successful, OID: %s", oid.to_string());
            wxMessageBox("JSON file '" + fileName + "' uploaded successfully!\nObjectID: " + oid.to_string(),
                         "Upload Success", wxOK | wxICON_INFORMATION, this);
            LoadMongoDBContent(); // Refresh the list view after successful upload
        }
        else
        {
            wxLogError("MongoDBPanel::OnUploadJson - MongoDB insert_one did not return a result.");
            wxMessageBox("Failed to upload JSON file to MongoDB (no result returned).", "Upload Error", wxOK | wxICON_ERROR, this);
        }
    }
    catch (const bsoncxx::exception &e)
    {
        wxLogError("BSON/JSON Parsing Error during upload: %s", e.what());
        wxMessageBox("Error parsing JSON file content: " + wxString(e.what()) +
                         "\nPlease ensure the selected file contains valid JSON.",
                     "JSON Parse Error", wxOK | wxICON_ERROR, this);
    }
    catch (const mongocxx::exception &e)
    {
        wxLogError("MongoDB Upload Error: %s", e.what());
        wxMessageBox("Error uploading file to MongoDB: " + wxString(e.what()),
                     "MongoDB Error", wxOK | wxICON_ERROR, this);
    }
    catch (const std::exception &e)
    {
        wxLogError("Standard Exception during upload: %s", e.what());
        wxMessageBox("An unexpected error occurred during upload: " + wxString(e.what()),
                     "Error", wxOK | wxICON_ERROR, this);
    }
    wxLogDebug("MongoDBPanel::OnUploadJson - Upload process finished.");
}

void MongoDBPanel::OnRefresh(wxCommandEvent &event)
{
    wxLogDebug("MongoDBPanel::OnRefresh - Refresh button clicked.");
    LoadMongoDBContent(); // Simply reload the content from the database
    wxLogDebug("MongoDBPanel::OnRefresh - Content reload initiated.");
}

// --- End of MongoDBPanel Implementation ---