#include "FlatpakStore.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <wx/tokenzr.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/log.h>
#include <iostream>

// Define event types
wxDEFINE_EVENT(wxEVT_SEARCH_COMPLETE, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_RESULT_READY, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_IMAGE_READY, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_UPDATE_PROGRESS, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_BATCH_PROCESS, wxCommandEvent);
wxDEFINE_EVENT(INSTALL_START_EVENT, wxCommandEvent);
wxDEFINE_EVENT(INSTALL_COMPLETE_EVENT, wxCommandEvent);
wxDEFINE_EVENT(INSTALL_CANCEL_EVENT, wxCommandEvent);

// Event tables
BEGIN_EVENT_TABLE(FlatpakStore, wxPanel)
    EVT_BUTTON(wxID_ANY, FlatpakStore::OnInstallButtonClicked)
    EVT_TEXT_ENTER(wxID_ANY, FlatpakStore::OnSearch)
    EVT_COMMAND(wxID_ANY, wxEVT_SEARCH_COMPLETE, FlatpakStore::OnSearchComplete)
    EVT_COMMAND(wxID_ANY, wxEVT_RESULT_READY, FlatpakStore::OnResultReady)
    EVT_COMMAND(wxID_ANY, wxEVT_IMAGE_READY, FlatpakStore::OnImageReady)
    EVT_COMMAND(wxID_ANY, wxEVT_UPDATE_PROGRESS, FlatpakStore::OnUpdateProgress)
    EVT_TIMER(ID_BATCH_TIMER, FlatpakStore::OnBatchTimer)
    EVT_TIMER(ID_LAYOUT_TIMER, FlatpakStore::OnLayoutTimer)
    EVT_COMMAND(wxID_ANY, INSTALL_START_EVENT, FlatpakStore::OnInstallStart)
    EVT_COMMAND(wxID_ANY, INSTALL_COMPLETE_EVENT, FlatpakStore::OnInstallComplete)
    EVT_COMMAND(wxID_ANY, INSTALL_CANCEL_EVENT, FlatpakStore::OnInstallCancel)
    EVT_END_PROCESS(wxID_ANY, FlatpakStore::OnProcessTerminated)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(AppCard, wxPanel)
    EVT_PAINT(AppCard::OnPaint)
    EVT_ENTER_WINDOW(AppCard::OnMouseEnter)
    EVT_LEAVE_WINDOW(AppCard::OnMouseLeave)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(RoundedSearchPanel, wxPanel)
    EVT_PAINT(RoundedSearchPanel::OnPaint)
END_EVENT_TABLE()

// Helper Functions (unchanged)
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

// RoundedSearchPanel Implementation (unchanged)
RoundedSearchPanel::RoundedSearchPanel(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size)
    : wxPanel(parent, id, pos, size, wxBORDER_NONE)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(parent->GetBackgroundColour());
}

void RoundedSearchPanel::OnPaint(wxPaintEvent& event)
{
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(wxBrush(GetBackgroundColour()));
    dc.Clear();
    
    wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
    if (gc) {
        wxRect rect = GetClientRect();
        gc->SetBrush(wxBrush(wxColour(55, 65, 81)));
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);
        gc->DrawRoundedRectangle(rect.x, rect.y, rect.width, rect.height, 10);
        delete gc;
    }
}

// AppCard Implementation
void AppCard::SetInstallButtonLabel(const wxString& label) {
    m_installButton->SetLabel(label);
}

void AppCard::DisableInstallButton() {
    m_installButton->Disable();
}

