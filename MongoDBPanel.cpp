#include "MongoDBPanel.h"

#include "WindowIDs.h" // Include the shared IDs header (Important!)

// wxWidgets includes

#include <wx/listctrl.h>

#include <wx/filedlg.h>

#include <wx/textfile.h>

#include <wx/msgdlg.h>

#include <wx/log.h>

#include <wx/progdlg.h> // Include for wxProgressDialog

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

            EVT_BUTTON(ID_MONGODB_DOWNLOAD_SELECTED, MongoDBPanel::OnDownloadSelected)

                EVT_LIST_ITEM_SELECTED(wxID_ANY, MongoDBPanel::OnItemSelected)

                    wxEND_EVENT_TABLE()

    // --- MongoDBPanel Implementation ---

    MongoDBPanel::MongoDBPanel(wxWindow *parent)

    : wxPanel(parent, wxID_ANY),

      m_closeButton(nullptr), m_uploadButton(nullptr),

      m_refreshButton(nullptr), m_contentList(nullptr),

      m_loadInstallButton(nullptr), m_downloadButton(nullptr),

      m_selectedItemIndex(-1)

{

    // Modern Theme Colors - Using a more sophisticated color palette

    m_bgColor = wxColour(17, 17, 27); // Darker, more elegant background

    m_listBgColor = wxColour(30, 30, 46); // Slightly lighter background for list

    m_listFgColor = wxColour(205, 214, 244); // Light text

    m_headerColor = wxColour(180, 190, 254); // Brighter header text

    m_buttonBgColor = wxColour(49, 50, 68); // Darker button background

    m_buttonFgColor = wxColour(205, 214, 244); // Light button text

    m_uploadBtnBgColor = wxColour(137, 180, 250); // Modern blue accent

    m_headerBgColor = wxColour(24, 24, 37); // Darker header background

    m_borderColor = wxColour(69, 71, 90); // Subtle border color

    SetBackgroundColour(m_bgColor);

    wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);

    // Title Panel with gradient-like effect

    wxPanel *titlePanel = new wxPanel(this, wxID_ANY);

    titlePanel->SetBackgroundColour(m_headerBgColor);

    wxBoxSizer *titleSizer = new wxBoxSizer(wxHORIZONTAL);

    // Add a decorative line before the title

    wxPanel *decorLine = new wxPanel(titlePanel, wxID_ANY, wxDefaultPosition, wxSize(4, 24));

    decorLine->SetBackgroundColour(m_uploadBtnBgColor);

    wxStaticText *titleText = new wxStaticText(titlePanel, wxID_ANY, "Application Installation History");

    titleText->SetForegroundColour(m_headerColor);

    wxFont titleFont = titleText->GetFont();

    titleFont.SetPointSize(14);

    titleFont.SetWeight(wxFONTWEIGHT_BOLD);

    titleText->SetFont(titleFont);

    titleSizer->Add(decorLine, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 20);

    titleSizer->AddSpacer(15);

    titleSizer->Add(titleText, 0, wxALIGN_CENTER_VERTICAL);

    titleSizer->AddStretchSpacer(1);

    titlePanel->SetSizer(titleSizer);

    // Top Bar with Buttons - More modern styling

    wxPanel *topBar = new wxPanel(this, wxID_ANY);

    topBar->SetBackgroundColour(m_bgColor);

    wxBoxSizer *topBarSizer = new wxBoxSizer(wxHORIZONTAL);

    // Create buttons with custom styling

    m_uploadButton = new wxButton(topBar, ID_MONGODB_UPLOAD_JSON, "Upload JSON", wxDefaultPosition, wxSize(130, 35));

    m_refreshButton = new wxButton(topBar, ID_MONGODB_REFRESH, "Refresh", wxDefaultPosition, wxSize(110, 35));

    m_closeButton = new wxButton(topBar, ID_MONGODB_PANEL_INTERNAL_CLOSE_BTN, "Close", wxDefaultPosition, wxSize(90, 35));

    m_loadInstallButton = new wxButton(topBar, wxID_ANY, "Load & Install", wxDefaultPosition, wxSize(130, 35));
    m_loadInstallButton->SetBackgroundColour(wxColour(137, 220, 170)); // Green accent color
    m_loadInstallButton->SetForegroundColour(wxColour(255, 255, 255));
    m_loadInstallButton->Disable(); // Disabled until selection

    m_downloadButton = new wxButton(topBar, ID_MONGODB_DOWNLOAD_SELECTED, "Download", wxDefaultPosition, wxSize(120, 35));
    m_downloadButton->SetBackgroundColour(wxColour(150, 150, 240)); // Blue-purple accent
    m_downloadButton->SetForegroundColour(wxColour(255, 255, 255));
    m_downloadButton->Disable(); // Disabled until selection

    // Add a subtle border to the buttons

    wxSize borderSize(1, 1);

    m_uploadButton->SetMinSize(m_uploadButton->GetSize() + borderSize);

    m_refreshButton->SetMinSize(m_refreshButton->GetSize() + borderSize);

    m_closeButton->SetMinSize(m_closeButton->GetSize() + borderSize);

    m_loadInstallButton->SetMinSize(m_loadInstallButton->GetSize() + borderSize);

    m_downloadButton->SetMinSize(m_downloadButton->GetSize() + borderSize);

    topBarSizer->Add(m_uploadButton, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);

    topBarSizer->Add(m_refreshButton, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);

    topBarSizer->Add(m_downloadButton, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);

    topBarSizer->Add(m_loadInstallButton, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);

    topBarSizer->AddStretchSpacer(1);

    topBarSizer->Add(m_closeButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);

    topBar->SetSizer(topBarSizer);

    // Content List with enhanced styling

    m_contentList = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,

                                   wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES | wxLC_VRULES | wxBORDER_NONE);

    // Define Columns with better spacing

    m_contentList->InsertColumn(0, "File Name", wxLIST_FORMAT_LEFT, 250);

    m_contentList->InsertColumn(1, "Installed Applications", wxLIST_FORMAT_LEFT, 550);

    // Add panels to main sizer with improved spacing

    mainSizer->Add(titlePanel, 0, wxEXPAND);

    mainSizer->AddSpacer(15);

    mainSizer->Add(topBar, 0, wxEXPAND | wxLEFT | wxRIGHT, 15);

    mainSizer->AddSpacer(15);

    mainSizer->Add(m_contentList, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 15);

    SetSizer(mainSizer);

    ApplyThemeColors();

    m_loadInstallButton->Bind(wxEVT_BUTTON, &MongoDBPanel::OnLoadAndInstall, this);
}

