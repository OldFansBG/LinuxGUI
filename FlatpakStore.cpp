#include "FlatpakStore.h"
#include <fstream>
#include <wx/filename.h>
#include <wx/wfstream.h>
#include <wx/mstream.h>

// Define event types
wxDEFINE_EVENT(wxEVT_SEARCH_COMPLETE, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_RESULT_READY, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_IMAGE_READY, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_UPDATE_PROGRESS, wxCommandEvent);

// Event tables
BEGIN_EVENT_TABLE(FlatpakStore, wxPanel)
    EVT_BUTTON(wxID_ANY, FlatpakStore::OnSearch)
    EVT_TEXT_ENTER(wxID_ANY, FlatpakStore::OnSearch)
    EVT_COMMAND(wxID_ANY, wxEVT_SEARCH_COMPLETE, FlatpakStore::OnSearchComplete)
    EVT_COMMAND(wxID_ANY, wxEVT_RESULT_READY, FlatpakStore::OnResultReady)
    EVT_COMMAND(wxID_ANY, wxEVT_IMAGE_READY, FlatpakStore::OnImageReady)
    EVT_COMMAND(wxID_ANY, wxEVT_UPDATE_PROGRESS, FlatpakStore::OnUpdateProgress)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(AppCard, wxPanel)
    EVT_PAINT(AppCard::OnPaint)
    EVT_ENTER_WINDOW(AppCard::OnMouseEnter)
    EVT_LEAVE_WINDOW(AppCard::OnMouseLeave)
END_EVENT_TABLE()

// Helper functions
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* buffer) {
    size_t newLength = size * nmemb;
    try {
        buffer->append((char*)contents, newLength);
    } catch (std::bad_alloc& e) {
        return 0;
    }
    return newLength;
}

size_t WriteCallbackBinary(void* contents, size_t size, size_t nmemb, std::vector<unsigned char>* buffer) {
    size_t newLength = size * nmemb;
    try {
        buffer->insert(buffer->end(), (unsigned char*)contents, (unsigned char*)contents + newLength);
    } catch (std::bad_alloc& e) {
        return 0;
    }
    return newLength;
}

bool DownloadImage(const std::string& url, std::vector<unsigned char>& buffer, std::atomic<bool>& stopFlag) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallbackBinary);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || stopFlag) {
        wxBitmap placeholderBmp(48, 48);
        wxMemoryDC dc;
        dc.SelectObject(placeholderBmp);
        dc.SetBackground(wxBrush(wxColour(79, 70, 229)));
        dc.Clear();
        dc.SetTextForeground(*wxWHITE);
        dc.DrawText("App", 16, 16);
        dc.SelectObject(wxNullBitmap);
        
        wxImage img = placeholderBmp.ConvertToImage();
        wxMemoryOutputStream memStream;
        img.SaveFile(memStream, wxBITMAP_TYPE_PNG);
        
        size_t size = memStream.GetLength();
        buffer.resize(size);
        memStream.CopyTo(buffer.data(), size);
    }
    
    return !buffer.empty();
}

std::string searchFlathubApps(const std::string& query, std::atomic<bool>& stopFlag) {
    CURL* curl = curl_easy_init();
    if (!curl) return "";

    std::string response;
    std::string url = "https://flathub.org/api/v2/search";
    std::string payload = "{\"query\":\"" + query + "\",\"filters\":[]}";

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

    CURLcode res = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || stopFlag) {
        return "";
    }

    return response;
}

std::string fetchRecentlyAddedApps(std::atomic<bool>& stopFlag) {
    CURL* curl = curl_easy_init();
    if (!curl) return "";

    std::string response;
    std::string url = "https://flathub.org/api/v2/collection/recently-added";

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || stopFlag) {
        return "";
    }

    return response;
}