AppCard::AppCard(wxWindow* parent, const wxString& name, const wxString& summary, const wxString& appId)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(320, 160), wxBORDER_NONE),
      m_appId(appId), m_isLoading(true)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(parent->GetBackgroundColour());

    wxBoxSizer* mainSizer = new wxBoxSizer(wxHORIZONTAL);
    mainSizer->AddSpacer(12);

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

    m_nameText = new wxStaticText(this, wxID_ANY, name.IsEmpty() ? "Loading app..." : name);
    m_nameText->SetForegroundColour(wxColour(229, 229, 229));
    wxFont nameFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
    m_nameText->SetFont(nameFont);

    m_summaryText = new wxStaticText(this, wxID_ANY, summary.IsEmpty() ? "Loading description..." : summary);
    m_summaryText->SetForegroundColour(wxColour(156, 163, 175));
    m_summaryText->Wrap(220);

    textSizer->Add(m_nameText, 0, wxBOTTOM, 6);
    textSizer->Add(m_summaryText, 1, wxEXPAND);

    wxBoxSizer* rightSizer = new wxBoxSizer(wxVERTICAL);
    rightSizer->Add(textSizer, 1, wxEXPAND);

    m_controlSizer = new wxBoxSizer(wxHORIZONTAL);
    m_installButton = new wxButton(this, wxID_ANY, "Install", wxDefaultPosition, wxSize(90, 34));
    m_installButton->SetBackgroundColour(wxColour(79, 70, 229));
    m_installButton->SetForegroundColour(*wxWHITE);
    m_installButton->SetClientData(new wxString(appId));

    m_progressGauge = new wxGauge(this, wxID_ANY, 100, wxDefaultPosition, wxSize(60, 20));
    m_progressGauge->Hide();

    m_cancelButton = new wxButton(this, wxID_ANY, "Cancel", wxDefaultPosition, wxSize(30, 20));
    m_cancelButton->Hide();

    m_controlSizer->Add(m_installButton, 0, wxALIGN_CENTER);
    m_controlSizer->Add(m_progressGauge, 0, wxALIGN_CENTER | wxLEFT, 5);
    m_controlSizer->Add(m_cancelButton, 0, wxALIGN_CENTER | wxLEFT, 5);

    rightSizer->Add(m_controlSizer, 0, wxALIGN_RIGHT | wxBOTTOM | wxRIGHT, 12);
    mainSizer->Add(rightSizer, 1, wxEXPAND | wxLEFT, 12);
    SetSizer(mainSizer);
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

void AppCard::ShowInstalling(bool show) {
    if (show) {
        m_controlSizer->Hide(m_installButton);
        m_controlSizer->Show(m_progressGauge);
        m_controlSizer->Show(m_cancelButton);
        m_progressGauge->Pulse();
    } else {
        m_controlSizer->Show(m_installButton);
        m_controlSizer->Hide(m_progressGauge);
        m_controlSizer->Hide(m_cancelButton);
    }
    Layout();
}

void AppCard::OnPaint(wxPaintEvent& event) {
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(wxBrush(GetParent()->GetBackgroundColour()));
    dc.Clear();
    
    wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
    if (gc) {
        wxRect rect = GetClientRect();
        wxColour bgColor = GetBackgroundColour();
        gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);
        gc->SetBrush(wxBrush(bgColor));
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->DrawRoundedRectangle(0, 0, rect.width, rect.height, 10);
        delete gc;
    }
}

void AppCard::OnMouseEnter(wxMouseEvent& event) {
    SetBackgroundColour(wxColour(65, 75, 91));
    Refresh();
}

void AppCard::OnMouseLeave(wxMouseEvent& event) {
    SetBackgroundColour(wxColour(55, 65, 81));
    Refresh();
}

// SearchThread Implementation (unchanged)
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

// InitialLoadThread Implementation (unchanged)
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
            resultEvent.SetInt(-1);
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

// InstallThread Implementation
wxThread::ExitCode InstallThread::Entry() {
    wxString command = wxString::Format(
        "docker exec %s /bin/bash -c \"chroot /root/custom_iso/squashfs-root /bin/bash -c 'flatpak install -y flathub %s'\"",
        m_store->GetContainerId(), m_appId
    );

    // Start the process asynchronously in the main thread via a command event
    wxCommandEvent startEvent(INSTALL_START_EVENT);
    startEvent.SetString(command);
    startEvent.SetClientData(m_state);
    wxQueueEvent(m_store, startEvent.Clone());

    // Poll for cancellation or process completion
    while (!m_state->cancelFlag && !TestDestroy()) {
        wxMilliSleep(100);  // Check every 100ms
        if (m_state->process && !m_state->process->Exists(m_state->pid)) {
            break;  // Process has ended naturally
        }
    }

    if (m_state->cancelFlag && m_state->process && m_state->process->Exists(m_state->pid)) {
        wxKill(m_state->pid, wxSIGTERM);  // Try to terminate gracefully
        wxMilliSleep(500);
        if (m_state->process->Exists(m_state->pid)) {
            wxKill(m_state->pid, wxSIGKILL);  // Force kill if necessary
        }
        wxCommandEvent cancelEvent(INSTALL_CANCEL_EVENT);
        cancelEvent.SetInt(1);  // Indicate cancellation
        cancelEvent.SetClientData(m_state);
        wxQueueEvent(m_store, cancelEvent.Clone());
    }

    return (ExitCode)0;
}

