#include "FlatpakStore.h"

// Define the event types
wxDEFINE_EVENT(wxEVT_SEARCH_COMPLETE, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_RESULT_READY, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_IMAGE_READY, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_UPDATE_PROGRESS, wxCommandEvent);

// Event table
wxBEGIN_EVENT_TABLE(FlatpakStore, wxPanel)
    EVT_BUTTON(wxID_ANY, FlatpakStore::OnSearch)
    EVT_TEXT_ENTER(wxID_ANY, FlatpakStore::OnSearch)
    EVT_COMMAND(wxID_ANY, wxEVT_SEARCH_COMPLETE, FlatpakStore::OnSearchComplete)
    EVT_COMMAND(wxID_ANY, wxEVT_RESULT_READY, FlatpakStore::OnResultReady)
    EVT_COMMAND(wxID_ANY, wxEVT_IMAGE_READY, FlatpakStore::OnImageReady)
    EVT_COMMAND(wxID_ANY, wxEVT_UPDATE_PROGRESS, FlatpakStore::OnUpdateProgress)
wxEND_EVENT_TABLE()

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

// Search functions
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

    return (res == CURLE_OK && !stopFlag);
}

std::string searchFlathubApps(const std::string& query, std::atomic<bool>& stopFlag) {
    std::cout << "Searching Flathub for: " << query << std::endl;

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
        std::cerr << "Search failed: " << curl_easy_strerror(res) << std::endl;
        return "";
    }

    return response;
}

// Search thread class
class SearchThread : public wxThread {
public:
    SearchThread(FlatpakStore* store, const std::string& query, int searchId, std::atomic<bool>& stopFlag)
        : wxThread(wxTHREAD_DETACHED), m_store(store), m_query(query),
          m_searchId(searchId), m_stopFlag(stopFlag) {}

protected:
    virtual ExitCode Entry() {
        try {
            if (TestDestroy()) return (ExitCode)0;

            std::cout << "Starting search thread execution..." << std::endl;
            std::string response = searchFlathubApps(m_query, m_stopFlag);

            if (response.empty()) {
                std::cerr << "Empty response from API" << std::endl;
                return (ExitCode)1;
            }

            rapidjson::Document doc;
            if (doc.Parse(response.c_str()).HasParseError()) {
                return (ExitCode)1;
            }

            if (!doc.IsObject() || !doc.HasMember("hits") || !doc["hits"].IsArray()) {
                return (ExitCode)1;
            }

            const rapidjson::Value& hits = doc["hits"];
            m_store->SetTotalResults(hits.Size());

            for (rapidjson::SizeType i = 0; i < hits.Size() && !m_stopFlag && !TestDestroy(); i++) {
                const rapidjson::Value& hit = hits[i];
                if (!hit.IsObject() || !hit.HasMember("name") ||
                    !hit.HasMember("summary") || !hit.HasMember("icon")) {
                    continue;
                }

                std::string name = hit["name"].GetString();
                std::string summary = hit["summary"].GetString();
                std::string iconUrl = hit["icon"].GetString();

                wxCommandEvent resultEvent(wxEVT_RESULT_READY);
                resultEvent.SetInt(m_searchId);
                resultEvent.SetString(name + "|" + summary + "|" + iconUrl);
                wxQueueEvent(m_store, resultEvent.Clone());

                wxCommandEvent progressEvent(wxEVT_UPDATE_PROGRESS);
                progressEvent.SetInt(i + 1);
                wxQueueEvent(m_store, progressEvent.Clone());
            }

            if (!m_stopFlag && !TestDestroy()) {
                wxCommandEvent event(wxEVT_SEARCH_COMPLETE);
                event.SetInt(m_searchId);
                wxQueueEvent(m_store, event.Clone());
            }

            return (ExitCode)0;
        }
        catch (const std::exception& e) {
            std::cerr << "Search thread error: " << e.what() << std::endl;
            return (ExitCode)1;
        }
    }

private:
    FlatpakStore* m_store;
    std::string m_query;
    int m_searchId;
    std::atomic<bool>& m_stopFlag;
};

