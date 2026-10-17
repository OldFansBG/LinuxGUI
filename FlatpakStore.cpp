#include "FlatpakStore.h"         // Декларации на класовете в магазина
#include "ContainerCommand.h"     // flatpak install / flatpak list в chroot средата на контейнера
#include <wx/mstream.h>           // wxMemoryInputStream за зареждане на иконите от паметта
#include <wx/weakref.h>           // wxWeakRef за картите в асинхронните callback-и
#include <wx/log.h>               // wxLogDebug
#include <curl/curl.h>            // HTTP заявки към Flathub
#include <rapidjson/document.h>   // Парсване на отговорите от Flathub
#include <rapidjson/writer.h>     // Създаване на JSON тялото на заявката за търсене
#include <rapidjson/stringbuffer.h>
#include <algorithm>              // std::min
#include <cstring>                // memset

#ifndef LINUXISOPRO_VERSION
#define LINUXISOPRO_VERSION "dev" // Задава се от CMakeLists.txt
#endif

wxDEFINE_EVENT(wxEVT_SEARCH_COMPLETE, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_RESULT_READY, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_IMAGE_READY, wxCommandEvent);

wxBEGIN_EVENT_TABLE(FlatpakStore, wxPanel)
    EVT_BUTTON(wxID_ANY, FlatpakStore::OnInstallButtonClicked) // Бутоните Install на картите
    EVT_TEXT_ENTER(wxID_ANY, FlatpakStore::OnSearch)           // Enter в полето за търсене
    EVT_COMMAND(wxID_ANY, wxEVT_SEARCH_COMPLETE, FlatpakStore::OnSearchComplete)
    EVT_COMMAND(wxID_ANY, wxEVT_RESULT_READY, FlatpakStore::OnResultReady)
    EVT_COMMAND(wxID_ANY, wxEVT_IMAGE_READY, FlatpakStore::OnImageReady)
    EVT_TIMER(ID_BATCH_TIMER, FlatpakStore::OnBatchTimer)
    EVT_TIMER(ID_LAYOUT_TIMER, FlatpakStore::OnLayoutTimer)
wxEND_EVENT_TABLE()

wxBEGIN_EVENT_TABLE(AppCard, wxPanel)
    EVT_PAINT(AppCard::OnPaint)
    EVT_ENTER_WINDOW(AppCard::OnMouseEnter)
    EVT_LEAVE_WINDOW(AppCard::OnMouseLeave)
wxEND_EVENT_TABLE()

wxBEGIN_EVENT_TABLE(RoundedSearchPanel, wxPanel)
    EVT_PAINT(RoundedSearchPanel::OnPaint)
wxEND_EVENT_TABLE()

namespace
{
    const char *const FLATHUB_SEARCH_URL = "https://flathub.org/api/v2/search";
    const char *const FLATHUB_RECENT_URL = "https://flathub.org/api/v2/collection/recently-added";
    const char *const FLATHUB_ICON_URL = "https://dl.flathub.org/repo/appstream/x86_64/icons/128x128/";
    const wxChar FIELD_SEPARATOR = 0x1F; // Разделител между полетата на резултат (не се среща в текст)
    const int ICON_SIZE = 64;

    size_t WriteToString(void *contents, size_t size, size_t nmemb, void *userp)
    {
        static_cast<std::string *>(userp)->append(static_cast<char *>(contents), size * nmemb);
        return size * nmemb;
    }

    // Прекъсва изтеглянето, когато магазинът поиска спиране (ненулев резултат отменя заявката)
    int AbortWhenStopped(void *clientp, curl_off_t, curl_off_t, curl_off_t, curl_off_t)
    {
        return static_cast<std::atomic<bool> *>(clientp)->load() ? 1 : 0;
    }