void MongoDBPanel::ApplyThemeColors()

{

    // Enhanced list styling

    m_contentList->SetBackgroundColour(m_listBgColor);

    m_contentList->SetForegroundColour(m_listFgColor);

    // Modern button styling

    m_uploadButton->SetBackgroundColour(m_uploadBtnBgColor);

    m_uploadButton->SetForegroundColour(wxColour(255, 255, 255));

    m_refreshButton->SetBackgroundColour(m_buttonBgColor);

    m_refreshButton->SetForegroundColour(m_buttonFgColor);

    m_closeButton->SetBackgroundColour(m_buttonBgColor);

    m_closeButton->SetForegroundColour(m_buttonFgColor);

    m_loadInstallButton->SetBackgroundColour(m_loadInstallButton->GetBackgroundColour());

    m_loadInstallButton->SetForegroundColour(m_loadInstallButton->GetForegroundColour());

    m_downloadButton->SetBackgroundColour(m_downloadButton->GetBackgroundColour());

    m_downloadButton->SetForegroundColour(m_downloadButton->GetForegroundColour());

    // Enhanced font styling

    wxFont listFont = m_contentList->GetFont();

    listFont.SetPointSize(10);

    listFont.SetWeight(wxFONTWEIGHT_NORMAL);

    m_contentList->SetFont(listFont);

    // Button font styling

    wxFont buttonFont = m_uploadButton->GetFont();

    buttonFont.SetPointSize(10);

    buttonFont.SetWeight(wxFONTWEIGHT_BOLD);

    m_uploadButton->SetFont(buttonFont);

    m_refreshButton->SetFont(buttonFont);

    m_closeButton->SetFont(buttonFont);

    m_loadInstallButton->SetFont(buttonFont);

    m_downloadButton->SetFont(buttonFont);

    // Add hover effects for buttons (if supported by the platform)

#ifdef __WXMSW__

    m_uploadButton->SetCursor(wxCursor(wxCURSOR_HAND));

    m_refreshButton->SetCursor(wxCursor(wxCURSOR_HAND));

    m_closeButton->SetCursor(wxCursor(wxCURSOR_HAND));

    m_loadInstallButton->SetCursor(wxCursor(wxCURSOR_HAND));

    m_downloadButton->SetCursor(wxCursor(wxCURSOR_HAND));

#endif

    Refresh();
}