// AppCard implementation
AppCard::AppCard(wxWindow* parent, const wxString& name, const wxString& summary, const wxString& appId)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(320, 160), wxBORDER_NONE),
      m_appId(appId)
{
    SetBackgroundColour(wxColour(55, 65, 81));
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    
    wxBoxSizer* mainSizer = new wxBoxSizer(wxHORIZONTAL);
    mainSizer->AddSpacer(12);

    m_iconBitmap = new wxStaticBitmap(this, wxID_ANY, wxBitmap(64, 64));
    mainSizer->Add(m_iconBitmap, 0, wxALIGN_CENTER_VERTICAL | wxTOP | wxBOTTOM, 12);

    wxBoxSizer* textSizer = new wxBoxSizer(wxVERTICAL);
    textSizer->AddSpacer(8);
    
    m_nameText = new wxStaticText(this, wxID_ANY, name);
    m_nameText->SetForegroundColour(wxColour(229, 229, 229));
    wxFont nameFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
    m_nameText->SetFont(nameFont);
    
    m_summaryText = new wxStaticText(this, wxID_ANY, summary);
    m_summaryText->SetForegroundColour(wxColour(156, 163, 175));
    m_summaryText->Wrap(220);
    
    textSizer->Add(m_nameText, 0, wxBOTTOM, 6);
    textSizer->Add(m_summaryText, 1, wxEXPAND);

    wxBoxSizer* rightSizer = new wxBoxSizer(wxVERTICAL);
    rightSizer->Add(textSizer, 1, wxEXPAND);
    
    m_installButton = new wxButton(this, wxID_ANY, "Install", wxDefaultPosition, wxSize(90, 34));
    m_installButton->SetBackgroundColour(wxColour(79, 70, 229));
    m_installButton->SetForegroundColour(*wxWHITE);
    rightSizer->Add(m_installButton, 0, wxALIGN_RIGHT | wxBOTTOM | wxRIGHT, 12);

    mainSizer->Add(rightSizer, 1, wxEXPAND | wxLEFT, 12);
    SetSizer(mainSizer);
    
    m_installButton->SetClientData(new wxString(appId));
}

AppCard::~AppCard() {
    wxString* appIdPtr = static_cast<wxString*>(m_installButton->GetClientData());
    if (appIdPtr) {
        delete appIdPtr;
        m_installButton->SetClientData(nullptr);
    }
}

void AppCard::SetIcon(const wxBitmap& bitmap) {
    m_iconBitmap->SetBitmap(bitmap);
    Refresh();
}

void AppCard::OnPaint(wxPaintEvent& event) {
    wxPaintDC dc(this);
    wxRect rect = GetClientRect();
    dc.SetBrush(wxBrush(GetBackgroundColour()));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRoundedRectangle(rect, 10);
}

void AppCard::OnMouseEnter(wxMouseEvent& event) {
    SetBackgroundColour(wxColour(65, 75, 91));
    Refresh();
}

void AppCard::OnMouseLeave(wxMouseEvent& event) {
    SetBackgroundColour(wxColour(55, 65, 81));
    Refresh();
}

// SearchThread implementation
wxThread::ExitCode SearchThread::Entry() {
    try {
        if (TestDestroy() || m_stopFlag) return (ExitCode)0;

        std::string response = searchFlathubApps(m_query, m_stopFlag);
        if (response.empty()) return (ExitCode)1;

        rapidjson::Document doc;
        if (doc.Parse(response.c_str()).HasParseError()) return (ExitCode)1;
        if (!doc.IsObject() || !doc.HasMember("hits") || !doc["hits"].IsArray()) return (ExitCode)1;

        const rapidjson::Value& hits = doc["hits"];
        m_store->SetTotalResults(hits.Size());

        for (rapidjson::SizeType i = 0; i < hits.Size() && !m_stopFlag && !TestDestroy(); i++) {
            const rapidjson::Value& hit = hits[i];
            if (!hit.IsObject() || !hit.HasMember("name") || !hit.HasMember("summary") || 
                !hit.HasMember("icon") || !hit.HasMember("app_id")) continue;

            wxString resultData = wxString(hit["name"].GetString(), wxConvUTF8) + "|" +
                                  wxString(hit["summary"].GetString(), wxConvUTF8) + "|" +
                                  wxString(hit["icon"].GetString(), wxConvUTF8) + "|" +
                                  wxString(hit["app_id"].GetString(), wxConvUTF8);

            wxCommandEvent resultEvent(wxEVT_RESULT_READY);
            resultEvent.SetInt(m_searchId);
            resultEvent.SetString(resultData);
            wxQueueEvent(m_store, resultEvent.Clone());

            wxCommandEvent progressEvent(wxEVT_UPDATE_PROGRESS);
            progressEvent.SetInt(i + 1);
            wxQueueEvent(m_store, progressEvent.Clone());
            
            wxMilliSleep(5); // Small delay to keep UI responsive
        }

        if (!m_stopFlag && !TestDestroy()) {
            wxCommandEvent event(wxEVT_SEARCH_COMPLETE);
            event.SetInt(m_searchId);
            wxQueueEvent(m_store, event.Clone());
        }

        return (ExitCode)0;
    }
    catch (...) { 
        return (ExitCode)1; 
    }
}