    // HTTP заявка (GET, или POST с JSON, ако body не е празно). Връща false при грешка или спиране.
    bool HttpRequest(const std::string &url, const std::string &body, std::string &response, std::atomic<bool> &stop)
    {
        CURL *curl = curl_easy_init();
        if (!curl)
            return false;

        struct curl_slist *headers = nullptr;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteToString);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);                         // HTTP 4xx/5xx = грешка
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "LinuxISOPro/" LINUXISOPRO_VERSION);
        curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, (long)CURLSSLOPT_NATIVE_CA); // Сертификатите се проверяват чрез хранилището на Windows
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, AbortWhenStopped);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &stop);
        if (!body.empty())
        {
            headers = curl_slist_append(headers, "Content-Type: application/json");
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        }

        CURLcode res = curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK)
        {
            wxLogDebug("HTTP request to %s failed: %s", url, curl_easy_strerror(res));
            return false;
        }
        return !stop;
    }

    // Мащабира иконата до ICON_SIZE x ICON_SIZE, като запазва пропорциите и прозрачността
    wxImage PrepareIcon(wxImage image)
    {
        if (!image.HasAlpha())
            image.InitAlpha();
        double scale = std::min(static_cast<double>(ICON_SIZE) / image.GetWidth(),
                                static_cast<double>(ICON_SIZE) / image.GetHeight());
        image.Rescale(std::max(1, static_cast<int>(image.GetWidth() * scale)),
                      std::max(1, static_cast<int>(image.GetHeight() * scale)), wxIMAGE_QUALITY_HIGH);
        if (image.GetWidth() == ICON_SIZE && image.GetHeight() == ICON_SIZE)
            return image;

        wxImage padded(ICON_SIZE, ICON_SIZE); // Прозрачен квадрат, в чийто център се поставя иконата
        padded.InitAlpha();
        memset(padded.GetAlpha(), 0, ICON_SIZE * ICON_SIZE);
        padded.Paste(image, (ICON_SIZE - image.GetWidth()) / 2, (ICON_SIZE - image.GetHeight()) / 2);
        return padded;
    }
}

// ---------------------------------------------------------------------------
// RoundedSearchPanel
// ---------------------------------------------------------------------------

RoundedSearchPanel::RoundedSearchPanel(wxWindow *parent, wxWindowID id, const wxPoint &pos, const wxSize &size)
    : wxPanel(parent, id, pos, size, wxBORDER_NONE)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(parent->GetBackgroundColour());
}

void RoundedSearchPanel::OnPaint(wxPaintEvent &event)
{
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(wxBrush(GetBackgroundColour()));
    dc.Clear();

    wxGraphicsContext *gc = wxGraphicsContext::Create(dc);
    if (gc)
    {
        wxRect rect = GetClientRect();
        gc->SetBrush(wxBrush(wxColour(55, 65, 81)));
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);
        gc->DrawRoundedRectangle(rect.x, rect.y, rect.width, rect.height, 10);
        delete gc;
    }
}

// ---------------------------------------------------------------------------
// AppCard
// ---------------------------------------------------------------------------

