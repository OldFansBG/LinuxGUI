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
wxDEFINE_EVENT(wxEVT_CATEGORY_SELECTED, wxCommandEvent);

// Event tables
BEGIN_EVENT_TABLE(FlatpakStore, wxPanel)
    EVT_BUTTON(wxID_ANY, FlatpakStore::OnSearch)
    EVT_TEXT_ENTER(wxID_ANY, FlatpakStore::OnSearch)
    EVT_COMMAND(wxID_ANY, wxEVT_SEARCH_COMPLETE, FlatpakStore::OnSearchComplete)
    EVT_COMMAND(wxID_ANY, wxEVT_RESULT_READY, FlatpakStore::OnResultReady)
    EVT_COMMAND(wxID_ANY, wxEVT_IMAGE_READY, FlatpakStore::OnImageReady)
    EVT_COMMAND(wxID_ANY, wxEVT_UPDATE_PROGRESS, FlatpakStore::OnUpdateProgress)
    EVT_COMMAND(wxID_ANY, wxEVT_CATEGORY_SELECTED, FlatpakStore::OnCategorySelected)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(AppCard, wxPanel)
    EVT_PAINT(AppCard::OnPaint)
    EVT_ENTER_WINDOW(AppCard::OnMouseEnter)
    EVT_LEAVE_WINDOW(AppCard::OnMouseLeave)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(CategoryChip, wxPanel)
    EVT_PAINT(CategoryChip::OnPaint)
    EVT_LEFT_DOWN(CategoryChip::OnMouseDown)
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