// InitialLoadThread implementation
wxThread::ExitCode InitialLoadThread::Entry() {
    try {
        if (TestDestroy() || m_stopFlag) return (ExitCode)0;

        std::string response = fetchRecentlyAddedApps(m_stopFlag);
        if (response.empty()) return (ExitCode)1;

        rapidjson::Document doc;
        if (doc.Parse(response.c_str()).HasParseError()) return (ExitCode)1;
        if (!doc.IsObject() || !doc.HasMember("hits") || !doc["hits"].IsArray()) return (ExitCode)1;

        const rapidjson::Value& hits = doc["hits"];
        m_store->SetTotalResults(hits.Size());

        for (rapidjson::SizeType i = 0; i < hits.Size() && !m_stopFlag && !TestDestroy(); i++) {
            const rapidjson::Value& hit = hits[i];
            if (!hit.IsObject() || !hit.HasMember("name") || !hit.HasMember("summary") || 
                !hit.HasMember("icon") || !hit.HasMember("app_id")) continue;

            wxString resultData = wxString(hit["name"].GetString(), wxConvUTF8) + "|" +
                                  wxString(hit["summary"].GetString(), wxConvUTF8) + "|" +
                                  wxString(hit["icon"].GetString(), wxConvUTF8) + "|" +
                                  wxString(hit["app_id"].GetString(), wxConvUTF8);

            wxCommandEvent resultEvent(wxEVT_RESULT_READY);
            resultEvent.SetInt(-1);  // Special ID for initial load
            resultEvent.SetString(resultData);
            wxQueueEvent(m_store, resultEvent.Clone());

            wxCommandEvent progressEvent(wxEVT_UPDATE_PROGRESS);
            progressEvent.SetInt(i + 1);
            wxQueueEvent(m_store, progressEvent.Clone());
            
            wxMilliSleep(5); // Small delay to keep UI responsive
        }

        if (!m_stopFlag && !TestDestroy()) {
            wxCommandEvent event(wxEVT_SEARCH_COMPLETE);
            event.SetInt(-1);  // Special ID for initial load
            wxQueueEvent(m_store, event.Clone());
        }

        return (ExitCode)0;
    }
    catch (...) { 
        return (ExitCode)1; 
    }
}

// FlatpakStore implementation
FlatpakStore::FlatpakStore(wxWindow* parent)
    : wxPanel(parent, wxID_ANY),
      m_isSearching(false), 
      m_isInitialLoading(false),
      totalResults(0),
      m_stopFlag(false), 
      m_searchId(0)
{
    m_threadPool = std::make_unique<ThreadPool>(4);
    SetBackgroundColour(wxColour(40, 44, 52));

    // Main layout
    m_mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Search panel
    wxPanel* searchPanel = new wxPanel(this, wxID_ANY);
    searchPanel->SetBackgroundColour(wxColour(40, 44, 52));
    
    wxBoxSizer* searchSizer = new wxBoxSizer(wxHORIZONTAL);
    
    m_searchBox = new wxTextCtrl(searchPanel, wxID_ANY, "", wxDefaultPosition, 
                          wxSize(-1, 42), wxTE_PROCESS_ENTER);
    m_searchBox->SetBackgroundColour(wxColour(55, 65, 81));
    m_searchBox->SetForegroundColour(wxColour(229, 229, 229));
    m_searchBox->SetHint("Search for applications...");
    
    m_searchButton = new wxButton(searchPanel, wxID_ANY, "Search", 
                            wxDefaultPosition, wxSize(100, 42));
    m_searchButton->SetBackgroundColour(wxColour(79, 70, 229));
    m_searchButton->SetForegroundColour(*wxWHITE);
    
    searchSizer->Add(m_searchBox, 1, wxEXPAND | wxRIGHT, 12);
    searchSizer->Add(m_searchButton, 0);
    searchPanel->SetSizer(searchSizer);
    
    // Progress panel
    m_progressPanel = new wxPanel(this, wxID_ANY);
    m_progressPanel->SetBackgroundColour(wxColour(40, 44, 52));
    
    wxBoxSizer* progressSizer = new wxBoxSizer(wxVERTICAL);
    
    m_progressText = new wxStaticText(m_progressPanel, wxID_ANY, "");
    m_progressText->SetForegroundColour(wxColour(229, 229, 229));
    progressSizer->Add(m_progressText, 0, wxEXPAND | wxBOTTOM, 5);
    
    m_progressBar = new wxGauge(m_progressPanel, wxID_ANY, 100, wxDefaultPosition, wxSize(-1, 8));
    m_progressBar->SetBackgroundColour(wxColour(55, 65, 81));
    m_progressBar->SetForegroundColour(wxColour(79, 70, 229));
    progressSizer->Add(m_progressBar, 0, wxEXPAND);
    
    m_progressPanel->SetSizer(progressSizer);
    m_progressPanel->Hide(); // Initially hidden

    // Results panel
    m_resultsPanel = new wxScrolledWindow(this, wxID_ANY);
    m_resultsPanel->SetBackgroundColour(wxColour(40, 44, 52));
    m_resultsPanel->SetScrollRate(0, 20);
    
    // Initialize results grid - 2 columns with equal size
    m_gridSizer = new wxFlexGridSizer(0, 2, 15, 15); // rows, cols, vgap, hgap
    m_gridSizer->AddGrowableCol(0, 1);
    m_gridSizer->AddGrowableCol(1, 1);
    
    wxBoxSizer* resultsSizer = new wxBoxSizer(wxVERTICAL);
    resultsSizer->Add(m_gridSizer, 1, wxEXPAND | wxALL, 15);
    m_resultsPanel->SetSizer(resultsSizer);

    // Add panels to main sizer
    m_mainSizer->Add(searchPanel, 0, wxEXPAND | wxALL, 15);
    m_mainSizer->Add(m_progressPanel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 15);
    m_mainSizer->Add(m_resultsPanel, 1, wxEXPAND);
    
    SetSizer(m_mainSizer);
    
    // Load initial apps
    LoadInitialApps();

    // Load container ID
    std::ifstream file("container_id.txt");
    if (file.is_open()) {
        std::string containerId;
        std::getline(file, containerId);
        m_containerId = wxString(containerId);
        file.close();
    }
    
    // Bind install button event to all future AppCard install buttons
    this->Bind(wxEVT_BUTTON, &FlatpakStore::OnInstallButtonClicked, this);
}