AppCard::AppCard(wxWindow *parent, const wxString &name, const wxString &summary, const wxString &appId)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(320, 160), wxBORDER_NONE),
      m_appId(appId)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(wxColour(55, 65, 81));

    wxBoxSizer *mainSizer = new wxBoxSizer(wxHORIZONTAL);
    mainSizer->AddSpacer(12);

    wxBitmap placeholderBmp(ICON_SIZE, ICON_SIZE); // Цветен квадрат, докато иконата се изтегля
    {
        wxMemoryDC dc;
        dc.SelectObject(placeholderBmp);
        dc.SetBackground(wxBrush(wxColour(79, 70, 229)));
        dc.Clear();
        dc.SelectObject(wxNullBitmap);
    }
    m_iconBitmap = new wxStaticBitmap(this, wxID_ANY, placeholderBmp);
    mainSizer->Add(m_iconBitmap, 0, wxALIGN_CENTER_VERTICAL | wxTOP | wxBOTTOM, 12);

    wxBoxSizer *textSizer = new wxBoxSizer(wxVERTICAL);
    textSizer->AddSpacer(8);

    m_nameText = new wxStaticText(this, wxID_ANY, name.IsEmpty() ? appId : name);
    m_nameText->SetForegroundColour(wxColour(229, 229, 229));
    m_nameText->SetFont(wxFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));

    m_summaryText = new wxStaticText(this, wxID_ANY, summary);
    m_summaryText->SetForegroundColour(wxColour(156, 163, 175));
    m_summaryText->Wrap(220);

    textSizer->Add(m_nameText, 0, wxBOTTOM, 6);
    textSizer->Add(m_summaryText, 1, wxEXPAND);

    wxBoxSizer *rightSizer = new wxBoxSizer(wxVERTICAL);
    rightSizer->Add(textSizer, 1, wxEXPAND);

    m_controlSizer = new wxBoxSizer(wxHORIZONTAL);
    m_installButton = new wxButton(this, wxID_ANY, "Install", wxDefaultPosition, wxSize(90, 34));
    m_installButton->SetBackgroundColour(wxColour(79, 70, 229));
    m_installButton->SetForegroundColour(*wxWHITE);

    m_progressGauge = new wxGauge(this, wxID_ANY, 100, wxDefaultPosition, wxSize(90, 20));
    m_progressGauge->Hide();

    m_controlSizer->Add(m_installButton, 0, wxALIGN_CENTER);
    m_controlSizer->Add(m_progressGauge, 0, wxALIGN_CENTER);

    rightSizer->Add(m_controlSizer, 0, wxALIGN_RIGHT | wxBOTTOM | wxRIGHT, 12);
    mainSizer->Add(rightSizer, 1, wxEXPAND | wxLEFT, 12);
    SetSizer(mainSizer);
}

void AppCard::SetInstallButtonLabel(const wxString &label)
{
    m_installButton->SetLabel(label);
}

void AppCard::DisableInstallButton()
{
    m_installButton->Disable();
}

void AppCard::SetIcon(const wxBitmap &bitmap)
{
    m_iconBitmap->SetBitmap(bitmap);
    Refresh();
}

void AppCard::ShowInstalling(bool show)
{
    m_controlSizer->Show(m_installButton, !show);
    m_controlSizer->Show(m_progressGauge, show);
    if (show)
        m_progressGauge->Pulse(); // Неопределен прогрес, докато flatpak install работи
    Layout();
}

void AppCard::OnPaint(wxPaintEvent &event)
{
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(wxBrush(GetParent()->GetBackgroundColour()));
    dc.Clear();

    wxGraphicsContext *gc = wxGraphicsContext::Create(dc);
    if (gc)
    {
        wxRect rect = GetClientRect();
        gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);
        gc->SetBrush(wxBrush(GetBackgroundColour()));
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->DrawRoundedRectangle(0, 0, rect.width, rect.height, 10);
        delete gc;
    }
}

void AppCard::OnMouseEnter(wxMouseEvent &event)
{
    SetBackgroundColour(wxColour(65, 75, 91));
    Refresh();
}

void AppCard::OnMouseLeave(wxMouseEvent &event)
{
    SetBackgroundColour(wxColour(55, 65, 81));
    Refresh();
}

// ---------------------------------------------------------------------------
// SearchThread
// ---------------------------------------------------------------------------

