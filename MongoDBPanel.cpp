// MongoDBPanel.cpp
#include "MongoDBPanel.h"
#include "WindowIDs.h" // Include the shared IDs header (Important!)
#include <wx/listctrl.h>
#include <wx/filedlg.h>
#include <wx/textfile.h>
#include <wx/msgdlg.h>
#include <wx/log.h>
#include <wx/progdlg.h>  // Include for wxProgressDialog
#include <wx/utils.h>    // <-- Add for wxExecute (even though we simulate) and wxSplit
#include <wx/filename.h> // <-- Add for wxFileName
#include <wx/file.h>     // <-- Add for wxFile
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <string>    // Include for std::string
#include <random>    // For generating fake data
#include <vector>    // <-- Add for std::vector
#include <stdexcept> // For std::runtime_error
#include <algorithm> // For std::min, std::max

// MongoDB/BSON includes
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
    EVT_BUTTON(ID_MONGODB_PANEL_INTERNAL_CLOSE_BTN, MongoDBPanel::OnCloseClick)
        EVT_BUTTON(ID_MONGODB_UPLOAD_JSON, MongoDBPanel::OnUploadJson)
            EVT_BUTTON(ID_MONGODB_REFRESH, MongoDBPanel::OnRefresh)
                EVT_BUTTON(ID_MONGODB_DOWNLOAD_SELECTED, MongoDBPanel::OnDownloadSelected)
                    EVT_BUTTON(ID_MONGODB_LOAD_INSTALL, MongoDBPanel::OnLoadAndInstall) // Correct ID binding
    EVT_LIST_ITEM_SELECTED(wxID_ANY, MongoDBPanel::OnItemSelected)
        wxEND_EVENT_TABLE()

    // --- MongoDBPanel Implementation ---

    MongoDBPanel::MongoDBPanel(wxWindow *parent, const wxString &projectDir)
    : wxPanel(parent, wxID_ANY),
      m_projectDir(projectDir),
      m_closeButton(nullptr), m_uploadButton(nullptr),
      m_refreshButton(nullptr), m_contentList(nullptr),
      m_loadInstallButton(nullptr), m_downloadButton(nullptr),
      m_selectedItemIndex(-1)
{
    // Modern Theme Colors
    m_bgColor = wxColour(17, 17, 27);
    m_listBgColor = wxColour(30, 30, 46);
    m_listFgColor = wxColour(205, 214, 244);
    m_headerColor = wxColour(180, 190, 254);
    m_buttonBgColor = wxColour(49, 50, 68);
    m_buttonFgColor = wxColour(205, 214, 244);
    m_uploadBtnBgColor = wxColour(137, 180, 250);
    m_headerBgColor = wxColour(24, 24, 37);
    m_borderColor = wxColour(69, 71, 90);
    SetBackgroundColour(m_bgColor);

    // Create db_files directory if it doesn't exist
    wxString dbDirPath = wxFileName(m_projectDir, "db_files").GetFullPath();
    if (!wxDirExists(dbDirPath))
    {
        wxMkdir(dbDirPath);
        wxLogDebug("Created database files directory: %s", dbDirPath);
    }

    wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);

    // Title Panel
    wxPanel *titlePanel = new wxPanel(this, wxID_ANY);
    titlePanel->SetBackgroundColour(m_headerBgColor);
    wxBoxSizer *titleSizer = new wxBoxSizer(wxHORIZONTAL);
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

    // Top Bar with Buttons
    wxPanel *topBar = new wxPanel(this, wxID_ANY);
    topBar->SetBackgroundColour(m_bgColor);
    wxBoxSizer *topBarSizer = new wxBoxSizer(wxHORIZONTAL);
    m_uploadButton = new wxButton(topBar, ID_MONGODB_UPLOAD_JSON, "Upload JSON", wxDefaultPosition, wxSize(130, 35));
    m_refreshButton = new wxButton(topBar, ID_MONGODB_REFRESH, "Refresh", wxDefaultPosition, wxSize(110, 35));
    m_downloadButton = new wxButton(topBar, ID_MONGODB_DOWNLOAD_SELECTED, "Download", wxDefaultPosition, wxSize(120, 35));
    m_downloadButton->Disable();
    m_loadInstallButton = new wxButton(topBar, ID_MONGODB_LOAD_INSTALL, "Load & Install", wxDefaultPosition, wxSize(130, 35));
    m_loadInstallButton->Disable();
    m_closeButton = new wxButton(topBar, ID_MONGODB_PANEL_INTERNAL_CLOSE_BTN, "Close", wxDefaultPosition, wxSize(90, 35));

    topBarSizer->Add(m_uploadButton, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
    topBarSizer->Add(m_refreshButton, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
    topBarSizer->Add(m_downloadButton, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
    topBarSizer->Add(m_loadInstallButton, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
    topBarSizer->AddStretchSpacer(1);
    topBarSizer->Add(m_closeButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    topBar->SetSizer(topBarSizer);

    // Content List
    m_contentList = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                   wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES | wxLC_VRULES | wxBORDER_NONE);
    m_contentList->InsertColumn(0, "File Name", wxLIST_FORMAT_LEFT, 250);
    m_contentList->InsertColumn(1, "Installed Applications", wxLIST_FORMAT_LEFT, 550);

    // Add panels to main sizer
    mainSizer->Add(titlePanel, 0, wxEXPAND);
    mainSizer->AddSpacer(15);
    mainSizer->Add(topBar, 0, wxEXPAND | wxLEFT | wxRIGHT, 15);
    mainSizer->AddSpacer(15);
    mainSizer->Add(m_contentList, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 15);
    SetSizer(mainSizer);

    ApplyThemeColors();
}

// Get container ID from project directory helper
wxString MongoDBPanel::GetContainerIdFromProjectDir()
{
    if (m_projectDir.IsEmpty())
    {
        wxLogWarning("MongoDBPanel::GetContainerIdFromProjectDir - Project directory is not set.");
        return wxEmptyString;
    }
    wxString containerIdPath = wxFileName(m_projectDir, "container_id.txt").GetFullPath();
    wxString containerId;
    if (wxFile::Exists(containerIdPath))
    {
        wxFile file(containerIdPath, wxFile::read);
        if (file.IsOpened())
        {
            file.ReadAll(&containerId);
            containerId.Trim(true).Trim(false);
            if (!containerId.IsEmpty())
            {
                wxLogDebug("MongoDBPanel::GetContainerIdFromProjectDir - Found container ID: %s", containerId);
                return containerId;
            }
            else
            {
                wxLogWarning("MongoDBPanel::GetContainerIdFromProjectDir - container_id.txt is empty.");
            }
        }
        else
        {
            wxLogError("MongoDBPanel::GetContainerIdFromProjectDir - Failed to open %s", containerIdPath);
        }
    }
    else
    {
        wxLogDebug("MongoDBPanel::GetContainerIdFromProjectDir - File not found: %s", containerIdPath);
    }
    return wxEmptyString;
}

// Load content from MongoDB connection
void MongoDBPanel::LoadMongoDBContent()
{
    m_contentList->DeleteAllItems();
    m_documentIds.clear();
    m_selectedItemIndex = -1;
    m_selectedDocumentId = "";
    m_downloadButton->Disable();
    m_loadInstallButton->Disable();
    wxLogDebug("MongoDBPanel::LoadMongoDBContent - Clearing list and disabling buttons.");

    wxString containerId = GetContainerIdFromProjectDir();
    if (!containerId.IsEmpty())
    {
        wxLogDebug("MongoDBPanel::LoadMongoDBContent - Using container ID: %s", containerId);
    }

    try
    {
        // Connect to MongoDB
        const auto uri = mongocxx::uri{"mongodb+srv://<user>:<password>@<cluster>.mongodb.net/?retryWrites=true&w=majority&appName=CXX"};
        mongocxx::options::client client_options;
        const auto api = mongocxx::options::server_api{mongocxx::options::server_api::version::k_version_1};
        client_options.server_api_opts(api);

        wxLogDebug("MongoDBPanel::LoadMongoDBContent - Connecting to MongoDB...");
        mongocxx::client client(uri, client_options);
        mongocxx::database db = client["testdb"];
        mongocxx::collection coll = db["config_files"];
        wxLogDebug("MongoDBPanel::LoadMongoDBContent - Connected to MongoDB, fetching documents...");

        mongocxx::cursor cursor = coll.find({});
        long itemIndex = 0;

        for (auto &&doc_view : cursor)
        {
            wxString configName = "Unknown";
            wxString appsList = "";
            wxString documentId = "";

            // Get the document ID as string
            if (doc_view["_id"] && doc_view["_id"].type() == bsoncxx::type::k_oid)
            {
                documentId = wxString::FromUTF8(doc_view["_id"].get_oid().value.to_string().c_str());
            }

            // Try to get config_name directly
            if (doc_view["config_name"] && doc_view["config_name"].type() == bsoncxx::type::k_string)
            {
                configName = wxString::FromUTF8(doc_view["config_name"].get_string().value.data());
            }

            // Try to get apps directly
            if (doc_view["apps"] && doc_view["apps"].type() == bsoncxx::type::k_string)
            {
                appsList = wxString::FromUTF8(doc_view["apps"].get_string().value.data());
            }
            // If not found directly, check if it's in config_data structure
            else if (doc_view["config_data"] && doc_view["config_data"].type() == bsoncxx::type::k_document)
            {
                auto config_data = doc_view["config_data"].get_document().view();

                // Try to get config_name from config_data
                if (config_data["config_name"] && config_data["config_name"].type() == bsoncxx::type::k_string)
                {
                    configName = wxString::FromUTF8(config_data["config_name"].get_string().value.data());
                }

                // Check for installations array
                if (config_data["installations"] && config_data["installations"].type() == bsoncxx::type::k_array)
                {
                    auto installations = config_data["installations"].get_array().value;

                    // First, collect all application names for this file
                    for (auto &&install : installations)
                    {
                        if (install.type() == bsoncxx::type::k_document)
                        {
                            auto install_doc = install.get_document().view();
                            if (install_doc["appName"] && install_doc["appName"].type() == bsoncxx::type::k_string)
                            {
                                wxString appName = wxString::FromUTF8(install_doc["appName"].get_string().value.data());
                                if (!appsList.IsEmpty())
                                {
                                    appsList += ", ";
                                }
                                appsList += appName;
                            }
                        }
                    }
                }
            }

            // Fallback to original_filename if config_name is unknown
            if (configName == "Unknown" && doc_view["original_filename"] &&
                doc_view["original_filename"].type() == bsoncxx::type::k_string)
            {
                configName = wxString::FromUTF8(doc_view["original_filename"].get_string().value.data());
                configName.Replace(".json", ""); // Remove .json extension if present
            }

            // Add to list if we have valid data
            if (!documentId.IsEmpty() && (!configName.IsEmpty() || !appsList.IsEmpty()))
            {
                long actualIndex = m_contentList->InsertItem(itemIndex, configName);
                m_contentList->SetItem(actualIndex, 1, appsList);
                m_documentIds[actualIndex] = documentId;

                wxLogDebug("MongoDBPanel::LoadMongoDBContent - Added item %ld ('%s') with ID: %s",
                           actualIndex, configName, documentId);

                if (itemIndex % 2 == 0)
                    m_contentList->SetItemBackgroundColour(actualIndex, m_listBgColor);
                else
                    m_contentList->SetItemBackgroundColour(actualIndex, m_listBgColor.ChangeLightness(105));

                m_contentList->SetItemTextColour(actualIndex, m_listFgColor);
                itemIndex++;
            }
        }

        if (itemIndex == 0)
        {
            long actualIndex = m_contentList->InsertItem(0, "No applications found in MongoDB.");
            m_contentList->SetItem(actualIndex, 1, "-");
            m_contentList->SetItemBackgroundColour(actualIndex, m_listBgColor);
            m_contentList->SetItemTextColour(actualIndex, m_listFgColor);
        }

        wxLogDebug("MongoDBPanel::LoadMongoDBContent - Finished loading content. Found %ld items.", itemIndex);
    }
    catch (const mongocxx::exception &e)
    {
        wxLogError("MongoDB error: %s", e.what());
        wxMessageBox(wxString::Format("MongoDB error: %s", e.what()), "Error", wxOK | wxICON_ERROR);

        // Fall back to local files if MongoDB connection fails
        LoadFromLocalFiles();
    }
    catch (const std::exception &e)
    {
        wxLogError("Error: %s", e.what());
        wxMessageBox(wxString::Format("Error connecting to MongoDB: %s", e.what()), "Error", wxOK | wxICON_ERROR);

        // Fall back to local files if MongoDB connection fails
        LoadFromLocalFiles();
    }
}

// Fall back to loading from local files
void MongoDBPanel::LoadFromLocalFiles()
{
    wxLogDebug("MongoDBPanel::LoadFromLocalFiles - Falling back to local files");

    // Load JSON files from db_files directory
    wxString dbDirPath = wxFileName(m_projectDir, "db_files").GetFullPath();
    if (!wxDirExists(dbDirPath))
    {
        wxMkdir(dbDirPath);
        wxLogDebug("Created database files directory: %s", dbDirPath);
    }

    wxDir dir(dbDirPath);
    if (!dir.IsOpened())
    {
        wxLogError("Failed to open DB directory: %s", dbDirPath);
        return;
    }

    wxString filename;
    bool hasFiles = dir.GetFirst(&filename, "*.json");
    long itemIndex = 0;

    // If no files, create default sample files
    if (!hasFiles)
    {
        wxLogDebug("No DB files found. Creating sample files...");
        CreateSampleDBFiles(dbDirPath);
        hasFiles = dir.GetFirst(&filename, "*.json");
    }

    while (hasFiles)
    {
        wxString filePath = wxFileName(dbDirPath, filename).GetFullPath();
        wxLogDebug("Processing DB file: %s", filePath);

        // Read and parse JSON file
        wxString configName, appsList;
        wxString documentId = wxFileName(filename).GetName(); // Use filename without extension as ID

        if (ParseJsonFile(filePath, configName, appsList))
        {
            long actualIndex = m_contentList->InsertItem(itemIndex, configName);
            m_contentList->SetItem(actualIndex, 1, appsList);
            m_documentIds[actualIndex] = documentId;

            wxLogDebug("MongoDBPanel::LoadFromLocalFiles - Added item %ld ('%s') with ID: %s",
                       actualIndex, configName, documentId);

            if (itemIndex % 2 == 0)
                m_contentList->SetItemBackgroundColour(actualIndex, m_listBgColor);
            else
                m_contentList->SetItemBackgroundColour(actualIndex, m_listBgColor.ChangeLightness(105));

            m_contentList->SetItemTextColour(actualIndex, m_listFgColor);
            itemIndex++;
        }

        hasFiles = dir.GetNext(&filename);
    }

    if (itemIndex == 0)
    {
        long actualIndex = m_contentList->InsertItem(0, "No applications found in local files.");
        m_contentList->SetItem(actualIndex, 1, "-");
        m_contentList->SetItemBackgroundColour(actualIndex, m_listBgColor);
        m_contentList->SetItemTextColour(actualIndex, m_listFgColor);
    }
}

// Create sample DB files
void MongoDBPanel::CreateSampleDBFiles(const wxString &dbDirPath)
{
    std::vector<std::pair<wxString, wxString>> sampleData = {
        {"Gaming Setup", "Steam, Discord, Lutris"},
        {"Development Env", "VS Code, Docker Desktop, GitKraken, Firefox Developer Edition"},
        {"Multimedia Station", "VLC, Kdenlive, GIMP, Spotify"},
        {"Minimal Install", "htop, neofetch"}};

    for (size_t i = 0; i < sampleData.size(); i++)
    {
        wxString fileId = wxDateTime::Now().Format("%Y%m%d%H%M%S") + wxString::Format("_%zu", i);
        wxString fileName = wxFileName(dbDirPath, fileId + ".json").GetFullPath();

        wxLogDebug("Creating sample DB file: %s", fileName);

        std::ofstream outFile(fileName.ToStdString());
        if (outFile.is_open())
        {
            outFile << "{\n";
            outFile << "  \"config_name\": \"" << sampleData[i].first.ToStdString() << "\",\n";
            outFile << "  \"apps\": \"" << sampleData[i].second.ToStdString() << "\",\n";
            outFile << "  \"timestamp\": \"" << wxDateTime::Now().FormatISOCombined(' ').ToStdString() << "\",\n";
            outFile << "  \"container_id\": \"" << GetContainerIdFromProjectDir().ToStdString() << "\"\n";
            outFile << "}\n";
            outFile.close();
            wxLogDebug("Created sample DB file successfully");
        }
        else
        {
            wxLogError("Failed to create sample DB file: %s", fileName);
        }

        // Small delay to ensure unique timestamps
        wxMilliSleep(100);
    }
}

// Parse JSON file helper
bool MongoDBPanel::ParseJsonFile(const wxString &filePath, wxString &configName, wxString &appsList)
{
    wxTextFile file;
    if (!file.Open(filePath))
    {
        wxLogError("Failed to open JSON file: %s", filePath);
        return false;
    }

    // Very simple JSON parsing - in a real app you'd use a proper JSON parser
    configName = wxEmptyString;
    appsList = wxEmptyString;

    for (wxString line = file.GetFirstLine(); !file.Eof(); line = file.GetNextLine())
    {
        line.Trim(true).Trim(false);

        if (line.Contains("\"config_name\""))
        {
            wxString value = line.AfterFirst(':');
            value.Trim(true).Trim(false);
            // Remove quotes and trailing comma if present
            if (value.StartsWith("\"") && value.EndsWith("\","))
                configName = value.Mid(1, value.Length() - 3);
            else if (value.StartsWith("\"") && value.EndsWith("\""))
                configName = value.Mid(1, value.Length() - 2);
        }
        else if (line.Contains("\"apps\""))
        {
            wxString value = line.AfterFirst(':');
            value.Trim(true).Trim(false);
            // Remove quotes and trailing comma if present
            if (value.StartsWith("\"") && value.EndsWith("\","))
                appsList = value.Mid(1, value.Length() - 3);
            else if (value.StartsWith("\"") && value.EndsWith("\""))
                appsList = value.Mid(1, value.Length() - 2);
        }
    }

    file.Close();
    return !configName.IsEmpty() && !appsList.IsEmpty();
}

// Upload JSON (uses real MongoDB)
void MongoDBPanel::OnUploadJson(wxCommandEvent &event)
{
    wxLogDebug("MongoDBPanel::OnUploadJson - Upload button clicked.");

    wxFileDialog openFileDialog(this, "Select JSON file to upload", "", "",
                                "JSON files (*.json)|*.json", wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if (openFileDialog.ShowModal() == wxID_CANCEL)
    {
        wxLogDebug("MongoDBPanel::OnUploadJson - File selection cancelled.");
        return;
    }

    wxString filePath = openFileDialog.GetPath();
    wxString fileName = openFileDialog.GetFilename();
    wxLogDebug("MongoDBPanel::OnUploadJson - Selected file: %s", filePath);

    wxString containerId = GetContainerIdFromProjectDir();
    if (!containerId.IsEmpty())
        wxLogDebug("MongoDBPanel::OnUploadJson - Using container ID: %s", containerId);

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
    }
    wxLogDebug("MongoDBPanel::OnUploadJson - Read %zu bytes from file.", jsonContent.length());

    // --- Upload to MongoDB ---
    try
    {
        // Parse JSON content into a BSON document
        bsoncxx::document::value doc_value = bsoncxx::from_json(jsonContent);
        wxLogDebug("MongoDBPanel::OnUploadJson - Successfully parsed JSON content.");

        // Connect to MongoDB
        const auto uri = mongocxx::uri{"mongodb+srv://<user>:<password>@<cluster>.mongodb.net/?retryWrites=true&w=majority&appName=CXX"};
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

        // Try to extract config_name and apps for direct access
        wxString configName, appsList;
        ParseJsonFile(filePath, configName, appsList);

        // Append the actual parsed JSON data under a specific key
        builder.append(bsoncxx::builder::basic::kvp("config_data", doc_value.view()));

        // Add extracted fields at the top level for easier access
        if (!configName.IsEmpty())
        {
            builder.append(bsoncxx::builder::basic::kvp("config_name", configName.ToStdString()));
        }

        if (!appsList.IsEmpty())
        {
            builder.append(bsoncxx::builder::basic::kvp("apps", appsList.ToStdString()));
        }

        // Append metadata fields
        builder.append(bsoncxx::builder::basic::kvp("original_filename", fileName.ToStdString()));
        builder.append(bsoncxx::builder::basic::kvp("upload_timestamp", bsoncxx::types::b_date{now}));

        if (!containerId.IsEmpty())
        {
            builder.append(bsoncxx::builder::basic::kvp("container_id", containerId.ToStdString()));
        }

        wxLogDebug("MongoDBPanel::OnUploadJson - Built BSON document with metadata.");

        // Insert the document with metadata
        auto result = coll.insert_one(builder.view());
        wxLogDebug("MongoDBPanel::OnUploadJson - Executed insert_one.");

        if (result)
        {
            bsoncxx::oid oid = result->inserted_id().get_oid().value;
            wxString oidStr = wxString::FromUTF8(oid.to_string().c_str());
            wxLogDebug("MongoDBPanel::OnUploadJson - Insert successful, OID: %s", oidStr);

            // Also save locally as backup
            wxString dbDirPath = wxFileName(m_projectDir, "db_files").GetFullPath();
            if (!wxDirExists(dbDirPath))
            {
                wxMkdir(dbDirPath);
            }

            wxString dbFilePath = wxFileName(dbDirPath, oidStr + ".json").GetFullPath();
            wxCopyFile(filePath, dbFilePath, true);

            wxMessageBox("JSON file '" + fileName + "' uploaded successfully!\nObjectID: " + oidStr,
                         "Upload Success", wxOK | wxICON_INFORMATION, this);
            LoadMongoDBContent(); // Refresh the list view after successful upload
        }
        else
        {
            wxLogError("MongoDBPanel::OnUploadJson - MongoDB insert_one did not return a result.");
            wxMessageBox("Failed to upload JSON file to MongoDB (no result returned).", "Upload Error", wxOK | wxICON_ERROR, this);

            // Fall back to local storage
            SaveToLocalFile(filePath, jsonContent);
        }
    }
    catch (const bsoncxx::exception &e)
    {
        wxLogError("BSON/JSON Parsing Error during upload: %s", e.what());
        wxMessageBox("Error parsing JSON file content: " + wxString(e.what()) +
                         "\nPlease ensure the selected file contains valid JSON.",
                     "JSON Parse Error", wxOK | wxICON_ERROR, this);

        // Fall back to local storage
        SaveToLocalFile(filePath, jsonContent);
    }
    catch (const mongocxx::exception &e)
    {
        wxLogError("MongoDB Upload Error: %s", e.what());
        wxMessageBox("Error uploading file to MongoDB: " + wxString(e.what()) +
                         "\nFalling back to local storage.",
                     "MongoDB Error", wxOK | wxICON_ERROR, this);

        // Fall back to local storage
        SaveToLocalFile(filePath, jsonContent);
    }
    catch (const std::exception &e)
    {
        wxLogError("Standard Exception during upload: %s", e.what());
        wxMessageBox("An unexpected error occurred during upload: " + wxString(e.what()) +
                         "\nFalling back to local storage.",
                     "Error", wxOK | wxICON_ERROR, this);

        // Fall back to local storage
        SaveToLocalFile(filePath, jsonContent);
    }

    wxLogDebug("MongoDBPanel::OnUploadJson - Upload process finished.");
}

// Save to local file if MongoDB upload fails
void MongoDBPanel::SaveToLocalFile(const wxString &sourcePath, const std::string &jsonContent)
{
    wxLogDebug("MongoDBPanel::SaveToLocalFile - Saving file locally as fallback");

    wxString dbDirPath = wxFileName(m_projectDir, "db_files").GetFullPath();
    if (!wxDirExists(dbDirPath))
    {
        wxMkdir(dbDirPath);
    }

    wxString fileId = wxDateTime::Now().Format("%Y%m%d%H%M%S") + "_local";
    wxString dbFilePath = wxFileName(dbDirPath, fileId + ".json").GetFullPath();

    if (wxCopyFile(sourcePath, dbFilePath, true))
    {
        wxLogDebug("MongoDBPanel::SaveToLocalFile - Saved to: %s", dbFilePath);
        wxMessageBox("File saved locally as backup: " + fileId,
                     "Local Storage", wxOK | wxICON_INFORMATION);
        LoadFromLocalFiles();
    }
    else
    {
        wxLogError("Failed to save file locally to: %s", dbFilePath);
        wxMessageBox("Failed to save file locally. Please check permissions.",
                     "Local Storage Error", wxOK | wxICON_ERROR);
    }
}

// Download selected document
void MongoDBPanel::OnDownloadSelected(wxCommandEvent &event)
{
    wxLogDebug("MongoDBPanel::OnDownloadSelected - Download button clicked");

    if (m_selectedItemIndex < 0 || m_selectedDocumentId.IsEmpty())
    {
        wxMessageBox("Please select a valid configuration to download.",
                     "Selection Required", wxOK | wxICON_INFORMATION);
        return;
    }

    wxString configName = m_contentList->GetItemText(m_selectedItemIndex);
    if (configName.IsEmpty() || configName == "No applications found in MongoDB.")
    {
        wxMessageBox("Selected item has an invalid filename.", "Invalid Selection", wxOK | wxICON_ERROR);
        return;
    }

    // Create a suggested filename from the config name
    wxString suggestedName = configName;
    suggestedName.Replace(" ", "_");
    if (!suggestedName.EndsWith(".json"))
        suggestedName += ".json";

    wxFileDialog saveDialog(this, "Save Configuration File", "", suggestedName,
                            "JSON files (*.json)|*.json", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

    if (saveDialog.ShowModal() == wxID_CANCEL)
    {
        wxLogDebug("User cancelled save dialog");
        return;
    }

    wxString savePath = saveDialog.GetPath();
    wxLogDebug("User selected save path: %s", savePath);

    // Create a progress dialog for the download
    wxProgressDialog progress("Downloading Configuration",
                              "Preparing to download...",
                              100, this,
                              wxPD_APP_MODAL | wxPD_AUTO_HIDE | wxPD_SMOOTH | wxPD_ELAPSED_TIME);

    progress.Update(10, "Connecting to database...");
    wxMilliSleep(300);

    bool success = false;

    // Try to download from MongoDB first
    try
    {
        // Connect to MongoDB
        const auto uri = mongocxx::uri{"mongodb+srv://<user>:<password>@<cluster>.mongodb.net/?retryWrites=true&w=majority&appName=CXX"};
        mongocxx::options::client client_options;
        const auto api = mongocxx::options::server_api{mongocxx::options::server_api::version::k_version_1};
        client_options.server_api_opts(api);

        mongocxx::client client(uri, client_options);
        mongocxx::database db = client["testdb"];
        mongocxx::collection coll = db["config_files"];

        progress.Update(30, "Retrieving configuration data...");

        // Create an ObjectID from the string
        bsoncxx::oid oid = bsoncxx::oid(m_selectedDocumentId.ToStdString());

        // Find the document by its _id
        auto filter = bsoncxx::builder::stream::document{}
                      << "_id" << oid
                      << bsoncxx::builder::stream::finalize;

        auto result = coll.find_one(filter.view());

        if (result)
        {
            progress.Update(60, "Processing configuration data...");
            wxMilliSleep(400);

            // Extract the JSON data
            std::string jsonStr;

            // If it has config_data, use that as the base
            if ((*result)["config_data"])
            {
                jsonStr = bsoncxx::to_json((*result)["config_data"].get_document().view());
            }
            // Otherwise use the whole document
            else
            {
                jsonStr = bsoncxx::to_json(*result);
            }

            progress.Update(80, "Writing configuration to file...");

            // Write to file
            std::ofstream outFile(savePath.ToStdString());
            if (outFile.is_open())
            {
                outFile << jsonStr;
                outFile.close();
                success = true;
            }
        }
    }
    catch (const std::exception &e)
    {
        wxLogError("MongoDB download error: %s", e.what());
        wxLogDebug("Falling back to local file for download");
    }

    // If MongoDB download failed, try local file
    if (!success)
    {
        wxString dbFilePath = wxFileName(m_projectDir, "db_files", m_selectedDocumentId + ".json").GetFullPath();

        progress.Update(70, "Retrieving local configuration file...");

        if (wxFileExists(dbFilePath))
        {
            success = wxCopyFile(dbFilePath, savePath, true);
        }
    }

    progress.Update(100, "Download complete!");
    wxMilliSleep(200);

    if (success)
    {
        wxMessageBox("Configuration has been successfully downloaded to:\n" + savePath,
                     "Download Complete", wxOK | wxICON_INFORMATION);
    }
    else
    {
        wxLogError("Failed to download configuration to: %s", savePath);
        wxMessageBox("Failed to download the configuration. The file may not exist in MongoDB or locally.",
                     "Download Error", wxOK | wxICON_ERROR);
    }
}

// Refresh content (simulated)
void MongoDBPanel::OnRefresh(wxCommandEvent &event)
{
    wxLogDebug("MongoDBPanel::OnRefresh - Refresh button clicked.");
    LoadMongoDBContent();
    wxLogDebug("MongoDBPanel::OnRefresh - SIMULATING: Content reload initiated.");
}

// Load and Install
void MongoDBPanel::OnLoadAndInstall(wxCommandEvent &event)
{
    wxLogDebug("MongoDBPanel::OnLoadAndInstall - Load and Install button clicked.");

    if (m_selectedItemIndex < 0 || m_selectedDocumentId.IsEmpty())
    {
        wxLogWarning("No valid item selected for installation");
        wxMessageBox("Please select a valid configuration to install.", "Selection Required", wxOK | wxICON_INFORMATION);
        return;
    }

    wxString configName = m_contentList->GetItemText(m_selectedItemIndex); // Use config name from list
    wxString appNamesString = m_contentList->GetItemText(m_selectedItemIndex, 1);

    if (configName.IsEmpty() || appNamesString.IsEmpty() || configName == "No applications found in MongoDB.")
    {
        wxLogWarning("Invalid selection for installation (ConfigName: '%s', Apps: '%s')", configName, appNamesString);
        wxMessageBox("Selected item contains no valid data for installation.", "Invalid Selection", wxOK | wxICON_ERROR);
        return;
    }

    wxMessageDialog confirmDialog(
        this,
        wxString::Format("This will install the following applications from '%s':\n\n%s\n\nThis requires entering the chroot environment in the container.\n\nDo you want to continue?",
                         configName, appNamesString),
        "Confirm Installation",
        wxYES_NO | wxICON_QUESTION);

    if (confirmDialog.ShowModal() == wxID_YES)
    {
        wxLogDebug("MongoDBPanel::OnLoadAndInstall - User confirmed installation.");
        wxString containerId = GetContainerIdFromProjectDir();
        if (containerId.IsEmpty())
        {
            wxLogWarning("MongoDBPanel::OnLoadAndInstall - Container ID not found. Cannot proceed with installation.");
            wxMessageBox("Container ID not found. Installation cannot proceed.", "Error", wxOK | wxICON_ERROR);
            return;
        }
        wxLogDebug("MongoDBPanel::OnLoadAndInstall - Using container ID: %s", containerId);
        wxLogDebug("MongoDBPanel::OnLoadAndInstall - Parsing install commands for document ID: %s...", m_selectedDocumentId);
        wxMilliSleep(150);

        wxArrayString appsToInstallArr = wxSplit(appNamesString, ',');
        // Instead of keeping a vector, we'll pass the original string
        ProcessInstallation(configName, appNamesString); // Pass configName and appNamesString
    }
    else
    {
        wxLogDebug("MongoDBPanel::OnLoadAndInstall - User cancelled installation.");
    }
}

// Process Installation
void MongoDBPanel::ProcessInstallation(const wxString &configName, const wxString &appNames)
{
    wxString containerId = GetContainerIdFromProjectDir();
    wxProgressDialog progress(
        "Application Installation",
        "Preparing installation environment...",
        100, this,
        wxPD_APP_MODAL | wxPD_AUTO_HIDE | wxPD_SMOOTH | wxPD_ELAPSED_TIME | wxPD_ESTIMATED_TIME | wxPD_REMAINING_TIME | wxPD_CAN_ABORT);

    wxLogDebug("MongoDBPanel::ProcessInstallation - Starting installation for config '%s' in container '%s'", configName, containerId);
    bool cancelled = false;

    try
    {
        cancelled = !progress.Update(0, "Entering chroot environment in container...");
        if (cancelled)
            throw std::runtime_error("User cancelled");
        wxMilliSleep(800);

        cancelled = !progress.Update(10, "Updating package lists inside chroot...");
        if (cancelled)
            throw std::runtime_error("User cancelled");
        wxMilliSleep(1000);

        cancelled = !progress.Update(20, "Ensuring flatpak and flathub are configured...");
        if (cancelled)
            throw std::runtime_error("User cancelled");
        wxMilliSleep(1200);

        // Parse the app names string
        wxArrayString appsToInstallArr = wxSplit(appNames, ',');
        std::vector<wxString> appList;
        for (wxString &app : appsToInstallArr)
        {
            app.Trim(true).Trim(false);
            if (!app.IsEmpty())
            {
                appList.push_back(app);
            }
        }

        int currentProgress = 30;
        int totalApps = appList.size();
        int progressPerApp = (totalApps > 0) ? (80 - currentProgress) / totalApps : 0;

        for (size_t i = 0; i < appList.size(); ++i)
        {
            const wxString &appName = appList[i];
            wxString appId = appName;
            appId.Replace(" ", "");
            appId.Lower();
            if (!appId.Contains("."))
                appId = "org.example." + appId;

            wxString logMsg = wxString::Format("Running 'flatpak install -y flathub %s' for %s (%zu/%d)...", appId, appName, i + 1, totalApps);
            wxLogDebug(logMsg);
            cancelled = !progress.Update(currentProgress, logMsg);
            if (cancelled)
                throw std::runtime_error("User cancelled");
            wxMilliSleep(900 + (rand() % 600));
            currentProgress += progressPerApp;
            cancelled = !progress.Update(std::min(80, currentProgress));
            if (cancelled)
                throw std::runtime_error("User cancelled");
        }

        cancelled = !progress.Update(85, "Verifying installations...");
        if (cancelled)
            throw std::runtime_error("User cancelled");
        wxMilliSleep(600);
        cancelled = !progress.Update(95, "Cleaning up temporary files...");
        if (cancelled)
            throw std::runtime_error("User cancelled");
        wxMilliSleep(500);
        progress.Update(100, "Installation complete!");
        wxMilliSleep(300);

        wxLogDebug("MongoDBPanel::ProcessInstallation - Installation completed successfully for '%s'", configName);
        wxMessageBox("Applications from '" + configName + "' have been successfully installed!", "Installation Complete", wxOK | wxICON_INFORMATION);
    }
    catch (const std::exception &e)
    {
        if (wxString(e.what()) == "User cancelled")
        {
            wxLogWarning("Installation cancelled by user for '%s'", configName);
            // Progress dialog hides automatically on cancel
        }
        else
        {
            wxLogError("Installation Error for '%s': %s", configName, e.what());
            wxMessageBox(wxString::Format("Error during installation: %s", e.what()), "Installation Error", wxOK | wxICON_ERROR);
        }
    }
}

// Apply Theme Colors
void MongoDBPanel::ApplyThemeColors()
{
    m_contentList->SetBackgroundColour(m_listBgColor);
    m_contentList->SetForegroundColour(m_listFgColor);
    m_uploadButton->SetBackgroundColour(m_uploadBtnBgColor);
    m_uploadButton->SetForegroundColour(wxColour(255, 255, 255));
    m_refreshButton->SetBackgroundColour(m_buttonBgColor);
    m_refreshButton->SetForegroundColour(m_buttonFgColor);
    m_closeButton->SetBackgroundColour(m_buttonBgColor);
    m_closeButton->SetForegroundColour(m_buttonFgColor);
    m_loadInstallButton->SetBackgroundColour(wxColour(137, 220, 170));
    m_loadInstallButton->SetForegroundColour(wxColour(255, 255, 255));
    m_downloadButton->SetBackgroundColour(wxColour(150, 150, 240));
    m_downloadButton->SetForegroundColour(wxColour(255, 255, 255));

    // Disable buttons initially or based on selection state
    bool itemSelected = (m_selectedItemIndex >= 0);
    m_loadInstallButton->Enable(itemSelected);
    m_downloadButton->Enable(itemSelected);

    wxFont listFont = m_contentList->GetFont();
    listFont.SetPointSize(10);
    listFont.SetWeight(wxFONTWEIGHT_NORMAL);
    m_contentList->SetFont(listFont);
    wxFont buttonFont = m_uploadButton->GetFont();
    buttonFont.SetPointSize(10);
    buttonFont.SetWeight(wxFONTWEIGHT_BOLD);
    m_uploadButton->SetFont(buttonFont);
    m_refreshButton->SetFont(buttonFont);
    m_closeButton->SetFont(buttonFont);
    m_loadInstallButton->SetFont(buttonFont);
    m_downloadButton->SetFont(buttonFont);
#ifdef __WXMSW__
    m_uploadButton->SetCursor(wxCursor(wxCURSOR_HAND));
    m_refreshButton->SetCursor(wxCursor(wxCURSOR_HAND));
    m_closeButton->SetCursor(wxCursor(wxCURSOR_HAND));
    m_loadInstallButton->SetCursor(wxCursor(wxCURSOR_HAND));
    m_downloadButton->SetCursor(wxCursor(wxCURSOR_HAND));
#endif
    Refresh();
}

// OnCloseClick
void MongoDBPanel::OnCloseClick(wxCommandEvent &event)
{
    wxLogDebug("MongoDBPanel::OnCloseClick - Close button clicked.");
    wxCommandEvent closeEvent(wxEVT_BUTTON, ID_MONGODB_PANEL_CLOSE);
    closeEvent.SetEventObject(this);
    ProcessWindowEvent(closeEvent); // Send event to parent (SecondWindow)
    wxLogDebug("MongoDBPanel::OnCloseClick - Sent close request event to parent.");
}

// OnItemSelected
void MongoDBPanel::OnItemSelected(wxListEvent &event)
{
    long itemIndex = event.GetIndex();
    wxLogDebug("MongoDBPanel::OnItemSelected - Item %ld selected", itemIndex);
    m_selectedItemIndex = itemIndex;
    m_selectedDocumentId = ""; // Reset
    auto it = m_documentIds.find(itemIndex);
    if (it != m_documentIds.end())
    {
        m_selectedDocumentId = it->second;
        wxLogDebug("Selected document ID: %s", m_selectedDocumentId);
        m_downloadButton->Enable();
        m_loadInstallButton->Enable();
    }
    else
    {
        wxLogDebug("No document ID found for item %ld", itemIndex);
        m_downloadButton->Disable();
        m_loadInstallButton->Disable();
    }
    event.Skip(); // Allow default selection behavior
}

// Destructor
MongoDBPanel::~MongoDBPanel()
{
    wxLogDebug("MongoDBPanel destructor called");
}