void FlatpakStore::SetContainerId(const wxString& containerId) {
    m_containerId = containerId;
}

void FlatpakStore::ClearResults() {
    // Detach sizer before clearing to prevent UI artifacts
    m_resultsPanel->SetSizer(nullptr, false);
    
    if (m_gridSizer) {
        m_gridSizer->Clear(true); // Remove and delete all items
    }
    
    // Create a new grid sizer
    m_gridSizer = new wxFlexGridSizer(0, 2, 15, 15); // rows, cols, vgap, hgap
    m_gridSizer->AddGrowableCol(0, 1);
    m_gridSizer->AddGrowableCol(1, 1);
    
    // Re-attach sizer
    wxBoxSizer* resultsSizer = new wxBoxSizer(wxVERTICAL);
    resultsSizer->Add(m_gridSizer, 1, wxEXPAND | wxALL, 15);
    m_resultsPanel->SetSizer(resultsSizer);
    
    // Reset scroll position
    m_resultsPanel->Scroll(0, 0);
    
    // Queue a layout update
    m_resultsPanel->Layout();
}

void FlatpakStore::LoadInitialApps() {
    try {
        std::lock_guard<std::mutex> lock(m_searchMutex);

        // Cancel any ongoing search
        m_stopFlag = true;
        wxMilliSleep(50);  // Allow cancellation to propagate

        // Reset state
        ClearResults();
        m_progressBar->SetValue(0);
        m_progressText->SetLabel("Loading recently added apps...");
        m_progressPanel->Show();
        Layout();
        
        totalResults = 0;
        m_stopFlag = false;
        m_isInitialLoading = true;
        m_allResults.clear();
        
        // Create and run thread
        InitialLoadThread* thread = new InitialLoadThread(this, m_stopFlag);
        if (thread->Run() != wxTHREAD_NO_ERROR) {
            delete thread;
            throw std::runtime_error("Failed to start initial load thread");
        }
    }
    catch (const std::exception& e) {
        m_isInitialLoading = false;
        m_progressPanel->Hide();
        Layout();
        wxMessageBox(wxString::Format("Initial load failed: %s", e.what()),
                    "Error", wxOK | wxICON_ERROR);
    }
}