wxThread::ExitCode SearchThread::Entry()
{
    std::atomic<bool> &stop = *m_stopFlag;
    std::string response;
    bool ok;

    if (m_query.empty())
    {
        ok = HttpRequest(FLATHUB_RECENT_URL, "", response, stop);
    }
    else
    {
        rapidjson::StringBuffer body; // {"query": "...", "filters": []} с правилно екранирани символи
        rapidjson::Writer<rapidjson::StringBuffer> writer(body);
        writer.StartObject();
        writer.Key("query");
        writer.String(m_query.c_str());
        writer.Key("filters");
        writer.StartArray();
        writer.EndArray();
        writer.EndObject();
        ok = HttpRequest(FLATHUB_SEARCH_URL, body.GetString(), response, stop);
    }

    int found = 0;
    rapidjson::Document doc;
    if (ok && !doc.Parse(response.c_str()).HasParseError() && doc.IsObject() && doc.HasMember("hits") && doc["hits"].IsArray())
    {
        const rapidjson::Value &hits = doc["hits"];
        for (rapidjson::SizeType i = 0; i < hits.Size() && !stop; i++)
        {
            const rapidjson::Value &hit = hits[i];
            if (!hit.IsObject())
                continue;
            auto field = [&hit](const char *name)
            {
                return (hit.HasMember(name) && hit[name].IsString()) ? wxString::FromUTF8(hit[name].GetString()) : wxString();
            };
            wxString appId = field("app_id");
            if (appId.IsEmpty())
                continue;

            wxString data = field("name") + FIELD_SEPARATOR + field("summary") + FIELD_SEPARATOR + field("icon") + FIELD_SEPARATOR + appId;
            wxCommandEvent *event = new wxCommandEvent(wxEVT_RESULT_READY);
            event->SetInt(m_searchId);
            event->SetExtraLong(hits.Size()); // Общ брой резултати за прогреса
            event->SetString(data);
            if (!FlatpakStore::PostFromThread(m_lifetime, m_store, event))
                return (ExitCode)0; // Магазинът е затворен
            found++;
        }
    }
    else if (!ok)
    {
        found = -1; // Заявката е неуспешна (няма интернет, грешка в Flathub и т.н.)
    }

    if (!stop)
    {
        wxCommandEvent *done = new wxCommandEvent(wxEVT_SEARCH_COMPLETE);
        done->SetInt(m_searchId);
        done->SetExtraLong(found);
        FlatpakStore::PostFromThread(m_lifetime, m_store, done);
    }
    return (ExitCode)0;
}

// ---------------------------------------------------------------------------
// FlatpakStore
// ---------------------------------------------------------------------------