// FlatpakStore implementation
FlatpakStore::FlatpakStore(wxWindow* parent)
    : wxPanel(parent, wxID_ANY),
      isSearching(false), totalResults(0), currentSearchThread(nullptr),
      stopFlag(false), searchId(0)
{
    // Initialize the thread pool with 4 worker threads
    threadPool = std::make_unique<ThreadPool>(4);

    SetBackgroundColour(wxColour(31, 41, 55));

    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Search controls
    wxBoxSizer* searchControlsSizer = new wxBoxSizer(wxHORIZONTAL);

    searchBox = new wxTextCtrl(this, wxID_ANY, "",
                              wxDefaultPosition, wxDefaultSize,
                              wxTE_PROCESS_ENTER);
    searchBox->SetBackgroundColour(wxColour(55, 65, 81));
    searchBox->SetForegroundColour(*wxWHITE);
    searchBox->SetHint("Search Flathub applications...");
    searchBox->Bind(wxEVT_TEXT_ENTER, &FlatpakStore::OnSearch, this);

    searchButton = new wxButton(this, wxID_ANY, "Search");
    searchButton->SetBackgroundColour(wxColour(37, 99, 235));
    searchButton->SetForegroundColour(*wxWHITE);
    searchButton->Bind(wxEVT_BUTTON, &FlatpakStore::OnSearch, this);

    searchControlsSizer->Add(searchBox, 1, wxEXPAND | wxRIGHT, 5);
    searchControlsSizer->Add(searchButton, 0);

    // Progress bar
    progressBar = new wxGauge(this, wxID_ANY, 100);
    progressBar->SetValue(0);

    // Results panel
    resultsPanel = new wxScrolledWindow(this, wxID_ANY);
    resultsPanel->SetBackgroundColour(wxColour(31, 41, 55));
    resultsSizer = new wxBoxSizer(wxVERTICAL);
    resultsPanel->SetSizer(resultsSizer);
    resultsPanel->EnableScrolling(false, true);
    resultsPanel->ShowScrollbars(wxSHOW_SB_NEVER, wxSHOW_SB_ALWAYS);

    mainSizer->Add(searchControlsSizer, 0, wxEXPAND | wxALL, 10);
    mainSizer->Add(progressBar, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    mainSizer->Add(resultsPanel, 1, wxEXPAND);

    SetSizer(mainSizer);
}

FlatpakStore::~FlatpakStore() {
    try {
        std::lock_guard<std::mutex> lock(searchMutex);
        stopFlag = true;

        if (currentSearchThread) {
            if (currentSearchThread->IsRunning()) {
                currentSearchThread->Delete();
            }
            delete currentSearchThread;
            currentSearchThread = nullptr;
        }

        if (threadPool) {
            threadPool->CancelAll();
            threadPool.reset();
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error during cleanup: " << e.what() << std::endl;
    }
}

void FlatpakStore::OnSearch(wxCommandEvent& event) {
    try {
        std::cout << "Search button clicked!" << std::endl;
        std::lock_guard<std::mutex> lock(searchMutex);

        wxString query = searchBox->GetValue();
        std::cout << "Search query: " << query << std::endl;

        if (query.IsEmpty()) {
            wxMessageBox("Please enter a search term", "Search Error",
                        wxOK | wxICON_INFORMATION);
            return;
        }

        if (isSearching && currentSearchThread) {
            std::cout << "Cancelling previous search..." << std::endl;
            stopFlag = true;

            if (currentSearchThread->IsRunning()) {
                currentSearchThread->Delete();
            }

            delete currentSearchThread;
            currentSearchThread = nullptr;
            isSearching = false;

            if (threadPool) {
                threadPool->CancelAll();
            }
        }

        if (resultsSizer) {
            resultsSizer->Clear(true);
        }
        if (resultsPanel) {
            resultsPanel->SetSizer(resultsSizer);
            resultsPanel->Layout();
        }

        progressBar->SetValue(0);
        totalResults = 0;
        stopFlag = false;
        searchId++;
        isSearching = true;

        currentSearchThread = new SearchThread(this, query.ToStdString(), searchId, stopFlag);
        if (!currentSearchThread || currentSearchThread->Run() != wxTHREAD_NO_ERROR) {
            throw std::runtime_error("Failed to start search thread");
        }

        std::cout << "Search thread started successfully" << std::endl;
    }
    catch (const std::exception& e) {
        isSearching = false;
        if (currentSearchThread) {
            delete currentSearchThread;
            currentSearchThread = nullptr;
        }
        wxMessageBox(wxString::Format("Search failed: %s", e.what()),
                    "Error", wxOK | wxICON_ERROR);
        std::cerr << "Search error: " << e.what() << std::endl;
    }
}

void FlatpakStore::OnSearchComplete(wxCommandEvent& event) {
    if (event.GetInt() != searchId) return;

    isSearching = false;
    progressBar->SetValue(100);

    resultsPanel->Layout();
    resultsPanel->FitInside();
    resultsPanel->SetScrollRate(0, 20);
    resultsPanel->Refresh();
}

void FlatpakStore::OnResultReady(wxCommandEvent& event) {
    if (event.GetInt() != searchId) return;

    std::string resultData = event.GetString().ToStdString();
    std::istringstream iss(resultData);
    std::string name, summary, iconUrl;

    std::getline(iss, name, '|');
    std::getline(iss, summary, '|');
    std::getline(iss, iconUrl, '|');

    // Create result panel
    wxPanel* resultPanel = new wxPanel(resultsPanel);
    resultPanel->SetBackgroundColour(wxColour(55, 65, 81));

    wxBoxSizer* resultSizer = new wxBoxSizer(wxHORIZONTAL);

    // Add app icon
    wxBitmap placeholderBitmap(32, 32);
    wxStaticBitmap* iconBitmap = new wxStaticBitmap(resultPanel, wxID_ANY, placeholderBitmap);
    resultSizer->Add(iconBitmap, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

    // Add text content
    wxBoxSizer* textSizer = new wxBoxSizer(wxVERTICAL);

    // App name
    wxStaticText* nameText = new wxStaticText(resultPanel, wxID_ANY, name);
    nameText->SetForegroundColour(*wxWHITE);
    wxFont nameFont = nameText->GetFont();
    nameFont.SetWeight(wxFONTWEIGHT_BOLD);
    nameText->SetFont(nameFont);

    // App summary
    wxStaticText* summaryText = new wxStaticText(resultPanel, wxID_ANY, summary);
    summaryText->SetForegroundColour(wxColour(156, 163, 175));
    summaryText->Wrap(400);

    textSizer->Add(nameText, 0, wxEXPAND);
    textSizer->Add(summaryText, 0, wxEXPAND | wxTOP, 2);

    resultSizer->Add(textSizer, 1, wxEXPAND | wxALL, 5);

    // Add install button
    wxButton* installButton = new wxButton(resultPanel, wxID_ANY, "Install");
    installButton->SetBackgroundColour(wxColour(37, 99, 235));
    installButton->SetForegroundColour(*wxWHITE);
    resultSizer->Add(installButton, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

    resultPanel->SetSizer(resultSizer);
    resultsSizer->Add(resultPanel, 0, wxEXPAND | wxALL, 5);

    // Download the app icon using the thread pool
    threadPool->Enqueue([this, iconUrl, iconBitmap, event]() {
        std::vector<unsigned char> iconData;
        if (DownloadImage(iconUrl, iconData, stopFlag)) {
            wxMemoryInputStream stream(iconData.data(), iconData.size());
            wxImage image;
            if (image.LoadFile(stream, wxBITMAP_TYPE_PNG)) {
                image.Rescale(32, 32);
                wxBitmap bitmap(image);

                wxCommandEvent imageEvent(wxEVT_IMAGE_READY);
                imageEvent.SetInt(event.GetInt());
                imageEvent.SetClientData(new wxBitmap(bitmap));
                imageEvent.SetEventObject(iconBitmap);
                wxQueueEvent(this, imageEvent.Clone());
            }
        }
    });

    resultsPanel->Layout();
    resultsPanel->FitInside();
}

void FlatpakStore::OnImageReady(wxCommandEvent& event) {
    if (event.GetInt() != searchId) {
        wxBitmap* bitmap = static_cast<wxBitmap*>(event.GetClientData());
        delete bitmap;
        return;
    }

    wxBitmap* bitmap = static_cast<wxBitmap*>(event.GetClientData());
    wxStaticBitmap* iconBitmap = static_cast<wxStaticBitmap*>(event.GetEventObject());

    if (bitmap && iconBitmap) {
        iconBitmap->SetBitmap(*bitmap);
        delete bitmap;
    }
}

void FlatpakStore::OnUpdateProgress(wxCommandEvent& event) {
    if (event.GetInt() != searchId) {
        return;
    }

    int currentProgress = event.GetInt();
    if (totalResults > 0) {
        int progressValue = (currentProgress * 100) / totalResults;
        progressBar->SetValue(progressValue);
    }
}