void FlatpakStore::OnSearch(wxCommandEvent& event) {
    try {
        std::lock_guard<std::mutex> lock(m_searchMutex);

        wxString query = m_searchBox->GetValue();
        if (query.IsEmpty()) {
            wxMessageBox("Please enter a search term", "Search Error", wxOK | wxICON_INFORMATION);
            return;
        }

        // Cancel existing operations
        m_stopFlag = true;
        wxMilliSleep(50);  // Allow cancellation to propagate

        // Reset search state
        ClearResults();
        m_progressBar->SetValue(0);
        m_progressText->SetLabel(wxString::Format("Searching for '%s'...", query));
        m_progressPanel->Show();
        Layout();
        
        totalResults = 0;
        m_stopFlag = false;
        m_searchId++;
        m_isSearching = true;
        m_isInitialLoading = false;
        m_allResults.clear();

        // Create and run thread
        SearchThread* thread = new SearchThread(this, query.ToStdString(), m_searchId, m_stopFlag);
        if (thread->Run() != wxTHREAD_NO_ERROR) {
            delete thread;
            throw std::runtime_error("Failed to start search thread");
        }
    }
    catch (const std::exception& e) {
        m_isSearching = false;
        m_progressPanel->Hide();
        Layout();
        wxMessageBox(wxString::Format("Search failed: %s", e.what()),
                    "Error", wxOK | wxICON_ERROR);
    }
}

void FlatpakStore::OnSearchComplete(wxCommandEvent& event) {
    if (event.GetInt() != m_searchId && event.GetInt() != -1) return;

    m_isSearching = false;
    m_isInitialLoading = false;
    
    // Process all accumulated results at once to prevent UI artifacts
    ProcessAllResults();
    
    m_progressBar->SetValue(100);
    m_progressText->SetLabel("Loading complete!");
    
    // Hide progress panel after a short delay
    wxTimer* timer = new wxTimer(this);
    timer->StartOnce(1000);
    timer->Bind(wxEVT_TIMER, [this, timer](wxTimerEvent&) {
        m_progressPanel->Hide();
        Layout();
        delete timer;
    });

    m_resultsPanel->Layout();
    m_resultsPanel->FitInside();
    m_resultsPanel->Refresh();
}

void FlatpakStore::ProcessAllResults() {
    // Make sure we have results to process
    if (m_allResults.empty()) return;
    
    // Sort by index to ensure correct order
    std::sort(m_allResults.begin(), m_allResults.end(), 
             [](const ResultData& a, const ResultData& b) { return a.index < b.index; });
    
    // Get a reference to prevent modification during processing
    const auto resultsCopy = m_allResults;
    
    // Clear the grid first to prevent any UI artifacts
    m_gridSizer->Clear(true);
    
    // Add all apps to the grid
    for (const auto& result : resultsCopy) {
        // Create app card
        AppCard* card = new AppCard(m_resultsPanel, result.name, result.summary, result.appId);
        m_gridSizer->Add(card, 1, wxEXPAND);
        
        // Queue icon download
        m_threadPool->Enqueue([this, card, iconUrl = result.iconUrl]() {
            std::vector<unsigned char> iconData;
            if (DownloadImage(iconUrl.ToStdString(), iconData, m_stopFlag)) {
                wxMemoryInputStream stream(iconData.data(), iconData.size());
                wxImage image;
                if (image.LoadFile(stream, wxBITMAP_TYPE_PNG)) {
                    const int target_size = 64;
                    float scale = std::min(static_cast<float>(target_size) / image.GetWidth(), 
                                          static_cast<float>(target_size) / image.GetHeight());
                    
                    image.Rescale(static_cast<int>(image.GetWidth() * scale),
                                 static_cast<int>(image.GetHeight() * scale),
                                 wxIMAGE_QUALITY_HIGH);

                    if (image.GetWidth() != target_size || image.GetHeight() != target_size) {
                        wxImage padded(target_size, target_size);
                        padded.SetRGB(wxRect(0, 0, target_size, target_size), 79, 70, 229);
                        padded.Paste(image, 
                                    (target_size - image.GetWidth())/2,
                                    (target_size - image.GetHeight())/2);
                        image = padded;
                    }

                    wxBitmap bitmap(image);
                    
                    wxCommandEvent imageEvent(wxEVT_IMAGE_READY);
                    imageEvent.SetClientData(new IconData{card, bitmap});
                    wxQueueEvent(this, imageEvent.Clone());
                }
            }
        });
    }
    
    // Update the layout
    m_resultsPanel->Layout();
    m_resultsPanel->FitInside();
}