FlatpakStore::FlatpakStore(wxWindow *parent, const wxString &workDir)
    : wxPanel(parent, wxID_ANY),
      m_displayedResults(0),
      m_searchId(0),
      m_workDir(workDir),
      m_lifetime(std::make_shared<StoreLifetime>()),
      m_searchStop(std::make_shared<std::atomic<bool>>(false))
{
    static const bool curlInitialized = (curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK); // Веднъж, преди нишките да използват curl
    (void)curlInitialized;

    SetDoubleBuffered(true);
    m_threadPool = std::make_unique<ThreadPool>(4);
    m_batchTimer = new wxTimer(this, ID_BATCH_TIMER);
    m_layoutTimer = new wxTimer(this, ID_LAYOUT_TIMER);

    SetBackgroundColour(wxColour(40, 44, 52));
    m_mainSizer = new wxBoxSizer(wxVERTICAL);

    // --- Лента за търсене ---
    RoundedSearchPanel *searchPanel = new RoundedSearchPanel(this, wxID_ANY);
    wxBoxSizer *searchSizer = new wxBoxSizer(wxHORIZONTAL);

    m_searchBox = new wxTextCtrl(searchPanel, wxID_ANY, "", wxDefaultPosition,
                                 wxSize(-1, 42), wxTE_PROCESS_ENTER | wxBORDER_NONE);
    m_searchBox->SetBackgroundColour(wxColour(55, 65, 81));
    m_searchBox->SetForegroundColour(wxColour(229, 229, 229));
    m_searchBox->SetHint("Search Flathub for applications...");
    wxFont searchFont = m_searchBox->GetFont();
    searchFont.SetPointSize(searchFont.GetPointSize() + 1);
    m_searchBox->SetFont(searchFont);

    wxFont buttonFont = GetFont();
    buttonFont.SetWeight(wxFONTWEIGHT_BOLD);

    m_searchButton = new wxButton(searchPanel, wxID_ANY, "Search", wxDefaultPosition, wxSize(120, 42), wxBORDER_NONE);
    m_searchButton->SetBackgroundColour(wxColour(79, 70, 229));
    m_searchButton->SetForegroundColour(*wxWHITE);
    m_searchButton->SetFont(buttonFont);

    m_showInstalledButton = new wxButton(searchPanel, wxID_ANY, "Show Installed", wxDefaultPosition, wxSize(120, 42), wxBORDER_NONE);
    m_showInstalledButton->SetBackgroundColour(wxColour(79, 70, 229));
    m_showInstalledButton->SetForegroundColour(*wxWHITE);
    m_showInstalledButton->SetFont(buttonFont);

    searchSizer->Add(m_searchBox, 1, wxEXPAND | wxALL, 10);
    searchSizer->Add(m_searchButton, 0, wxALL, 10);
    searchSizer->Add(m_showInstalledButton, 0, wxALL, 10);
    searchPanel->SetSizer(searchSizer);

    // --- Прогрес ---
    m_progressPanel = new wxPanel(this, wxID_ANY);
    m_progressPanel->SetBackgroundColour(wxColour(40, 44, 52));
    wxBoxSizer *progressSizer = new wxBoxSizer(wxVERTICAL);
    m_progressText = new wxStaticText(m_progressPanel, wxID_ANY, "");
    m_progressText->SetForegroundColour(wxColour(229, 229, 229));
    progressSizer->Add(m_progressText, 0, wxEXPAND | wxBOTTOM, 5);
    m_progressBar = new wxGauge(m_progressPanel, wxID_ANY, 100, wxDefaultPosition, wxSize(-1, 8));
    progressSizer->Add(m_progressBar, 0, wxEXPAND);
    m_progressPanel->SetSizer(progressSizer);
    m_progressPanel->Hide();

    // --- Резултати ---
    m_resultsPanel = new wxScrolledWindow(this, wxID_ANY);
    m_resultsPanel->SetDoubleBuffered(true);
    m_resultsPanel->SetBackgroundColour(wxColour(40, 44, 52));
    m_resultsPanel->SetScrollRate(0, 20);
    wxBoxSizer *resultsSizer = new wxBoxSizer(wxVERTICAL);
    m_gridSizer = new wxWrapSizer(wxHORIZONTAL, wxWRAPSIZER_DEFAULT_FLAGS);
    resultsSizer->Add(m_gridSizer, 1, wxEXPAND | wxALL, 15);
    m_resultsPanel->SetSizer(resultsSizer);

    m_mainSizer->Add(searchPanel, 0, wxEXPAND | wxALL, 15);
    m_mainSizer->Add(m_progressPanel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 15);
    m_mainSizer->Add(m_resultsPanel, 1, wxEXPAND);
    SetSizer(m_mainSizer);

    // Бутоните се свързват директно, за да не стигат до EVT_BUTTON(wxID_ANY) за картите
    m_searchButton->Bind(wxEVT_BUTTON, &FlatpakStore::OnSearch, this);
    m_showInstalledButton->Bind(wxEVT_BUTTON, &FlatpakStore::OnShowInstalledButtonClicked, this);

    StartSearch(wxEmptyString); // Показва наскоро добавените приложения
}

FlatpakStore::~FlatpakStore()
{
    {
        std::lock_guard<std::mutex> lock(m_lifetime->mutex);
        m_lifetime->alive = false; // Нишките вече няма да изпращат събития към този обект
    }
    *m_searchStop = true; // Прекъсва текущите HTTP заявки

    if (m_threadPool)
    {
        m_threadPool->CancelAll();
        m_threadPool.reset(); // Изчаква започналите изтегляния (те се прекъсват бързо заради флага)
    }

    m_batchTimer->Stop();
    m_layoutTimer->Stop();
    delete m_batchTimer;
    delete m_layoutTimer;
}