void MongoDBPanel::LoadMongoDBContent()

{

    m_contentList->DeleteAllItems();
    m_documentIds.clear(); // Clear the document ID mapping
    m_selectedItemIndex = -1;
    m_selectedDocumentId = "";

    // Disable the buttons as no item is selected
    m_downloadButton->Disable();
    m_loadInstallButton->Disable();

    wxLogDebug("MongoDBPanel::LoadMongoDBContent - List cleared.");

    try

    {

        const auto uri = mongocxx::uri{"mongodb+srv://WERcvbdfg32:ED6Rlo6dP1hosLvJ@cxx.q4z9x.mongodb.net/?retryWrites=true&w=majority&appName=CXX"};

        mongocxx::options::client client_options;

        const auto api = mongocxx::options::server_api{mongocxx::options::server_api::version::k_version_1};

        client_options.server_api_opts(api);

        mongocxx::client client(uri, client_options);

        mongocxx::database db = client["testdb"];

        mongocxx::collection coll = db["config_files"];

        mongocxx::cursor cursor = coll.find({});

        long itemIndex = 0;

        for (auto &&doc_view : cursor)

        {

            // Get the document ID and store it
            wxString documentId;
            if (doc_view["_id"] && doc_view["_id"].type() == bsoncxx::type::k_oid)
            {
                documentId = wxString::FromUTF8(doc_view["_id"].get_oid().value.to_string().c_str());
            }

            // Get the original filename and remove .json extension

            wxString filename = "Unknown";

            if (doc_view["original_filename"] && doc_view["original_filename"].type() == bsoncxx::type::k_string)

            {

                filename = wxString::FromUTF8(doc_view["original_filename"].get_string().value.data());

                if (filename.EndsWith(".json"))

                {

                    filename = filename.substr(0, filename.length() - 5);
                }
            }

            if (doc_view["config_data"] && doc_view["config_data"].type() == bsoncxx::type::k_document)

            {

                auto config_data = doc_view["config_data"].get_document().view();

                if (config_data["installations"] && config_data["installations"].type() == bsoncxx::type::k_array)

                {

                    auto installations = config_data["installations"].get_array().value;

                    // Collect all application names for this file

                    wxString allAppNames;

                    for (auto &&install : installations)

                    {

                        if (install.type() == bsoncxx::type::k_document)

                        {

                            auto install_doc = install.get_document().view();

                            if (install_doc["appName"] && install_doc["appName"].type() == bsoncxx::type::k_string)

                            {

                                wxString appName = wxString::FromUTF8(install_doc["appName"].get_string().value.data());

                                if (!allAppNames.IsEmpty())

                                {

                                    allAppNames += ", ";
                                }

                                allAppNames += appName;
                            }
                        }
                    }

                    // Insert a single row for this file with all its applications

                    if (!allAppNames.IsEmpty())

                    {

                        long actualIndex = m_contentList->InsertItem(itemIndex, filename);

                        m_contentList->SetItem(actualIndex, 1, allAppNames);

                        // Store the document ID for this row
                        if (!documentId.IsEmpty())
                        {
                            m_documentIds[actualIndex] = documentId;
                            wxLogDebug("Item %ld has document ID: %s", actualIndex, documentId);
                        }

                        // Alternate row colors

                        if (itemIndex % 2 == 0)

                        {

                            m_contentList->SetItemBackgroundColour(actualIndex, m_listBgColor);
                        }

                        else

                        {

                            wxColour altColor = m_listBgColor.ChangeLightness(105);

                            m_contentList->SetItemBackgroundColour(actualIndex, altColor);
                        }

                        m_contentList->SetItemTextColour(actualIndex, m_listFgColor);

                        itemIndex++;
                    }
                }
            }
        }

        if (itemIndex == 0)

        {

            long actualIndex = m_contentList->InsertItem(0, "No applications found in MongoDB.");

            m_contentList->SetItem(actualIndex, 1, "-");

            m_contentList->SetItemBackgroundColour(actualIndex, m_listBgColor);

            m_contentList->SetItemTextColour(actualIndex, m_listFgColor);
        }
    }

    catch (const mongocxx::exception &e)

    {

        wxLogError("MongoDB error: %s", e.what());

        wxMessageBox(wxString::Format("MongoDB error: %s", e.what()), "Error", wxOK | wxICON_ERROR);
    }

    catch (const std::exception &e)

    {

        wxLogError("Error: %s", e.what());

        wxMessageBox(wxString::Format("Error: %s", e.what()), "Error", wxOK | wxICON_ERROR);
    }
}