void FlatpakStore::HandleInstallClick(const wxString& appId) {
    if (m_containerId.IsEmpty()) {
        wxMessageBox("Container ID is not available. Cannot install applications.", 
                    "Installation Error", wxOK | wxICON_ERROR);
        return;
    }
    
    wxProgressDialog progressDialog("Installing Application",
                                  "Installing " + appId + "...",
                                  100, this,
                                  wxPD_APP_MODAL | wxPD_AUTO_HIDE);
    
    progressDialog.Pulse();
    
    wxString installCommand = wxString::Format(
        "docker exec %s /bin/bash -c \"chroot /root/custom_iso/squashfs-root /bin/bash -c 'flatpak install -y flathub %s'\"",
        m_containerId, appId
    );
    
    wxArrayString output, errors;
    int result = wxExecute(installCommand, output, errors, wxEXEC_SYNC);
    
    if (result == 0) {
        progressDialog.Update(100);
        wxMessageBox("Successfully installed " + appId, "Installation Complete", 
                    wxOK | wxICON_INFORMATION);
    } else {
        wxString errorMsg = "Installation failed for " + appId + "\n\nErrors:\n";
        for (const auto& error : errors) {
            errorMsg += error + "\n";
        }
        wxMessageBox(errorMsg, "Installation Error", wxOK | wxICON_ERROR);
    }
}

void FlatpakStore::OnInstallButtonClicked(wxCommandEvent& event) {
    wxButton* button = dynamic_cast<wxButton*>(event.GetEventObject());
    if (!button) return;
    
    wxString* appIdPtr = static_cast<wxString*>(button->GetClientData());
    if (!appIdPtr) return;
    
    wxString appId = *appIdPtr;
    HandleInstallClick(appId);
}

void FlatpakStore::OnResultReady(wxCommandEvent& event) {
    if (event.GetInt() != m_searchId && event.GetInt() != -1) return;

    // Parse the data
    std::string resultData = event.GetString().ToStdString();
    std::istringstream iss(resultData);
    std::string name, summary, iconUrl, appId;

    std::getline(iss, name, '|');
    std::getline(iss, summary, '|');
    std::getline(iss, iconUrl, '|');
    std::getline(iss, appId, '|');
    
    // Store the result for later processing
    ResultData data;
    data.index = m_allResults.size();
    data.name = name;
    data.summary = summary;
    data.iconUrl = iconUrl;
    data.appId = appId;
    
    m_allResults.push_back(data);
    
    // Update progress
    if (totalResults > 0) {
        int progressPercent = static_cast<int>((m_allResults.size() * 100) / totalResults);
        m_progressBar->SetValue(progressPercent);
        m_progressText->SetLabel(wxString::Format("Loading applications: %zu of %d", 
                                                m_allResults.size(), totalResults));
    } else {
        m_progressBar->Pulse();
    }
}

void FlatpakStore::OnImageReady(wxCommandEvent& event) {
    IconData* data = static_cast<IconData*>(event.GetClientData());
    if (data) {
        data->card->SetIcon(data->bitmap);
        delete data;
    }
}

void FlatpakStore::OnUpdateProgress(wxCommandEvent& event) {
    // Simple pulse animation if totalResults is not known
    if (totalResults <= 0) {
        static int pulseVal = 0;
        pulseVal = (pulseVal + 5) % 100;
        m_progressBar->SetValue(pulseVal);
        return;
    }
    
    // Otherwise, show actual progress
    int current = event.GetInt();
    int progressPercent = (current * 100) / totalResults;
    m_progressBar->SetValue(progressPercent);
    
    m_progressText->SetLabel(wxString::Format("Loading applications: %d of %d", 
                                            current, totalResults));
}

// ThreadPool implementation
ThreadPool::ThreadPool(size_t threads) : stop(false) {
    for (size_t i = 0; i < threads; ++i) {
        workers.emplace_back([this] {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(queueMutex);
                    condition.wait(lock, [this] { 
                        return stop || !tasks.empty(); 
                    });
                    if (stop && tasks.empty()) return;
                    task = std::move(tasks.front());
                    tasks.pop();
                }
                task();
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        stop = true;
    }
    condition.notify_all();
    for (std::thread& worker : workers) {
        if (worker.joinable()) worker.join();
    }
}

void ThreadPool::CancelAll() {
    std::unique_lock<std::mutex> lock(queueMutex);
    while (!tasks.empty()) {
        tasks.pop();
    }
}

FlatpakStore::~FlatpakStore() {
    try {
        std::lock_guard<std::mutex> lock(m_searchMutex);
        m_stopFlag = true;

        if (m_threadPool) {
            m_threadPool->CancelAll();
            m_threadPool.reset();
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error during cleanup: " << e.what() << std::endl;
    }
}