bool FlatpakStore::PostFromThread(const std::shared_ptr<StoreLifetime> &lifetime, FlatpakStore *store, wxEvent *event)
{
    std::lock_guard<std::mutex> lock(lifetime->mutex);
    if (!lifetime->alive)
    {
        delete event;
        return false;
    }
    wxQueueEvent(store, event); // Нишково-безопасно; събитието се обработва в главната нишка
    return true;
}

void FlatpakStore::StopCurrentSearch()
{
    *m_searchStop = true;                                          // Старото търсене и иконите му спират
    m_searchStop = std::make_shared<std::atomic<bool>>(false);     // Нов флаг за следващата операция
    m_searchId++;                                                  // Закъснели резултати от старото търсене се игнорират
}

void FlatpakStore::ClearResults()
{
    m_batchTimer->Stop();
    m_resultsPanel->Freeze();
    m_gridSizer->Clear(true); // Унищожава картите и текстовете в решетката
    m_pendingResults.clear();
    m_displayedResults = 0;
    m_resultsPanel->Scroll(0, 0);
    m_resultsPanel->Layout();
    m_resultsPanel->Thaw();
    m_resultsPanel->Refresh();
}

void FlatpakStore::ShowMessage(const wxString &message)
{
    wxStaticText *text = new wxStaticText(m_resultsPanel, wxID_ANY, message);
    text->SetForegroundColour(wxColour(229, 229, 229));
    m_gridSizer->Add(text, 0, wxALL, 15);
    m_resultsPanel->Layout();
    m_resultsPanel->FitInside();
}

void FlatpakStore::StartSearch(const wxString &query)
{
    StopCurrentSearch();
    ClearResults();

    m_progressBar->SetValue(0);
    m_progressText->SetLabel(query.IsEmpty() ? wxString("Loading recently added apps...")
                                             : wxString::Format("Searching for '%s'...", query));
    m_progressPanel->Show();
    Layout();

    SearchThread *thread = new SearchThread(this, m_lifetime, m_searchStop, std::string(query.ToUTF8().data()), m_searchId);
    if (thread->Run() != wxTHREAD_NO_ERROR)
    {
        delete thread;
        m_progressPanel->Hide();
        Layout();
        ShowMessage("Could not start the search thread.");
    }
}

void FlatpakStore::OnSearch(wxCommandEvent &event)
{
    wxString query = m_searchBox->GetValue().Trim(true).Trim(false);
    if (query.IsEmpty())
    {
        wxMessageBox("Please enter a search term", "Search", wxOK | wxICON_INFORMATION, this);
        return;
    }
    StartSearch(query);
}

void FlatpakStore::OnResultReady(wxCommandEvent &event)
{
    if (event.GetInt() != m_searchId) // Резултат от предишно търсене
        return;

    m_pendingResults.push_back(event.GetString());
    if (!m_batchTimer->IsRunning())
        m_batchTimer->StartOnce(10); // Картите се добавят на порции, за да не замръзва интерфейсът

    m_displayedResults++;
    long total = event.GetExtraLong();
    if (total > 0)
    {
        m_progressBar->SetValue(static_cast<int>(std::min<long>(100, m_displayedResults * 100 / total)));
        m_progressText->SetLabel(wxString::Format("Loading applications: %d of %ld", m_displayedResults, total));
    }
}

void FlatpakStore::OnSearchComplete(wxCommandEvent &event)
{
    if (event.GetInt() != m_searchId)
        return;

    ProcessPendingBatch(true);
    m_progressPanel->Hide();
    Layout();

    long found = event.GetExtraLong();
    if (found < 0)
        ShowMessage("Could not reach Flathub. Check your internet connection and try again.");
    else if (found == 0)
        ShowMessage("No results");
}

void FlatpakStore::OnBatchTimer(wxTimerEvent &event)
{
    ProcessPendingBatch(false);
}

