// FlatpakStore.cpp (Updated)

#include "FlatpakStore.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <wx/tokenzr.h>
#include <iostream>

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

// Helper Functions

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* buffer) {
    size_t newLength = size * nmemb;
    try {
        buffer->append(reinterpret_cast<char*>(contents), newLength);
    } catch (std::bad_alloc& e) {
        return 0;
    }
    return newLength;
}

size_t WriteCallbackBinary(void* contents, size_t size, size_t nmemb, std::vector<unsigned char>* buffer) {
    size_t newLength = size * nmemb;
    try {
        buffer->insert(buffer->end(), reinterpret_cast<unsigned char*>(contents),
            reinterpret_cast<unsigned char*>(contents) + newLength);
    } catch (std::bad_alloc& e) {
        return 0;
    }
    return newLength;
}

bool DownloadImage(const std::string& url, std::vector<unsigned char>& buffer, std::atomic<bool>& stopFlag) {
    CURL* curl = curl_easy_init();
    if (!curl)
        return false;
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallbackBinary);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || stopFlag) {
        // Create a placeholder image (solid color only, no text)
        wxBitmap placeholderBmp(48, 48);
        wxMemoryDC dc;
        dc.SelectObject(placeholderBmp);
        dc.SetBackground(wxBrush(wxColour(79, 70, 229)));
        dc.Clear();
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
    if (!curl)
        return "";

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
    if (!curl)
        return "";

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

// AppCard Implementation

AppCard::AppCard(wxWindow* parent, const wxString& name, const wxString& summary, const wxString& appId)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(320, 160), wxBORDER_NONE),
      m_appId(appId), m_isLoading(true)
{
    // Set background style to prevent flickering
    SetBackgroundStyle(wxBG_STYLE_PAINT);

    SetBackgroundColour(wxColour(55, 65, 81));
    
    wxBoxSizer* mainSizer = new wxBoxSizer(wxHORIZONTAL);
    mainSizer->AddSpacer(12);

    // Create placeholder icon (solid color only, no text)
    wxBitmap placeholderBmp(64, 64);
    {
        wxMemoryDC dc;
        dc.SelectObject(placeholderBmp);
        dc.SetBackground(wxBrush(wxColour(79, 70, 229)));
        dc.Clear();
        dc.SelectObject(wxNullBitmap);
    }
    
    m_iconBitmap = new wxStaticBitmap(this, wxID_ANY, placeholderBmp);
    mainSizer->Add(m_iconBitmap, 0, wxALIGN_CENTER_VERTICAL | wxTOP | wxBOTTOM, 12);

    wxBoxSizer* textSizer = new wxBoxSizer(wxVERTICAL);
    textSizer->AddSpacer(8);
    
    // Create placeholder text if name is empty
    wxString displayName = name.IsEmpty() ? "Loading app..." : name;
    m_nameText = new wxStaticText(this, wxID_ANY, displayName);
    m_nameText->SetForegroundColour(wxColour(229, 229, 229));
    wxFont nameFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
    m_nameText->SetFont(nameFont);
    
    // Create placeholder summary if empty
    wxString displaySummary = summary.IsEmpty() ? "Loading description..." : summary;
    m_summaryText = new wxStaticText(this, wxID_ANY, displaySummary);
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
    m_isLoading = false;
    Refresh();
}