// FlatpakStore Implementation
FlatpakStore::FlatpakStore(wxWindow* parent, const wxString& workDir)
    : wxPanel(parent, wxID_ANY),
      m_workDir(workDir),
      m_isSearching(false),
      m_isInitialLoading(false),
      totalResults(0),
      m_stopFlag(false),
      m_searchId(0),
      m_displayedResults(0),
      m_pendingBatchSize(0)
{
    SetDoubleBuffered(true);

    m_threadPool = std::make_unique<ThreadPool>(4);
    
    m_batchTimer = new wxTimer(this, ID_BATCH_TIMER);
    m_layoutTimer = new wxTimer(this, ID_LAYOUT_TIMER);

    SetBackgroundColour(wxColour(40, 44, 52));

    m_mainSizer = new wxBoxSizer(wxVERTICAL);

    RoundedSearchPanel* searchPanel = new RoundedSearchPanel(this, wxID_ANY);
    wxBoxSizer* searchSizer = new wxBoxSizer(wxHORIZONTAL);

    m_searchBox = new wxTextCtrl(searchPanel, wxID_ANY, "", wxDefaultPosition,
                                 wxSize(-1, 42), wxTE_PROCESS_ENTER | wxBORDER_NONE);
    m_searchBox->SetBackgroundColour(wxColour(55, 65, 81));
    m_searchBox->SetForegroundColour(wxColour(229, 229, 229));
    m_searchBox->SetHint("Search for applications...");
    
    wxFont searchFont = m_searchBox->GetFont();
    searchFont.SetPointSize(searchFont.GetPointSize() + 1);
    m_searchBox->SetFont(searchFont);

    m_searchButton = new wxButton(searchPanel, wxID_ANY, "Search",
                                  wxDefaultPosition, wxSize(120, 42), wxBORDER_NONE);
    m_searchButton->SetBackgroundColour(wxColour(79, 70, 229));
    m_searchButton->SetForegroundColour(*wxWHITE);
    
    wxFont buttonFont = m_searchButton->GetFont();
    buttonFont.SetWeight(wxFONTWEIGHT_BOLD);
    m_searchButton->SetFont(buttonFont);

    searchSizer->Add(m_searchBox, 1, wxEXPAND | wxALL, 10);
    searchSizer->Add(m_searchButton, 0, wxALL, 10);
    searchPanel->SetSizer(searchSizer);

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
    m_progressPanel->Hide();

    m_resultsPanel = new wxScrolledWindow(this, wxID_ANY);
    m_resultsPanel->SetDoubleBuffered(true);
    m_resultsPanel->SetBackgroundColour(wxColour(40, 44, 52));
    m_resultsPanel->SetScrollRate(0, 20);

    wxBoxSizer* resultsSizer = new wxBoxSizer(wxVERTICAL);
    m_gridSizer = new wxWrapSizer(wxHORIZONTAL, wxWRAPSIZER_DEFAULT_FLAGS);
    resultsSizer->Add(m_gridSizer, 1, wxEXPAND | wxALL, 15);
    m_resultsPanel->SetSizer(resultsSizer);

    m_mainSizer->Add(searchPanel, 0, wxEXPAND | wxALL, 15);
    m_mainSizer->Add(m_progressPanel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 15);
    m_mainSizer->Add(m_resultsPanel, 1, wxEXPAND);

    SetSizer(m_mainSizer);

    LoadInitialApps();

    wxString containerPath = m_workDir + wxFILE_SEP_PATH + "container_id.txt";
    wxLogDebug("Checking for container_id.txt in: %s", containerPath);
    std::ifstream file(containerPath.ToStdString());
    if (file.is_open()) {
        std::string containerId;
        std::getline(file, containerId);
        if (!containerId.empty()) {
            m_containerId = wxString(containerId);
            wxLogDebug("Loaded container ID: %s from %s", m_containerId, containerPath);
        } else {
            wxLogDebug("container_id.txt is empty at: %s", containerPath);
        }
        file.close();
    } else {
        wxLogDebug("container_id.txt not found in workDir: %s", containerPath);
    }
}

void FlatpakStore::SetContainerId(const wxString& containerId) {
    m_containerId = containerId;
}

wxString FlatpakStore::GetContainerId() const {
    return m_containerId;
}

void FlatpakStore::ClearResults() {
    m_resultsPanel->Freeze();
    m_gridSizer->Clear(true);
    m_displayedResults = 0;
    m_resultsPanel->Scroll(0, 0);
    m_resultsPanel->Layout();
    m_resultsPanel->Refresh();
    m_resultsPanel->Thaw();
    
    std::lock_guard<std::mutex> lock(m_resultsMutex);
    m_pendingResults.clear();
    m_pendingBatchSize = 0;
}