void FlatpakStore::ProcessPendingBatch(bool processAll)
{
    size_t batchSize = processAll ? m_pendingResults.size() : std::min<size_t>(10, m_pendingResults.size());
    if (batchSize == 0)
        return;

    m_resultsPanel->Freeze();
    for (size_t i = 0; i < batchSize; i++)
    {
        wxArrayString fields = wxSplit(m_pendingResults[i], FIELD_SEPARATOR, wxT('\0')); // name, summary, icon, app_id
        if (fields.GetCount() == 4)
            AddCard(fields[0], fields[1], fields[2], fields[3]);
    }
    m_pendingResults.erase(m_pendingResults.begin(), m_pendingResults.begin() + batchSize);
    m_resultsPanel->Layout();
    m_resultsPanel->Thaw();
    m_layoutTimer->StartOnce(100);

    if (!m_pendingResults.empty() && !m_batchTimer->IsRunning())
        m_batchTimer->StartOnce(50);
}

AppCard *FlatpakStore::AddCard(const wxString &name, const wxString &summary, const wxString &iconUrl, const wxString &appId)
{
    AppCard *card = new AppCard(m_resultsPanel, name, summary, appId);
    m_gridSizer->Add(card, 0, wxALL, 5);
    if (!iconUrl.IsEmpty())
        LoadIconAsync(card, iconUrl);
    return card;
}

void FlatpakStore::LoadIconAsync(AppCard *card, const wxString &iconUrl)
{
    std::shared_ptr<StoreLifetime> lifetime = m_lifetime;
    std::shared_ptr<std::atomic<bool>> stop = m_searchStop;
    std::string url(iconUrl.ToUTF8().data());
    FlatpakStore *store = this;

    // Нишката не използва card - само го връща в събитието; OnImageReady проверява дали картата още съществува
    m_threadPool->Enqueue([store, lifetime, stop, card, url]()
    {
        std::string bytes;
        if (*stop || !HttpRequest(url, "", bytes, *stop))
            return;

        IconData *data = new IconData{card, wxImage()};
        {
            wxMemoryInputStream stream(bytes.data(), bytes.size());
            wxImage image;
            if (!image.LoadFile(stream, wxBITMAP_TYPE_ANY) || !image.IsOk())
            {
                delete data;
                return;
            }
            data->image = PrepareIcon(image);
        } // Локалното копие се освобождава, преди данните да се предадат на главната нишка

        wxCommandEvent *event = new wxCommandEvent(wxEVT_IMAGE_READY);
        event->SetClientData(data);
        if (!FlatpakStore::PostFromThread(lifetime, store, event))
            delete data;
    });
}

void FlatpakStore::OnImageReady(wxCommandEvent &event)
{
    IconData *data = static_cast<IconData *>(event.GetClientData());
    if (!data)
        return;

    // Картата може да е унищожена от по-ново търсене, преди иконата да пристигне
    for (wxWindow *child : m_resultsPanel->GetChildren())
    {
        if (child == data->card)
        {
            data->card->SetIcon(wxBitmap(data->image));
            break;
        }
    }
    delete data;
}

void FlatpakStore::OnLayoutTimer(wxTimerEvent &event)
{
    m_resultsPanel->Layout();
    m_resultsPanel->FitInside();
    m_resultsPanel->Refresh();
}