void AppCard::OnPaint(wxPaintEvent& event) {
    wxAutoBufferedPaintDC dc(this); // Use buffered DC to prevent flicker
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

// SearchThread Implementation

wxThread::ExitCode SearchThread::Entry() {
    try {
        if (TestDestroy() || m_stopFlag)
            return (ExitCode)0;
        
        std::string response = searchFlathubApps(m_query, m_stopFlag);
        if (response.empty())
            return (ExitCode)1;
        
        rapidjson::Document doc;
        if (doc.Parse(response.c_str()).HasParseError())
            return (ExitCode)1;
        if (!doc.IsObject() || !doc.HasMember("hits") || !doc["hits"].IsArray())
            return (ExitCode)1;
        
        const rapidjson::Value& hits = doc["hits"];
        // Set the total results count (even if some items will be skipped)
        m_store->SetTotalResults(hits.Size());
        
        for (rapidjson::SizeType i = 0; i < hits.Size() && !m_stopFlag && !TestDestroy(); i++) {
            const rapidjson::Value& hit = hits[i];
            if (!hit.IsObject() || !hit.HasMember("name") || !hit.HasMember("summary") ||
                !hit.HasMember("icon") || !hit.HasMember("app_id"))
                continue;
            
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
            
            wxMilliSleep(5);
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

// InitialLoadThread Implementation

wxThread::ExitCode InitialLoadThread::Entry() {
    try {
        if (TestDestroy() || m_stopFlag)
            return (ExitCode)0;
        
        std::string response = fetchRecentlyAddedApps(m_stopFlag);
        if (response.empty())
            return (ExitCode)1;
        
        rapidjson::Document doc;
        if (doc.Parse(response.c_str()).HasParseError())
            return (ExitCode)1;
        if (!doc.IsObject() || !doc.HasMember("hits") || !doc["hits"].IsArray())
            return (ExitCode)1;
        
        const rapidjson::Value& hits = doc["hits"];
        m_store->SetTotalResults(hits.Size());
        
        for (rapidjson::SizeType i = 0; i < hits.Size() && !m_stopFlag && !TestDestroy(); i++) {
            const rapidjson::Value& hit = hits[i];
            if (!hit.IsObject() || !hit.HasMember("name") || !hit.HasMember("summary") ||
                !hit.HasMember("icon") || !hit.HasMember("app_id"))
                continue;
            
            wxString resultData = wxString(hit["name"].GetString(), wxConvUTF8) + "|" +
                                  wxString(hit["summary"].GetString(), wxConvUTF8) + "|" +
                                  wxString(hit["icon"].GetString(), wxConvUTF8) + "|" +
                                  wxString(hit["app_id"].GetString(), wxConvUTF8);
            
            wxCommandEvent resultEvent(wxEVT_RESULT_READY);
            resultEvent.SetInt(-1);  // special id for initial load
            resultEvent.SetString(resultData);
            wxQueueEvent(m_store, resultEvent.Clone());
            
            wxCommandEvent progressEvent(wxEVT_UPDATE_PROGRESS);
            progressEvent.SetInt(i + 1);
            wxQueueEvent(m_store, progressEvent.Clone());
            
            wxMilliSleep(5);
        }
        
        if (!m_stopFlag && !TestDestroy()) {
            wxCommandEvent event(wxEVT_SEARCH_COMPLETE);
            event.SetInt(-1);
            wxQueueEvent(m_store, event.Clone());
        }
        return (ExitCode)0;
    }
    catch (...) {
        return (ExitCode)1;
    }
}

// FlatpakStore Implementation
FlatpakStore::FlatpakStore(wxWindow* parent)
    : wxPanel(parent, wxID_ANY),
      m_isSearching(false),
      m_isInitialLoading(false),
      totalResults(0),
      m_stopFlag(false),
      m_searchId(0),
      m_displayedResults(0)
{
    // Enable double buffering for the entire panel
    SetDoubleBuffered(true);

    // Initialize thread pool
    m_threadPool = std::make_unique<ThreadPool>(4);

    // Set background color
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

    // Results panel with a responsive (wrap) layout
    m_resultsPanel = new wxScrolledWindow(this, wxID_ANY);
    m_resultsPanel->SetDoubleBuffered(true); // Enable double buffering for smooth scrolling
    m_resultsPanel->SetBackgroundColour(wxColour(40, 44, 52));
    m_resultsPanel->SetScrollRate(0, 20);

    // Create a wxBoxSizer for the results area - this will contain the grid sizer
    wxBoxSizer* resultsSizer = new wxBoxSizer(wxVERTICAL);
    
    // Create a wxWrapSizer with HORIZONTAL orientation and no gap between items
    m_gridSizer = new wxWrapSizer(wxHORIZONTAL, wxWRAPSIZER_DEFAULT_FLAGS);
    
    // Add the grid sizer to the results sizer with padding
    resultsSizer->Add(m_gridSizer, 1, wxEXPAND | wxALL, 15);
    m_resultsPanel->SetSizer(resultsSizer);

    // Add panels to main sizer
    m_mainSizer->Add(searchPanel, 0, wxEXPAND | wxALL, 15);
    m_mainSizer->Add(m_progressPanel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 15);
    m_mainSizer->Add(m_resultsPanel, 1, wxEXPAND);

    SetSizer(m_mainSizer);

    // Load initial apps
    LoadInitialApps();

    // Load container ID from file
    std::ifstream file("container_id.txt");
    if (file.is_open()) {
        std::string containerId;
        std::getline(file, containerId);
        m_containerId = wxString(containerId);
        file.close();
    }

    // Bind install button events for future AppCard buttons
    this->Bind(wxEVT_BUTTON, &FlatpakStore::OnInstallButtonClicked, this);
}

void FlatpakStore::SetContainerId(const wxString& containerId) {
    m_containerId = containerId;
}

void FlatpakStore::ClearResults() {
    // Remove any stray UI elements by freezing updates first
    m_resultsPanel->Freeze();
    
    // Clear all child windows and delete them
    m_gridSizer->Clear(true);
    
    // Reset the displayed results counter
    m_displayedResults = 0;
    
    // Reset the scroll position to top
    m_resultsPanel->Scroll(0, 0);
    
    // Make sure all layouts are properly updated
    m_resultsPanel->Layout();
    
    // Refresh the panel to ensure all remnants are gone
    m_resultsPanel->Refresh();
    
    // Resume UI updates
    m_resultsPanel->Thaw();
}

void FlatpakStore::LoadInitialApps() {
    try {
        std::lock_guard<std::mutex> lock(m_searchMutex);
        // Cancel any ongoing search
        m_stopFlag = true;
        wxMilliSleep(50);  // allow cancellation to propagate
        
        ClearResults();
        m_progressBar->SetValue(0);
        m_progressText->SetLabel("Loading recently added apps...");
        m_progressPanel->Show();
        Layout();
        
        totalResults = 0;
        m_stopFlag = false;
        m_isInitialLoading = true;
        
        // Create and run the initial load thread
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
        wxMilliSleep(50);
        
        ClearResults();
        m_progressBar->SetValue(0);
        m_progressText->SetLabel(wxString::Format("Searching for '%s'...", query));
        m_progressPanel->Show();
        Layout();
        
        totalResults = 0;
        m_stopFlag = false;
        m_searchId++;
        m_isSearching = true;
        
        // Create and run the search thread
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
    
    m_progressBar->SetValue(100);
    m_progressText->SetLabel("Loading complete!");
    
    // Final layout update
    m_resultsPanel->Layout();
    m_resultsPanel->FitInside();
    m_resultsPanel->Refresh();
    
    // Hide progress panel after a short delay
    wxTimer* timer = new wxTimer(this);
    timer->StartOnce(1000);
    timer->Bind(wxEVT_TIMER, [this, timer](wxTimerEvent&) {
        m_progressPanel->Hide();
        Layout();
        delete timer;
    });
}

// Updated OnResultReady method to fix UI issues
void FlatpakStore::OnResultReady(wxCommandEvent& event) {
    if (event.GetInt() != m_searchId && event.GetInt() != -1) return;

    // Parse the data
    wxString resultData = event.GetString();
    wxStringTokenizer tokenizer(resultData, "|");
    
    if (tokenizer.CountTokens() < 4) return;
    
    wxString name = tokenizer.GetNextToken();
    wxString summary = tokenizer.GetNextToken();
    wxString iconUrl = tokenizer.GetNextToken();
    wxString appId = tokenizer.GetNextToken();
    
    // Freeze UI before adding elements to prevent flickering and showing partial UI
    m_resultsPanel->Freeze();
    
    // Create the card
    AppCard* card = new AppCard(m_resultsPanel, name, summary, appId);
    
    // Use fixed spacing and no absolute positioning to prevent arrows/stray text
    m_gridSizer->Add(card, 0, wxALL, 5); // Use fixed margin all around
    m_displayedResults++;
    
    // Explicitly update the layout every time to ensure clean display
    m_gridSizer->Layout();
    
    // Thaw UI to show changes
    m_resultsPanel->Thaw();
    
    // Update layout every few items for efficiency
    if (m_displayedResults % 5 == 0 || m_displayedResults <= 2) {
        m_resultsPanel->Layout();
        m_resultsPanel->FitInside();
        m_resultsPanel->Refresh();
    }

    // Queue icon download
    m_threadPool->Enqueue([this, card, iconUrl]() {
        std::vector<unsigned char> iconData;
        if (DownloadImage(iconUrl.ToStdString(), iconData, m_stopFlag)) {
            wxMemoryInputStream stream(iconData.data(), iconData.size());
            wxImage image;
            if (image.LoadFile(stream, wxBITMAP_TYPE_PNG)) {
                // Scale the image to fit
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
    
    // Update progress
    if (totalResults > 0) {
        int progressPercent = static_cast<int>((m_displayedResults * 100) / totalResults);
        m_progressBar->SetValue(progressPercent);
        m_progressText->SetLabel(wxString::Format("Loading applications: %d of %d", 
                                                m_displayedResults, totalResults));
    } else {
        m_progressBar->Pulse();
    }
}

void FlatpakStore::OnImageReady(wxCommandEvent& event) {
    m_resultsPanel->Freeze(); // Pause UI updates
    IconData* data = static_cast<IconData*>(event.GetClientData());
    if (data) {
        data->card->SetIcon(data->bitmap);
        delete data;
    }
    m_resultsPanel->Thaw(); // Resume UI updates
}

void FlatpakStore::OnUpdateProgress(wxCommandEvent& event) {
    // If totalResults is unknown, perform a simple pulse.
    if (totalResults <= 0) {
        static int pulseVal = 0;
        pulseVal = (pulseVal + 5) % 100;
        m_progressBar->SetValue(pulseVal);
        return;
    }
    
    int current = event.GetInt();
    int progressPercent = (current * 100) / totalResults;
    m_progressBar->SetValue(progressPercent);
    m_progressText->SetLabel(wxString::Format("Loading applications: %d of %d", current, totalResults));
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
    }
    else {
        wxString errorMsg = "Installation failed for " + appId + "\n\nErrors:\n";
        for (const auto& err : errors) {
            errorMsg += err + "\n";
        }
        wxMessageBox(errorMsg, "Installation Error", wxOK | wxICON_ERROR);
    }
}

void FlatpakStore::OnInstallButtonClicked(wxCommandEvent& event) {
    wxButton* button = dynamic_cast<wxButton*>(event.GetEventObject());
    if (!button)
        return;
    
    wxString* appIdPtr = static_cast<wxString*>(button->GetClientData());
    if (!appIdPtr)
        return;
    
    wxString appId = *appIdPtr;
    HandleInstallClick(appId);
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

// ThreadPool Implementation
ThreadPool::ThreadPool(size_t threads) : stop(false) {
    for (size_t i = 0; i < threads; ++i) {
        workers.emplace_back([this] {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(queueMutex);
                    condition.wait(lock, [this] { return stop || !tasks.empty(); });
                    if (stop && tasks.empty())
                        return;
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
    for (std::thread &worker : workers) {
        if (worker.joinable())
            worker.join();
    }
}

void ThreadPool::CancelAll() {
    std::unique_lock<std::mutex> lock(queueMutex);
    while (!tasks.empty())
        tasks.pop();
}