std::string searchFlathubApps(const std::string& query, const std::string& category, std::atomic<bool>& stopFlag) {
    CURL* curl = curl_easy_init();
    if (!curl) return "";

    std::string response;
    std::string url = "https://flathub.org/api/v2/search";
    std::string filters = category.empty() ? "[]" : "[{\"key\":\"category\",\"value\":\"" + category + "\"}]";
    std::string payload = "{\"query\":\"" + query + "\",\"filters\":" + filters + "}";

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

// CategoryChip implementation
CategoryChip::CategoryChip(wxWindow* parent, const wxString& label, int id)
    : wxPanel(parent, id, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE),
      m_selected(false)
{
    m_label = new wxStaticText(this, wxID_ANY, label);
    m_label->SetForegroundColour(wxColour(156, 163, 175));
    
    wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(m_label, 0, wxALL | wxALIGN_CENTER_VERTICAL, 8);
    SetSizer(sizer);
    
    SetBackgroundColour(wxColour(55, 65, 81));
    SetMinSize(wxSize(120, 34));
    SetBackgroundStyle(wxBG_STYLE_PAINT);
}

void CategoryChip::SetSelected(bool selected) {
    m_selected = selected;
    m_label->SetForegroundColour(selected ? *wxWHITE : wxColour(156, 163, 175));
    Refresh();
}

void CategoryChip::OnPaint(wxPaintEvent& event) {
    wxPaintDC dc(this);
    wxRect rect = GetClientRect();
    
    dc.SetBrush(wxBrush(m_selected ? wxColour(79, 70, 229) : wxColour(55, 65, 81)));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRoundedRectangle(rect, 16);
}

void CategoryChip::OnMouseDown(wxMouseEvent& event) {
    wxCommandEvent evt(wxEVT_CATEGORY_SELECTED, GetId());
    evt.SetEventObject(this);
    wxPostEvent(GetParent(), evt);
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

        std::string response = searchFlathubApps(m_query, m_category.ToStdString(), m_stopFlag);
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
            
            wxMilliSleep(50);
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

// FlatpakStore implementation
FlatpakStore::FlatpakStore(wxWindow* parent)
    : wxPanel(parent, wxID_ANY),
      m_isSearching(false), 
      totalResults(0),
      m_stopFlag(false), 
      m_searchId(0)
{
    m_threadPool = std::make_unique<ThreadPool>(4);
    SetBackgroundColour(wxColour(40, 44, 52));

    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

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

    wxPanel* categoriesPanel = new wxPanel(this, wxID_ANY);
    categoriesPanel->SetName("CategoriesPanel");
    categoriesPanel->SetBackgroundColour(wxColour(40, 44, 52));
    
    m_categoriesSizer = new wxBoxSizer(wxHORIZONTAL);
    categoriesPanel->SetSizer(m_categoriesSizer);

    m_progressBar = new wxGauge(this, wxID_ANY, 100, wxDefaultPosition, wxSize(-1, 6));
    m_progressBar->SetBackgroundColour(wxColour(40, 44, 52));
    m_progressBar->SetForegroundColour(wxColour(79, 70, 229));

    m_resultsPanel = new wxScrolledWindow(this, wxID_ANY);
    m_resultsPanel->SetBackgroundColour(wxColour(40, 44, 52));
    m_resultsPanel->SetScrollRate(0, 20);
    
    m_resultsSizer = new wxBoxSizer(wxVERTICAL);
    m_gridSizer = new wxGridSizer(2, 15, 15);
    m_resultsSizer->Add(m_gridSizer, 1, wxEXPAND | wxALL, 15);
    m_resultsPanel->SetSizer(m_resultsSizer);

    mainSizer->Add(searchPanel, 0, wxEXPAND | wxALL, 15);
    mainSizer->Add(categoriesPanel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 15);
    mainSizer->Add(m_progressBar, 0, wxEXPAND | wxLEFT | wxRIGHT, 15);
    mainSizer->Add(m_resultsPanel, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 15);
    SetSizer(mainSizer);

    InitializeCategories();

    std::ifstream file("container_id.txt");
    if (file.is_open()) {
        std::string containerId;
        std::getline(file, containerId);
        m_containerId = wxString(containerId);
        file.close();
    }
}

// ... [Rest of FlatpakStore methods] ...

void FlatpakStore::InitializeCategories() {
    const struct {
        wxString label;
        int id;
    } categories[] = {
        {"All", 1000},
        {"Games", 1001},
        {"Graphics", 1002},
        {"Internet", 1003},
        {"Office", 1004},
        {"Developer", 1005},
        {"Audio & Video", 1006},
        {"Education", 1007},
        {"Utilities", 1008}
    };

    wxWindow* categoriesPanel = nullptr;
    for (wxWindowList::iterator it = GetChildren().begin(); it != GetChildren().end(); ++it) {
        if ((*it)->GetName() == "CategoriesPanel") {
            categoriesPanel = *it;
            break;
        }
    }

    if (!categoriesPanel) {
        wxLogError("Categories panel not found!");
        return;
    }

    for (const auto& category : categories) {
        CategoryChip* chip = new CategoryChip(categoriesPanel, category.label, category.id);
        m_categoriesSizer->Add(chip, 0, wxRIGHT, 8);
        m_categories.push_back(chip);

        if (category.id == 1000) {
            chip->SetSelected(true);
            m_currentCategory = "";
        }
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
void FlatpakStore::SetContainerId(const wxString& containerId) {
    m_containerId = containerId;
}

void FlatpakStore::ClearResults() {
    if (m_gridSizer) {
        m_gridSizer->Clear(true);
    }
    m_resultsPanel->Layout();
}

void FlatpakStore::OnSearch(wxCommandEvent& event) {
    try {
        std::lock_guard<std::mutex> lock(m_searchMutex);

        wxString query = m_searchBox->GetValue();
        if (query.IsEmpty()) {
            wxMessageBox("Please enter a search term", "Search Error", wxOK | wxICON_INFORMATION);
            return;
        }

        // Cancel existing search
        m_stopFlag = true;
        wxMilliSleep(50);  // Allow cancellation to propagate

        // Reset search state
        ClearResults();
        m_progressBar->SetValue(0);
        totalResults = 0;
        m_stopFlag = false;
        m_searchId++;
        m_isSearching = true;

        // Create and run thread without storing the pointer
        SearchThread* thread = new SearchThread(this, query.ToStdString(), 
                                              m_currentCategory, m_searchId, m_stopFlag);
        if (thread->Run() != wxTHREAD_NO_ERROR) {
            delete thread;
            throw std::runtime_error("Failed to start search thread");
        }
    }
    catch (const std::exception& e) {
        m_isSearching = false;
        wxMessageBox(wxString::Format("Search failed: %s", e.what()),
                    "Error", wxOK | wxICON_ERROR);
    }
}

void FlatpakStore::OnSearchComplete(wxCommandEvent& event) {
    if (event.GetInt() != m_searchId) return;

    m_isSearching = false;
    m_progressBar->SetValue(100);

    m_resultsPanel->Layout();
    m_resultsPanel->FitInside();
    m_resultsPanel->Refresh();
}

void FlatpakStore::OnCategorySelected(wxCommandEvent& event) {
    int selectedId = event.GetId();
    
    for (CategoryChip* chip : m_categories) {
        bool isSelected = chip->GetId() == selectedId;
        chip->SetSelected(isSelected);
        
        if (isSelected) {
            wxString label = dynamic_cast<wxStaticText*>(chip->GetChildren().GetFirst()->GetData())->GetLabelText();
            if (label == "All") {
                m_currentCategory = "";
            } else {
                m_currentCategory = label;
            }
        }
    }
    
    if (!m_searchBox->GetValue().IsEmpty()) {
        wxCommandEvent searchEvent(wxEVT_COMMAND_BUTTON_CLICKED);
        OnSearch(searchEvent);
    }
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
    if (event.GetInt() != m_searchId) return;

    std::string resultData = event.GetString().ToStdString();
    std::istringstream iss(resultData);
    std::string name, summary, iconUrl, appId;

    std::getline(iss, name, '|');
    std::getline(iss, summary, '|');
    std::getline(iss, iconUrl, '|');
    std::getline(iss, appId, '|');

    AppCard* card = new AppCard(m_resultsPanel, name, summary, appId);
    m_gridSizer->Add(card, 0, wxEXPAND);
    
    m_threadPool->Enqueue([this, iconUrl, card, event]() {
        std::vector<unsigned char> iconData;
        if (DownloadImage(iconUrl, iconData, m_stopFlag)) {
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
                imageEvent.SetInt(event.GetInt());
                imageEvent.SetClientData(new wxBitmap(bitmap));
                imageEvent.SetEventObject(card);
                wxQueueEvent(this, imageEvent.Clone());
            }
        }
    });

    m_resultsPanel->Layout();
}

void FlatpakStore::OnImageReady(wxCommandEvent& event) {
    if (event.GetInt() != m_searchId) {
        wxBitmap* bitmap = static_cast<wxBitmap*>(event.GetClientData());
        delete bitmap;
        return;
    }

    wxBitmap* bitmap = static_cast<wxBitmap*>(event.GetClientData());
    AppCard* card = static_cast<AppCard*>(event.GetEventObject());

    if (bitmap && card) {
        card->SetIcon(*bitmap);
        delete bitmap;
    }
}

void FlatpakStore::OnUpdateProgress(wxCommandEvent& event) {
    if (event.GetInt() != m_searchId) {
        return;
    }

    int currentProgress = event.GetInt();
    if (totalResults > 0) {
        int progressValue = (currentProgress * 100) / totalResults;
        m_progressBar->SetValue(progressValue);
    }
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