// Инсталира приложението в ISO образа: flatpak install в chroot средата на контейнера
void FlatpakStore::OnInstallButtonClicked(wxCommandEvent &event)
{
    wxButton *button = dynamic_cast<wxButton *>(event.GetEventObject());
    AppCard *card = button ? dynamic_cast<AppCard *>(button->GetParent()) : nullptr;
    if (!card) // Бутонът не е от карта
    {
        event.Skip();
        return;
    }

    wxString containerId = ContainerCommand::ReadContainerId(m_workDir);
    if (containerId.IsEmpty())
    {
        wxMessageBox("The Docker container is not ready yet. Wait until the initial setup has finished.",
                     "Container Not Ready", wxOK | wxICON_INFORMATION, this);
        return;
    }

    wxString appId = card->GetAppId();
    if (!ContainerCommand::IsValidFlatpakId(appId))
    {
        wxMessageBox("Invalid application ID: " + appId, "Install", wxOK | wxICON_ERROR, this);
        return;
    }

    card->ShowInstalling(true);
    wxWeakRef<AppCard> cardRef(card); // Картата може да бъде унищожена (ново търсене) преди края на инсталацията
    wxString appName = card->GetName();
    wxArrayString appIds;
    appIds.Add(appId);

    long pid = ContainerCommand::RunInChrootAsync(containerId, ContainerCommand::FlatpakInstallScript(appIds),
                                                  [cardRef, containerId, appName](int status)
    {
        if (!cardRef)
            return;
        cardRef->ShowInstalling(false);
        if (status == 0)
        {
            cardRef->SetInstallButtonLabel("Installed");
            cardRef->DisableInstallButton();
            return;
        }

        cardRef->SetInstallButtonLabel("Retry");
        wxString reason = (status == ContainerCommand::FLATPAK_MISSING_EXIT_CODE)
                              ? wxString("Flatpak is not available in this image.")
                              : wxString("flatpak install failed.");
        wxMessageBox(reason + "\n\nLast lines of the log:\n" + ContainerCommand::FlatpakInstallLogTail(containerId),
                     "Installing " + appName + " failed", wxOK | wxICON_WARNING, cardRef);
    });

    if (pid == 0)
    {
        card->ShowInstalling(false);
        wxMessageBox("Could not start the installation. Is Docker Desktop running?", "Install", wxOK | wxICON_ERROR, this);
    }
}

// Показва приложенията, които вече са инсталирани в ISO образа (flatpak list в chroot средата)
void FlatpakStore::OnShowInstalledButtonClicked(wxCommandEvent &event)
{
    wxString containerId = ContainerCommand::ReadContainerId(m_workDir);
    if (containerId.IsEmpty())
    {
        wxMessageBox("The Docker container is not ready yet. Wait until the initial setup has finished.",
                     "Container Not Ready", wxOK | wxICON_INFORMATION, this);
        return;
    }

    StopCurrentSearch();
    ClearResults();
    m_progressPanel->Hide();
    Layout();

    wxArrayString output;
    long exitCode;
    {
        wxBusyCursor busy;
        exitCode = ContainerCommand::RunInChroot(containerId, "flatpak list --app --columns=application,name 2>/dev/null", &output);
    }
    if (exitCode != 0)
    {
        ShowMessage("Could not list the installed applications. Is Flatpak installed in this image?");
        return;
    }

    int count = 0;
    for (const wxString &line : output)
    {
        wxString appId = line.BeforeFirst('\t');
        wxString name = line.AfterFirst('\t');
        appId.Trim(true).Trim(false);
        name.Trim(true).Trim(false);
        if (!ContainerCommand::IsValidFlatpakId(appId))
            continue;

        AppCard *card = AddCard(name.IsEmpty() ? appId : name, appId, FLATHUB_ICON_URL + appId + ".png", appId);
        card->SetInstallButtonLabel("Installed");
        card->DisableInstallButton();
        count++;
    }

    if (count == 0)
        ShowMessage("No Flatpak applications are installed in this image yet.");
    m_resultsPanel->Layout();
    m_resultsPanel->FitInside();
}

// ---------------------------------------------------------------------------
// ThreadPool
// ---------------------------------------------------------------------------

ThreadPool::ThreadPool(size_t threads) : stop(false)
{
    for (size_t i = 0; i < threads; ++i)
    {
        workers.emplace_back([this]
        {
            while (true)
            {
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

ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(queueMutex); // Заключването трябва да е освободено преди join,
        stop = true;                                   // иначе работните нишки не могат да се събудят (deadlock)
    }
    condition.notify_all();
    for (std::thread &worker : workers)
    {
        if (worker.joinable())
            worker.join();
    }
}

void ThreadPool::CancelAll()
{
    std::unique_lock<std::mutex> lock(queueMutex);
    std::queue<std::function<void()>>().swap(tasks); // Изчиства опашката
}