void FlatpakStore::LoadInitialApps() {
    try {
        std::lock_guard<std::mutex> lock(m_searchMutex);
        m_stopFlag = true;
        wxMilliSleep(50);
        
        ClearResults();
        m_progressBar->SetValue(0);
        m_progressText->SetLabel("Loading recently added apps...");
        m_progressPanel->Show();
        Layout();
        
        totalResults = 0;
        m_stopFlag = false;
        m_isInitialLoading = true;
        
        m_layoutTimer->Start(100);
        
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
        
        m_layoutTimer->Start(100);
        
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
    
    ProcessPendingBatch(true);
    
    wxTimer* timer = new wxTimer(this);
    timer->StartOnce(1000);
    timer->Bind(wxEVT_TIMER, [this, timer](wxTimerEvent&) {
        m_progressPanel->Hide();
        Layout();
        delete timer;
    });
}

void FlatpakStore::OnResultReady(wxCommandEvent& event) {
    if (event.GetInt() != m_searchId && event.GetInt() != -1) return;

    wxString resultData = event.GetString();
    
    std::lock_guard<std::mutex> lock(m_resultsMutex);
    m_pendingResults.push_back(resultData);
    m_pendingBatchSize++;
    
    if (!m_batchTimer->IsRunning() && m_pendingBatchSize >= 1) {
        m_batchTimer->StartOnce(10);
    }
    
    m_displayedResults++;
    
    if (totalResults > 0) {
        int progressPercent = static_cast<int>((m_displayedResults * 100) / totalResults);
        m_progressBar->SetValue(progressPercent);
        m_progressText->SetLabel(wxString::Format("Loading applications: %d of %d", 
                                                 m_displayedResults, totalResults));
    } else {
        m_progressBar->Pulse();
    }
}

void FlatpakStore::OnBatchTimer(wxTimerEvent& event) {
    ProcessPendingBatch(false);
}

void FlatpakStore::ProcessPendingBatch(bool processAll) {
    std::vector<wxString> currentBatch;
    {
        std::lock_guard<std::mutex> lock(m_resultsMutex);
        
        int batchSize = processAll ? m_pendingResults.size() : std::min(10, static_cast<int>(m_pendingResults.size()));
        if (batchSize <= 0) return;
        
        currentBatch.assign(m_pendingResults.begin(), m_pendingResults.begin() + batchSize);
        m_pendingResults.erase(m_pendingResults.begin(), m_pendingResults.begin() + batchSize);
        m_pendingBatchSize = static_cast<int>(m_pendingResults.size());
    }
    
    if (currentBatch.empty()) return;
    
    m_resultsPanel->Freeze();
    
    for (const wxString& resultData : currentBatch) {
        wxStringTokenizer tokenizer(resultData, "|");
        if (tokenizer.CountTokens() < 4) continue;
        wxString name = tokenizer.GetNextToken();
        wxString summary = tokenizer.GetNextToken();
        wxString iconUrl = tokenizer.GetNextToken();
        wxString appId = tokenizer.GetNextToken();
        
        AppCard* card = new AppCard(m_resultsPanel, name, summary, appId);
        m_gridSizer->Add(card, 0, wxALL, 5);
        
        m_threadPool->Enqueue([this, card, iconUrl]() {
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
                        padded.Paste(image, (target_size - image.GetWidth())/2,
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

    m_gridSizer->Layout();
    m_resultsPanel->Layout();
    m_layoutTimer->StartOnce(100);
    
    m_resultsPanel->Thaw();
    
    if (!m_pendingResults.empty() && !m_batchTimer->IsRunning()) {
        int delay = (currentBatch.size() == 1) ? 100 : 50;
        m_batchTimer->StartOnce(delay);
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

void FlatpakStore::OnLayoutTimer(wxTimerEvent& event) {
    m_resultsPanel->Layout();
    m_resultsPanel->FitInside();
    m_resultsPanel->Refresh();
}

void FlatpakStore::OnInstallButtonClicked(wxCommandEvent& event) {
    wxButton* button = dynamic_cast<wxButton*>(event.GetEventObject());
    if (!button) return;

    AppCard* card = dynamic_cast<AppCard*>(button->GetParent());
    if (!card) return;

    wxString* appIdPtr = static_cast<wxString*>(button->GetClientData());
    if (!appIdPtr) return;

    wxString appId = *appIdPtr;

    // Create installation state
    InstallState* state = new InstallState(card);

    // Update UI and bind cancel button
    card->ShowInstalling(true);
    auto cancelHandler = [state](wxCommandEvent&) { state->cancelFlag = true; };
    card->GetCancelButton()->Bind(wxEVT_BUTTON, cancelHandler);

    // Start installation thread
    InstallThread* thread = new InstallThread(this, appId, state);
    if (thread->Run() != wxTHREAD_NO_ERROR) {
        delete thread;
        card->GetCancelButton()->Unbind(wxEVT_BUTTON, cancelHandler);
        delete state;
        card->ShowInstalling(false);
        wxMessageBox("Failed to start installation thread", "Error", wxICON_ERROR);
        return;
    }
}

void FlatpakStore::OnInstallStart(wxCommandEvent& event) {
    InstallState* state = static_cast<InstallState*>(event.GetClientData());
    if (!state || !state->card) return;

    state->process = new wxProcess(this);
    state->process->Redirect();
    state->pid = wxExecute(event.GetString(), wxEXEC_ASYNC | wxEXEC_HIDE_CONSOLE, state->process);
    if (state->pid == 0) {
        delete state->process;
        state->process = nullptr;
        wxCommandEvent completeEvent(INSTALL_COMPLETE_EVENT);
        completeEvent.SetInt(-1);  // Indicate failure to start
        completeEvent.SetClientData(state);
        wxQueueEvent(this, completeEvent.Clone());
    }
}

void FlatpakStore::OnProcessTerminated(wxProcessEvent& event) {
    // Find the InstallState associated with this process
    for (wxWindow* child : m_resultsPanel->GetChildren()) {
        AppCard* card = dynamic_cast<AppCard*>(child);
        if (!card) continue;

        InstallState* state = nullptr;
        // Check if this card has an associated InstallState (assuming stored as client data, adjust if stored differently)
        // For simplicity, we assume the last state is the one we want; ideally, use a map
        state = static_cast<InstallState*>(card->GetClientData());
        if (state && state->process && state->process->GetPid() == event.GetPid()) {
            int exitCode = event.GetExitCode();
            wxCommandEvent completeEvent(INSTALL_COMPLETE_EVENT);
            completeEvent.SetInt(exitCode);
            completeEvent.SetClientData(state);
            wxQueueEvent(this, completeEvent.Clone());
            return;
        }
    }
}

void FlatpakStore::OnInstallComplete(wxCommandEvent& event) {
    InstallState* state = static_cast<InstallState*>(event.GetClientData());
    if (!state || !state->card) return;

    int result = event.GetInt();
    AppCard* card = state->card;

    card->ShowInstalling(false);
    auto cancelHandler = [state](wxCommandEvent&) { state->cancelFlag = true; };
    card->GetCancelButton()->Unbind(wxEVT_BUTTON, cancelHandler);

    if (result == 0) {
        card->SetInstallButtonLabel("Installed");
        card->DisableInstallButton();
    } else if (result != -1) {  // -1 means failed to start
        wxMessageBox("Installation failed for " + card->GetAppId(), "Error", wxICON_ERROR);
    }

    delete state->process;
    delete state;
}

void FlatpakStore::OnInstallCancel(wxCommandEvent& event) {
    InstallState* state = static_cast<InstallState*>(event.GetClientData());
    if (!state || !state->card) return;

    AppCard* card = state->card;
    card->ShowInstalling(false);
    auto cancelHandler = [state](wxCommandEvent&) { state->cancelFlag = true; };
    card->GetCancelButton()->Unbind(wxEVT_BUTTON, cancelHandler);

    wxMessageBox("Installation of " + card->GetAppId() + " was canceled.", "Canceled", wxICON_INFORMATION);

    delete state->process;
    delete state;
}

FlatpakStore::~FlatpakStore() {
    try {
        std::lock_guard<std::mutex> lock(m_searchMutex);
        m_stopFlag = true;
        if (m_threadPool) {
            m_threadPool->CancelAll();
            m_threadPool.reset();
        }
        
        if (m_batchTimer) {
            m_batchTimer->Stop();
            delete m_batchTimer;
        }
        
        if (m_layoutTimer) {
            m_layoutTimer->Stop();
            delete m_layoutTimer;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error during cleanup: " << e.what() << std::endl;
    }
}

// ThreadPool Implementation (unchanged)
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
    std::unique_lock<std::mutex> lock(queueMutex);
    stop = true;
    condition.notify_all();
    for (std::thread& worker : workers) {
        if (worker.joinable())
            worker.join();
    }
}

void ThreadPool::CancelAll() {
    std::unique_lock<std::mutex> lock(queueMutex);
    while (!tasks.empty())
        tasks.pop();
}