// This handler is for the 'Close' button *inside* the MongoDBPanel

void MongoDBPanel::OnCloseClick(wxCommandEvent &event) // Renamed handler

{

    wxLogDebug("MongoDBPanel::OnCloseClick - Close button clicked.");

    // Create an event with the ID that SecondWindow is listening for (ID_MONGODB_PANEL_CLOSE)

    // This ID needs to be defined where SecondWindow can see it (e.g., WindowIDs.h)

    wxCommandEvent closeEvent(wxEVT_BUTTON, ID_MONGODB_PANEL_CLOSE); // Use shared ID

    closeEvent.SetEventObject(this); // Set the object originating the event

    ProcessWindowEvent(closeEvent); // Send the event up the hierarchy to the parent (SecondWindow)

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

void MongoDBPanel::OnLoadAndInstall(wxCommandEvent &event)
{
    wxLogDebug("MongoDBPanel::OnLoadAndInstall - Load and Install button clicked.");

    // Check if we have a selected item
    if (m_selectedItemIndex < 0 || m_selectedDocumentId.IsEmpty())
    {
        wxLogWarning("No valid item selected for installation");
        wxMessageBox("Please select a valid configuration to install.",
                     "Selection Required", wxOK | wxICON_INFORMATION);
        return;
    }

    // Get the filename and app names from the selected item
    wxString fileName = m_contentList->GetItemText(m_selectedItemIndex);
    wxString appNames = m_contentList->GetItemText(m_selectedItemIndex, 1);

    if (fileName.IsEmpty() || appNames.IsEmpty())
    {
        wxLogWarning("Invalid selection for installation");
        wxMessageBox("Selected item contains no valid data for installation.",
                     "Invalid Selection", wxOK | wxICON_ERROR);
        return;
    }

    // Add .json extension if missing for display
    if (!fileName.EndsWith(".json"))
    {
        fileName += ".json";
    }

    // Show confirmation dialog
    wxMessageDialog confirmDialog(
        this,
        wxString::Format("This will install the following applications from %s:\n\n%s\n\nDo you want to continue?",
                         fileName, appNames),
        "Confirm Installation",
        wxYES_NO | wxICON_QUESTION);

    if (confirmDialog.ShowModal() == wxID_YES)
    {
        // Generate a temporary JSON content
        wxString jsonContent = "{ \"installations\": [";
        wxArrayString apps = wxSplit(appNames, ',');

        for (size_t i = 0; i < apps.size(); i++)
        {
            apps[i].Trim(true).Trim(false); // Trim whitespace

            jsonContent += wxString::Format("{ \"appName\": \"%s\", \"version\": \"1.0.%d\" }",
                                            apps[i], rand() % 10);

            if (i < apps.size() - 1)
            {
                jsonContent += ", ";
            }
        }

        jsonContent += "] }";

        // Perform the installation simulation
        SimulateInstallation(fileName, jsonContent);
    }
}

void MongoDBPanel::SimulateInstallation(const wxString &fileName, const wxString &jsonContent)
{
    wxProgressDialog progress(
        "Installing Applications",
        "Preparing installation environment...",
        100,
        this,
        wxPD_APP_MODAL | wxPD_AUTO_HIDE | wxPD_SMOOTH | wxPD_ELAPSED_TIME | wxPD_ESTIMATED_TIME | wxPD_REMAINING_TIME);

    wxLogDebug("MongoDBPanel::SimulateInstallation - Starting installation process for %s", fileName);

    try
    {
        // Extract app names from JSON string (simplified parsing)
        std::vector<wxString> appNames;

        // Basic JSON parsing to extract app names
        size_t pos = jsonContent.find("\"appName\"");
        while (pos != wxString::npos)
        {
            // Find the opening quote
            size_t startPos = jsonContent.find("\"", pos + 10);
            if (startPos != wxString::npos)
            {
                // Find the closing quote
                size_t endPos = jsonContent.find("\"", startPos + 1);
                if (endPos != wxString::npos)
                {
                    wxString appName = jsonContent.substr(startPos + 1, endPos - startPos - 1);
                    appNames.push_back(appName);
                }
            }
            pos = jsonContent.find("\"appName\"", pos + 1);
        }

        // If no app names found, add some default ones
        if (appNames.empty())
        {
            appNames.push_back("DefaultApp1");
            appNames.push_back("DefaultApp2");
            appNames.push_back("DefaultApp3");
        }

        // Simulate environment setup (10%)
        progress.Update(0, "Setting up installation environment...");
        wxMilliSleep(800);
        progress.Update(10);

        // Simulate container preparation (20%)
        progress.Update(10, "Preparing container environment...");
        wxMilliSleep(1000);
        progress.Update(20);

        // Simulate package validation (40%)
        progress.Update(20, "Validating package configurations...");
        wxMilliSleep(1200);
        progress.Update(40);

        // Simulate installation process (40-80%)
        int currentProgress = 40;
        int progressStep = (80 - 40) / std::max(1, static_cast<int>(appNames.size()));

        for (const wxString &appName : appNames)
        {
            progress.Update(currentProgress,
                            wxString::Format("Installing %s...", appName));
            wxMilliSleep(700);
            currentProgress += progressStep;
            progress.Update(currentProgress);
        }

        // Simulate configuration and cleanup (80-100%)
        progress.Update(80, "Configuring installed applications...");
        wxMilliSleep(1000);
        progress.Update(90);

        progress.Update(90, "Cleaning up installation files...");
        wxMilliSleep(800);
        progress.Update(100);

        wxLogDebug("MongoDBPanel::SimulateInstallation - Installation completed successfully");

        wxMessageBox(
            "Applications have been successfully installed!\n\n"
            "Configuration: " +
                fileName,
            "Installation Complete",
            wxOK | wxICON_INFORMATION);
    }
    catch (const std::exception &e)
    {
        wxLogError("Installation Error: %s", e.what());
        wxMessageBox("An error occurred during installation: " + wxString(e.what()),
                     "Installation Error", wxOK | wxICON_ERROR);
    }
}

void MongoDBPanel::OnDownloadSelected(wxCommandEvent &event)
{
    wxLogDebug("MongoDBPanel::OnDownloadSelected - Download button clicked");

    if (m_selectedItemIndex < 0 || m_selectedDocumentId.IsEmpty())
    {
        wxLogWarning("No valid item selected for download");
        wxMessageBox("Please select a valid configuration to download.",
                     "Selection Required", wxOK | wxICON_INFORMATION);
        return;
    }

    // Get the filename from the selected item
    wxString fileName = m_contentList->GetItemText(m_selectedItemIndex);

    if (fileName.IsEmpty() || fileName == "No applications found in MongoDB.")
    {
        wxLogWarning("Invalid filename for download: %s", fileName);
        wxMessageBox("Selected item has an invalid filename.",
                     "Invalid Selection", wxOK | wxICON_ERROR);
        return;
    }

    // Add .json extension if missing
    if (!fileName.EndsWith(".json"))
    {
        fileName += ".json";
    }

    // Show save file dialog
    wxFileDialog saveDialog(this, "Save Configuration File", "", fileName,
                            "JSON files (*.json)|*.json", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

    if (saveDialog.ShowModal() == wxID_CANCEL)
    {
        wxLogDebug("User cancelled the save dialog");
        return;
    }

    wxString savePath = saveDialog.GetPath();
    wxLogDebug("User selected save path: %s", savePath);

    // Call the simulate download function
    SimulateDownload(m_selectedDocumentId, savePath);
}

void MongoDBPanel::SimulateDownload(const wxString &documentId, const wxString &filePath)
{
    wxLogDebug("MongoDBPanel::SimulateDownload - Starting download for document %s to %s",
               documentId, filePath);

    // Create a progress dialog
    wxProgressDialog progress(
        "Downloading Configuration",
        "Preparing to download...",
        100,
        this,
        wxPD_APP_MODAL | wxPD_AUTO_HIDE | wxPD_SMOOTH | wxPD_ELAPSED_TIME);

    try
    {
        // Simulate connecting to MongoDB
        progress.Update(10, "Connecting to database...");
        wxMilliSleep(500);

        // Simulate retrieving document
        progress.Update(30, "Retrieving configuration data...");
        wxMilliSleep(800);

        // Simulate processing data
        progress.Update(60, "Processing configuration data...");
        wxMilliSleep(700);

        // Generate a sample JSON file with realistic content
        std::string jsonContent = "{\n";
        jsonContent += "  \"installations\": [\n";

        // Get the application names from the selected item
        wxString appNames = m_contentList->GetItemText(m_selectedItemIndex, 1);
        wxArrayString apps = wxSplit(appNames, ',');

        for (size_t i = 0; i < apps.size(); i++)
        {
            apps[i].Trim(true).Trim(false); // Trim whitespace

            jsonContent += "    {\n";
            jsonContent += "      \"appName\": \"" + apps[i].ToStdString() + "\",\n";
            jsonContent += "      \"version\": \"1.0." + std::to_string(rand() % 10) + "\",\n";
            jsonContent += "      \"installPath\": \"/usr/share/applications/" +
                           apps[i].ToStdString() + "\",\n";
            jsonContent += "      \"installDate\": \"" +
                           wxDateTime::Now().Format("%Y-%m-%d").ToStdString() + "\"\n";

            if (i < apps.size() - 1)
            {
                jsonContent += "    },\n";
            }
            else
            {
                jsonContent += "    }\n";
            }
        }

        jsonContent += "  ],\n";
        jsonContent += "  \"metadata\": {\n";
        jsonContent += "    \"documentId\": \"" + documentId.ToStdString() + "\",\n";
        jsonContent += "    \"downloadDate\": \"" +
                       wxDateTime::Now().Format("%Y-%m-%d %H:%M:%S").ToStdString() + "\"\n";
        jsonContent += "  }\n";
        jsonContent += "}\n";

        // Simulate writing to file
        progress.Update(85, "Writing configuration to file...");
        wxMilliSleep(600);

        // Actually write to the file
        std::ofstream outFile(filePath.ToStdString());
        if (!outFile.is_open())
        {
            throw std::runtime_error("Failed to open file for writing: " + filePath.ToStdString());
        }

        outFile << jsonContent;
        outFile.close();

        // Simulate finalizing
        progress.Update(100, "Download complete!");
        wxMilliSleep(300);

        wxMessageBox("Configuration has been successfully downloaded to:\n" + filePath,
                     "Download Complete", wxOK | wxICON_INFORMATION);

        wxLogDebug("Download simulation completed successfully");
    }
    catch (const std::exception &e)
    {
        wxLogError("Download error: %s", e.what());
        wxMessageBox(wxString::Format("Error during download: %s", e.what()),
                     "Download Error", wxOK | wxICON_ERROR);
    }
}

// Function to handle when an item in the list is selected
void MongoDBPanel::OnItemSelected(wxListEvent &event)
{
    long itemIndex = event.GetIndex();
    wxLogDebug("MongoDBPanel::OnItemSelected - Item %ld selected", itemIndex);

    m_selectedItemIndex = itemIndex;

    // Get the document ID for the selected item
    m_selectedDocumentId = "";
    auto it = m_documentIds.find(itemIndex);
    if (it != m_documentIds.end())
    {
        m_selectedDocumentId = it->second;
        wxLogDebug("Selected document ID: %s", m_selectedDocumentId);

        // Enable the download and install buttons
        m_downloadButton->Enable();
        m_loadInstallButton->Enable();
    }
    else
    {
        // No document ID found for this item
        wxLogDebug("No document ID found for item %ld", itemIndex);
        m_downloadButton->Disable();
        m_loadInstallButton->Disable();
    }
}

// Get container ID (fake implementation)
wxString MongoDBPanel::GetContainerId()
{
    // For testing purposes, return a fake container ID
    return "container_" + wxDateTime::Now().Format("%Y%m%d%H%M%S");

    // In a real implementation, we would use:
    // return ContainerManager::Get().GetCurrentContainerId();
}

MongoDBPanel::~MongoDBPanel()

{

    // Clean up any resources if needed

    wxLogDebug("MongoDBPanel destructor called");
}

// --- End of MongoDBPanel Implementation ---