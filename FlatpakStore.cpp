#include "FlatpakStore.h"           // Включва заглавния файл за FlatpakStore класа.
#include <fstream>                  // За работа с файлови потоци (четене/запис на файлове).
#include <sstream>                  // За работа с низови потоци (манипулиране на низове като потоци).
#include <algorithm>                // За различни алгоритми, като std::min.
#include <wx/tokenzr.h>             // За wxStringTokenizer, който разделя низове.
#include <wx/filename.h>            // За wxFileName, който работи с имена на файлове и пътища.
#include <wx/stdpaths.h>            // За wxStandardPaths, който предоставя стандартни системни пътища.
#include <wx/log.h>                 // За wxLogDebug и други функции за отчитане на грешки/отстраняване на грешки.
#include <rapidjson/document.h>     // За работа с JSON документи (парсване и манипулиране).
#include <rapidjson/writer.h>       // За запис на JSON документи.
#include <rapidjson/stringbuffer.h> // За буфер за низове за rapidjson::Writer.
#include <iostream>                 // За стандартни входно-изходни операции (като std::cerr).
#include <wx/progdlg.h>             // За wxProgressDialog, диалогов прозорец за показване на прогрес.

// Define event types
wxDEFINE_EVENT(wxEVT_SEARCH_COMPLETE, wxCommandEvent);  // Дефинира потребителски тип събитие за завършване на търсенето.
wxDEFINE_EVENT(wxEVT_RESULT_READY, wxCommandEvent);     // Дефинира потребителски тип събитие за готов резултат.
wxDEFINE_EVENT(wxEVT_IMAGE_READY, wxCommandEvent);      // Дефинира потребителски тип събитие за готов образ.
wxDEFINE_EVENT(wxEVT_UPDATE_PROGRESS, wxCommandEvent);  // Дефинира потребителски тип събитие за актуализация на прогреса.
wxDEFINE_EVENT(wxEVT_BATCH_PROCESS, wxCommandEvent);    // Дефинира потребителски тип събитие за обработка на партида (batch).
wxDEFINE_EVENT(INSTALL_START_EVENT, wxCommandEvent);    // Дефинира потребителски тип събитие за начало на инсталация.
wxDEFINE_EVENT(INSTALL_COMPLETE_EVENT, wxCommandEvent); // Дефинира потребителски тип събитие за завършване на инсталация.
wxDEFINE_EVENT(INSTALL_CANCEL_EVENT, wxCommandEvent);   // Дефинира потребителски тип събитие за отказ на инсталация.

// Event tables
BEGIN_EVENT_TABLE(FlatpakStore, wxPanel)                                         // Започва дефиницията на таблицата със събития за класа FlatpakStore.
EVT_BUTTON(wxID_ANY, FlatpakStore::OnInstallButtonClicked)                       // Обработва събития за кликване на бутон, като извиква OnInstallButtonClicked.
EVT_TEXT_ENTER(wxID_ANY, FlatpakStore::OnSearch)                                 // Обработва събития за натискане на Enter в текстово поле, като извиква OnSearch.
EVT_COMMAND(wxID_ANY, wxEVT_SEARCH_COMPLETE, FlatpakStore::OnSearchComplete)     // Обработва потребителски събития за завършване на търсенето.
EVT_COMMAND(wxID_ANY, wxEVT_RESULT_READY, FlatpakStore::OnResultReady)           // Обработва потребителски събития за готов резултат.
EVT_COMMAND(wxID_ANY, wxEVT_IMAGE_READY, FlatpakStore::OnImageReady)             // Обработва потребителски събития за готов образ.
EVT_COMMAND(wxID_ANY, wxEVT_UPDATE_PROGRESS, FlatpakStore::OnUpdateProgress)     // Обработва потребителски събития за актуализация на прогреса.
EVT_TIMER(ID_BATCH_TIMER, FlatpakStore::OnBatchTimer)                            // Обработва събития от таймер за обработка на партиди.
EVT_TIMER(ID_LAYOUT_TIMER, FlatpakStore::OnLayoutTimer)                          // Обработва събития от таймер за актуализация на оформлението.
EVT_COMMAND(wxID_ANY, INSTALL_COMPLETE_EVENT, FlatpakStore::OnInstallComplete)   // Обработва потребителски събития за завършване на инсталация.
EVT_COMMAND(wxID_ANY, INSTALL_CANCEL_EVENT, FlatpakStore::OnInstallCancel)       // Обработва потребителски събития за отказ на инсталация.
EVT_BUTTON(ID_SHOW_INSTALLED_BUTTON, FlatpakStore::OnShowInstalledButtonClicked) // Обработва събития за кликване на бутон "Покажи инсталираните".
END_EVENT_TABLE()                                                                // Освобождава паметта на пула.

BEGIN_EVENT_TABLE(AppCard, wxPanel)     // Започва дефиницията на таблицата със събития за класа AppCard.
EVT_PAINT(AppCard::OnPaint)             // Обработва събития за прерисуване на панела (OnPaint).
EVT_ENTER_WINDOW(AppCard::OnMouseEnter) // Обработва събития при влизане на мишката в прозореца.
EVT_LEAVE_WINDOW(AppCard::OnMouseLeave) // Обработва събития при напускане на мишката от прозореца.
END_EVENT_TABLE()                       // Освобождава паметта на пула.

BEGIN_EVENT_TABLE(RoundedSearchPanel, wxPanel) // Започва дефиницията на таблицата със събития за класа RoundedSearchPanel.
EVT_PAINT(RoundedSearchPanel::OnPaint)         // Обработва събития за прерисуване на панела (OnPaint).
END_EVENT_TABLE()                              // Освобождава паметта на пула.

// Helper Functions (unchanged)
// Помощни функции (непроменени)

// Callback функция за запис на данни, получени от CURL, в std::string буфер.
size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *buffer)
{
    size_t newLength = size * nmemb; // Изчислява размера на новите данни.
    try
    {
        buffer->append(reinterpret_cast<char *>(contents), newLength); // Добавя данните към буфера.
    }
    catch (std::bad_alloc &e) // Прихваща грешки при заделяне на памет.
    {
        // wxLogDebug("Memory allocation failed in WriteCallback: %s", e.what()); // Коментар за отстраняване на грешки.
        return 0; // Връща 0, за да укаже грешка.
    }
    return newLength; // Връща броя на записаните байтове.
}

// Callback функция за запис на двоични данни, получени от CURL, в std::vector<unsigned char> буфер.
size_t WriteCallbackBinary(void *contents, size_t size, size_t nmemb, std::vector<unsigned char> *buffer)
{
    size_t newLength = size * nmemb; // Изчислява размера на новите данни.
    try
    {
        buffer->insert(buffer->end(), reinterpret_cast<unsigned char *>(contents),
                       reinterpret_cast<unsigned char *>(contents) + newLength); // Добавя данните към вектора.
    }
    catch (std::bad_alloc &e) // Прихваща грешки при заделяне на памет.
    {
        // wxLogDebug("Memory allocation failed in WriteCallbackBinary: %s", e.what()); // Коментар за отстраняване на грешки.
        return 0; // Връща 0, за да укаже грешка.
    }
    return newLength; // Връща броя на записаните байтове.
}

// Изтегля изображение от даден URL адрес в буфер.
bool DownloadImage(const std::string &url, std::vector<unsigned char> &buffer, std::atomic<bool> &stopFlag)
{
    CURL *curl = curl_easy_init(); // Инициализира CURL сесия.
    if (!curl)                     // Проверява за грешка при инициализация.
        return false;              // Връща false при грешка.

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());                   // Задава URL адреса за изтегляне.
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallbackBinary); // Задава callback функция за запис на двоични данни.
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);                 // Задава буфера за запис на данните.
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);                 // Позволява пренасочване (redirects).
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);                       // Задава време за изчакване (timeout) от 10 секунди.
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);                 // Деактивира проверката на SSL сертификата (за по-просто, но по-малко сигурно).

    CURLcode res = curl_easy_perform(curl); // Изпълнява CURL заявката.
    curl_easy_cleanup(curl);                // Почиства CURL ресурсите.

    if (res != CURLE_OK || stopFlag) // Проверява за грешка при изпълнение или ако е подаден сигнал за спиране.
    {
        wxLogDebug("Image download failed or stopped: %s", curl_easy_strerror(res)); // Отстраняване на грешки.
        wxBitmap placeholderBmp(48, 48);                                             // Създава placeholder изображение.
        wxMemoryDC dc;                                                               // Създава контекст на устройството в паметта.
        dc.SelectObject(placeholderBmp);                                             // Избира placeholder изображението.
        dc.SetBackground(wxBrush(wxColour(79, 70, 229)));                            // Задава фон.
        dc.Clear();                                                                  // Изчиства фона.
        dc.SelectObject(wxNullBitmap);                                               // Отменя избора на изображението.

        wxImage img = placeholderBmp.ConvertToImage(); // Преобразува bitmap в изображение.
        wxMemoryOutputStream memStream;                // Създава изходен поток в паметта.
        img.SaveFile(memStream, wxBITMAP_TYPE_PNG);    // Записва изображението като PNG в потока.

        size_t size = memStream.GetLength();   // Взема размера на записаните данни.
        buffer.resize(size);                   // Преоразмерява буфера.
        memStream.CopyTo(buffer.data(), size); // Копира данните в буфера.
    }
    return !buffer.empty(); // Връща true, ако буферът не е празен (т.е., изображението е изтеглено или заменено с placeholder).
}

// Търси приложения във Flathub API.
std::string searchFlathubApps(const std::string &query, std::atomic<bool> &stopFlag)
{
    CURL *curl = curl_easy_init(); // Инициализира CURL сесия.
    if (!curl)                     // Проверява за грешка при инициализация.
        return "";                 // Връща празен низ при грешка.

    std::string response;                                                 // Буфер за съхранение на отговора.
    std::string url = "https://flathub.org/api/v2/search";                // URL адрес за търсене.
    std::string payload = "{\"query\":\"" + query + "\",\"filters\":[]}"; // JSON тяло на заявката.

    struct curl_slist *headers = nullptr;                                   // Указател към списък с HTTP хедъри.
    headers = curl_slist_append(headers, "Content-Type: application/json"); // Добавя Content-Type хедър.

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());             // Задава URL адреса.
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());  // Задава POST данни (JSON payload).
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback); // Задава callback функция за запис на данни.
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);         // Задава буфера за запис на отговора.
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);          // Задава HTTP хедърите.
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);                 // Задава време за изчакване (timeout) от 10 секунди.
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);           // Деактивира проверката на SSL сертификата.

    CURLcode res = curl_easy_perform(curl); // Изпълнява CURL заявката.

    curl_slist_free_all(headers); // Освобождава паметта, заета от списъка с хедъри.
    curl_easy_cleanup(curl);      // Почиства CURL ресурсите.

    if (res != CURLE_OK || stopFlag) // Проверява за грешка при изпълнение или ако е подаден сигнал за спиране.
    {
        wxLogDebug("Flathub search failed or stopped: %s", curl_easy_strerror(res)); // Отстраняване на грешки.
        return "";                                                                   // Връща празен низ при грешка.
    }

    return response; // Връща получения отговор.
}

// Извлича наскоро добавени приложения от Flathub API.
std::string fetchRecentlyAddedApps(std::atomic<bool> &stopFlag)
{
    CURL *curl = curl_easy_init(); // Инициализира CURL сесия.
    if (!curl)                     // Проверява за грешка при инициализация.
        return "";                 // Връща празен низ при грешка.

    std::string response;                                                     // Буфер за съхранение на отговора.
    std::string url = "https://flathub.org/api/v2/collection/recently-added"; // URL адрес за наскоро добавени.

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());             // Задава URL адреса.
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback); // Задава callback функция за запис на данни.
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);         // Задава буфера за запис на отговора.
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);                 // Задава време за изчакване (timeout) от 10 секунди.
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);           // Деактивира проверката на SSL сертификата.

    CURLcode res = curl_easy_perform(curl); // Изпълнява CURL заявката.
    curl_easy_cleanup(curl);                // Почиства CURL ресурсите.

    if (res != CURLE_OK || stopFlag) // Проверява за грешка при изпълнение или ако е подаден сигнал за спиране.
    {
        wxLogDebug("Flathub recently added fetch failed or stopped: %s", curl_easy_strerror(res)); // Отстраняване на грешки.
        return "";                                                                                 // Връща празен низ при грешка.
    }

    return response; // Връща получения отговор.
}

// RoundedSearchPanel Implementation (unchanged)
// Имплементация на RoundedSearchPanel (непроменена)
RoundedSearchPanel::RoundedSearchPanel(wxWindow *parent, wxWindowID id, const wxPoint &pos, const wxSize &size)
    : wxPanel(parent, id, pos, size, wxBORDER_NONE) // Конструктор: инициализира базовия клас wxPanel без рамка.
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);               // Задава стил на фон за ръчно рисуване.
    SetBackgroundColour(parent->GetBackgroundColour()); // Задава цвета на фона от родителя.
}

void RoundedSearchPanel::OnPaint(wxPaintEvent &event) // Обработва събитието за прерисуване.
{
    wxAutoBufferedPaintDC dc(this);                   // Създава DC (Device Context) за двойно буферирано рисуване.
    dc.SetBackground(wxBrush(GetBackgroundColour())); // Задава фон за DC.
    dc.Clear();                                       // Изчиства DC.

    wxGraphicsContext *gc = wxGraphicsContext::Create(dc); // Създава графичен контекст.
    if (gc)                                                // Проверява дали контекстът е създаден успешно.
    {
        wxRect rect = GetClientRect();                                         // Взема размера на клиентската област.
        gc->SetBrush(wxBrush(wxColour(55, 65, 81)));                           // Задава четка за запълване с цвят.
        gc->SetPen(*wxTRANSPARENT_PEN);                                        // Задава прозрачен контур.
        gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);                             // Задава режим за антиалиасинг.
        gc->DrawRoundedRectangle(rect.x, rect.y, rect.width, rect.height, 10); // Рисува заоблен правоъгълник.
        delete gc;                                                             // Освобождава графичния контекст.
    }
}

// AppCard Implementation
// Имплементация на AppCard
void AppCard::SetInstallButtonLabel(const wxString &label) // Задава текста на бутона за инсталиране.
{
    wxLogDebug("Setting install button label to: %s for app: %s", label, m_appId); // Отстраняване на грешки.
    m_installButton->SetLabel(label);                                              // Задава текста.
}

void AppCard::DisableInstallButton() // Деактивира бутона за инсталиране.
{
    wxLogDebug("Disabling install button for app: %s", m_appId); // Отстраняване на грешки.
    m_installButton->Disable();                                  // Деактивира бутона.
}

AppCard::AppCard(wxWindow *parent, const wxString &name, const wxString &summary, const wxString &appId)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(320, 160), wxBORDER_NONE), // Конструктор: инициализира базовия клас wxPanel.
      m_appId(appId), m_isLoading(true)                                              // Инициализира член-данни.
{
    wxLogDebug("Creating AppCard for app: %s", appId);  // Отстраняване на грешки.
    SetBackgroundStyle(wxBG_STYLE_PAINT);               // Задава стил на фон за ръчно рисуване.
    SetBackgroundColour(parent->GetBackgroundColour()); // Задава цвета на фона от родителя.

    wxBoxSizer *mainSizer = new wxBoxSizer(wxHORIZONTAL); // Създава основен хоризонтален sizer.
    mainSizer->AddSpacer(12);                             // Добавя разстояние.

    wxBitmap placeholderBmp(64, 64); // Създава placeholder bitmap за иконата.
    {
        wxMemoryDC dc;                                    // Създава DC в паметта.
        dc.SelectObject(placeholderBmp);                  // Избира bitmap-а.
        dc.SetBackground(wxBrush(wxColour(79, 70, 229))); // Задава фон.
        dc.Clear();                                       // Изчиства.
        dc.SelectObject(wxNullBitmap);                    // Отменя избора.
    }
    m_iconBitmap = new wxStaticBitmap(this, wxID_ANY, placeholderBmp);               // Създава статично изображение за иконата.
    mainSizer->Add(m_iconBitmap, 0, wxALIGN_CENTER_VERTICAL | wxTOP | wxBOTTOM, 12); // Добавя иконата към sizer-а.

    wxBoxSizer *textSizer = new wxBoxSizer(wxVERTICAL); // Създава вертикален sizer за текст.
    textSizer->AddSpacer(8);                            // Добавя разстояние.

    m_nameText = new wxStaticText(this, wxID_ANY, name.IsEmpty() ? "Loading app..." : name); // Създава статичен текст за името.
    m_nameText->SetForegroundColour(wxColour(229, 229, 229));                                // Задава цвят на текста.
    wxFont nameFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);          // Задава шрифт.
    m_nameText->SetFont(nameFont);                                                           // Прилага шрифта.

    m_summaryText = new wxStaticText(this, wxID_ANY, summary.IsEmpty() ? "Loading description..." : summary); // Създава статичен текст за резюмето.
    m_summaryText->SetForegroundColour(wxColour(156, 163, 175));                                              // Задава цвят на текста.
    m_summaryText->Wrap(220);                                                                                 // Ограничава текста на 220 пиксела ширина.

    textSizer->Add(m_nameText, 0, wxBOTTOM, 6); // Добавя името към текстовия sizer.
    textSizer->Add(m_summaryText, 1, wxEXPAND); // Добавя резюмето към текстовия sizer.

    wxBoxSizer *rightSizer = new wxBoxSizer(wxVERTICAL); // Създава вертикален sizer за дясната част.
    rightSizer->Add(textSizer, 1, wxEXPAND);             // Добавя текстовия sizer.

    m_controlSizer = new wxBoxSizer(wxHORIZONTAL);                                                // Създава хоризонтален sizer за контролите.
    m_installButton = new wxButton(this, wxID_ANY, "Install", wxDefaultPosition, wxSize(90, 34)); // Създава бутон "Install".
    m_installButton->SetBackgroundColour(wxColour(79, 70, 229));                                  // Задава цвят на фона.
    m_installButton->SetForegroundColour(*wxWHITE);                                               // Задава цвят на текста.
    m_installButton->SetClientData(new wxString(appId));                                          // Задава ID на приложението като клиентски данни.

    m_progressGauge = new wxGauge(this, wxID_ANY, 100, wxDefaultPosition, wxSize(60, 20)); // Създава индикатор за прогрес.
    m_progressGauge->Hide();                                                               // Скрива индикатора за прогрес по подразбиране.

    m_cancelButton = new wxButton(this, wxID_ANY, "Cancel", wxDefaultPosition, wxSize(30, 20)); // Създава бутон "Cancel".
    m_cancelButton->Hide();                                                                     // Скрива бутона "Cancel" по подразбиране.

    m_controlSizer->Add(m_installButton, 0, wxALIGN_CENTER);             // Добавя бутона "Install" към контролния sizer.
    m_controlSizer->Add(m_progressGauge, 0, wxALIGN_CENTER | wxLEFT, 5); // Добавя индикатора за прогрес.
    m_controlSizer->Add(m_cancelButton, 0, wxALIGN_CENTER | wxLEFT, 5);  // Добавя бутона "Cancel".

    rightSizer->Add(m_controlSizer, 0, wxALIGN_RIGHT | wxBOTTOM | wxRIGHT, 12); // Добавя контролния sizer към десния sizer.
    mainSizer->Add(rightSizer, 1, wxEXPAND | wxLEFT, 12);                       // Добавя десния sizer към основния sizer.
    SetSizer(mainSizer);                                                        // Задава основния sizer за AppCard.
}

AppCard::~AppCard() // Деструктор на AppCard.
{
    wxLogDebug("Destroying AppCard for app: %s", m_appId);                          // Отстраняване на грешки.
    wxString *appIdPtr = static_cast<wxString *>(m_installButton->GetClientData()); // Взема указателя към клиентските данни.
    if (appIdPtr)                                                                   // Проверява дали указателя е валиден.
    {
        delete appIdPtr;                         // Изтрива динамично заделената памет.
        m_installButton->SetClientData(nullptr); // Задава указателя на null.
    }
}

void AppCard::SetIcon(const wxBitmap &bitmap) // Задава иконата на приложението.
{
    wxLogDebug("Setting icon for app: %s", m_appId); // Отстраняване на грешки.
    m_iconBitmap->SetBitmap(bitmap);                 // Задава bitmap-а за статичното изображение.
    m_isLoading = false;                             // Показва, че зареждането е приключило.
    Refresh();                                       // Прерисува контрола.
}

void AppCard::ShowInstalling(bool show) // Показва или скрива UI за инсталиране.
{
    wxLogDebug("ShowInstalling set to %d for app: %s", show, m_appId); // Отстраняване на грешки.
    if (show)                                                          // Ако трябва да се покаже инсталационния прогрес.
    {
        m_controlSizer->Hide(m_installButton);                             // Скрива бутона "Install".
        m_controlSizer->Show(m_progressGauge);                             // Показва индикатора за прогрес.
        m_controlSizer->Show(m_cancelButton);                              // Показва бутона "Cancel".
        m_progressGauge->Pulse();                                          // Започва импулсно движение на прогрес бара.
        wxLogDebug("Progress gauge started pulsing for app: %s", m_appId); // Отстраняване на грешки.
    }
    else // Ако трябва да се скрие инсталационния прогрес.
    {
        m_controlSizer->Show(m_installButton);                   // Показва бутона "Install".
        m_controlSizer->Hide(m_progressGauge);                   // Скрива индикатора за прогрес.
        m_controlSizer->Hide(m_cancelButton);                    // Скрива бутона "Cancel".
        wxLogDebug("Installing UI hidden for app: %s", m_appId); // Отстраняване на грешки.
    }
    Layout(); // Актуализира оформлението.
}

void AppCard::OnPaint(wxPaintEvent &event) // Обработва събитието за прерисуване.
{
    wxAutoBufferedPaintDC dc(this);                                // Създава DC за двойно буферирано рисуване.
    dc.SetBackground(wxBrush(GetParent()->GetBackgroundColour())); // Задава фон от родителя.
    dc.Clear();                                                    // Изчиства DC.

    wxGraphicsContext *gc = wxGraphicsContext::Create(dc); // Създава графичен контекст.
    if (gc)                                                // Проверява дали контекстът е създаден успешно.
    {
        wxRect rect = GetClientRect();                               // Взема размера на клиентската област.
        wxColour bgColor = GetBackgroundColour();                    // Взема цвета на фона.
        gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);                   // Задава режим за антиалиасинг.
        gc->SetBrush(wxBrush(bgColor));                              // Задава четка за запълване.
        gc->SetPen(*wxTRANSPARENT_PEN);                              // Задава прозрачен контур.
        gc->DrawRoundedRectangle(0, 0, rect.width, rect.height, 10); // Рисува заоблен правоъгълник.
        delete gc;                                                   // Освобождава графичния контекст.
    }
}

void AppCard::OnMouseEnter(wxMouseEvent &event) // Обработва събитие при влизане на мишката.
{
    SetBackgroundColour(wxColour(65, 75, 91)); // Променя цвета на фона.
    Refresh();                                 // Прерисува контрола.
}

void AppCard::OnMouseLeave(wxMouseEvent &event) // Обработва събитие при напускане на мишката.
{
    SetBackgroundColour(wxColour(55, 65, 81)); // Възстановява цвета на фона.
    Refresh();                                 // Прерисува контрола.
}

// SearchThread Implementation (unchanged)
// Имплементация на SearchThread (непроменена)
wxThread::ExitCode SearchThread::Entry() // Входна точка за нишката за търсене.
{
    try // Започва блок за прихващане на изключения.
    {
        if (TestDestroy() || m_stopFlag) // Проверява дали нишката трябва да бъде унищожена или спряна.
            return (ExitCode)0;          // Връща 0 (успех) ако нишката е спряна.

        std::string response = searchFlathubApps(m_query, m_stopFlag); // Изпълнява търсенето на Flathub.
        if (response.empty())                                          // Проверява дали отговорът е празен.
            return (ExitCode)1;                                        // Връща 1 (грешка) ако отговорът е празен.

        rapidjson::Document doc;                                                 // Създава rapidjson документ.
        if (doc.Parse(response.c_str()).HasParseError())                         // Парсва отговора и проверява за грешки.
            return (ExitCode)1;                                                  // Връща 1 при грешка при парсване.
        if (!doc.IsObject() || !doc.HasMember("hits") || !doc["hits"].IsArray()) // Проверява структурата на JSON.
            return (ExitCode)1;                                                  // Връща 1 при невалидна JSON структура.

        const rapidjson::Value &hits = doc["hits"]; // Взема масива "hits".
        m_store->SetTotalResults(hits.Size());      // Задава общия брой резултати в магазина.

        for (rapidjson::SizeType i = 0; i < hits.Size() && !m_stopFlag && !TestDestroy(); i++) // Итерира през резултатите.
        {
            const rapidjson::Value &hit = hits[i]; // Взема текущия резултат.
            if (!hit.IsObject() || !hit.HasMember("name") || !hit.HasMember("summary") ||
                !hit.HasMember("icon") || !hit.HasMember("app_id")) // Проверява необходимите полета.
                continue;                                           // Продължава към следващия резултат, ако полетата липсват.

            wxString resultData = wxString(hit["name"].GetString(), wxConvUTF8) + "|" +
                                  wxString(hit["summary"].GetString(), wxConvUTF8) + "|" +
                                  wxString(hit["icon"].GetString(), wxConvUTF8) + "|" +
                                  wxString(hit["app_id"].GetString(), wxConvUTF8); // Форматира данните за резултата.

            wxCommandEvent resultEvent(wxEVT_RESULT_READY); // Създава събитие за готов резултат.
            resultEvent.SetInt(m_searchId);                 // Задава ID на търсенето.
            resultEvent.SetString(resultData);              // Задава данните за резултата.
            wxQueueEvent(m_store, resultEvent.Clone());     // Поставя събитието в опашката на магазина.

            wxCommandEvent progressEvent(wxEVT_UPDATE_PROGRESS); // Създава събитие за актуализация на прогреса.
            progressEvent.SetInt(i + 1);                         // Задава текущия прогрес.
            wxQueueEvent(m_store, progressEvent.Clone());        // Поставя събитието в опашката.

            wxMilliSleep(5); // Пауза от 5 милисекунди, за да не се натоварва UI.
        }

        if (!m_stopFlag && !TestDestroy()) // Проверява дали нишката не е спряна или унищожена.
        {
            wxCommandEvent event(wxEVT_SEARCH_COMPLETE); // Създава събитие за завършване на търсенето.
            event.SetInt(m_searchId);                    // Задава ID на търсенето.
            wxQueueEvent(m_store, event.Clone());        // Поставя събитието в опашката.
        }
        return (ExitCode)0; // Връща 0 (успех).
    }
    catch (...) // Прихваща всякакви други изключения.
    {
        wxLogDebug("Exception in SearchThread::Entry()"); // Отстраняване на грешки.
        return (ExitCode)1;                               // Връща 1 (грешка).
    }
}

// InitialLoadThread Implementation (unchanged)
// Имплементация на InitialLoadThread (непроменена)
wxThread::ExitCode InitialLoadThread::Entry() // Входна точка за нишката за първоначално зареждане.
{
    try // Започва блок за прихващане на изключения.
    {
        if (TestDestroy() || m_stopFlag) // Проверява дали нишката трябва да бъде унищожена или спряна.
            return (ExitCode)0;          // Връща 0 (успех) ако нишката е спряна.

        std::string response = fetchRecentlyAddedApps(m_stopFlag); // Извлича наскоро добавени приложения.
        if (response.empty())                                      // Проверява дали отговорът е празен.
            return (ExitCode)1;                                    // Връща 1 (грешка) ако отговорът е празен.

        rapidjson::Document doc;                                                 // Създава rapidjson документ.
        if (doc.Parse(response.c_str()).HasParseError())                         // Парсва отговора и проверява за грешки.
            return (ExitCode)1;                                                  // Връща 1 при грешка при парсване.
        if (!doc.IsObject() || !doc.HasMember("hits") || !doc["hits"].IsArray()) // Проверява структурата на JSON.
            return (ExitCode)1;                                                  // Връща 1 при невалидна JSON структура.

        const rapidjson::Value &hits = doc["hits"]; // Взема масива "hits".
        m_store->SetTotalResults(hits.Size());      // Задава общия брой резултати в магазина.

        for (rapidjson::SizeType i = 0; i < hits.Size() && !m_stopFlag && !TestDestroy(); i++) // Итерира през резултатите.
        {
            const rapidjson::Value &hit = hits[i]; // Взема текущия резултат.
            if (!hit.IsObject() || !hit.HasMember("name") || !hit.HasMember("summary") ||
                !hit.HasMember("icon") || !hit.HasMember("app_id")) // Проверява необходимите полета.
                continue;                                           // Продължава към следващия резултат, ако полетата липсват.

            wxString resultData = wxString(hit["name"].GetString(), wxConvUTF8) + "|" +
                                  wxString(hit["summary"].GetString(), wxConvUTF8) + "|" +
                                  wxString(hit["icon"].GetString(), wxConvUTF8) + "|" +
                                  wxString(hit["app_id"].GetString(), wxConvUTF8); // Форматира данните за резултата.

            wxCommandEvent resultEvent(wxEVT_RESULT_READY); // Създава събитие за готов резултат.
            resultEvent.SetInt(-1);                         // Задава ID на търсенето (-1 за първоначално зареждане).
            resultEvent.SetString(resultData);              // Задава данните за резултата.
            wxQueueEvent(m_store, resultEvent.Clone());     // Поставя събитието в опашката на магазина.

            wxCommandEvent progressEvent(wxEVT_UPDATE_PROGRESS); // Създава събитие за актуализация на прогреса.
            progressEvent.SetInt(i + 1);                         // Задава текущия прогрес.
            wxQueueEvent(m_store, progressEvent.Clone());        // Поставя събитието в опашката.

            wxMilliSleep(5); // Пауза от 5 милисекунди, за да не се натоварва UI.
        }

        if (!m_stopFlag && !TestDestroy()) // Проверява дали нишката не е спряна или унищожена.
        {
            wxCommandEvent event(wxEVT_SEARCH_COMPLETE); // Създава събитие за завършване на търсенето.
            event.SetInt(-1);                            // Задава ID на търсенето (-1).
            wxQueueEvent(m_store, event.Clone());        // Поставя събитието в опашката.
        }
        return (ExitCode)0; // Връща 0 (успех).
    }
    catch (...) // Прихваща всякакви други изключения.
    {
        wxLogDebug("Exception in InitialLoadThread::Entry()"); // Отстраняване на грешки.
        return (ExitCode)1;                                    // Връща 1 (грешка).
    }
}

// InstallThread Implementation
// Имплементация на InstallThread
wxThread::ExitCode InstallThread::Entry() // Входна точка за нишката за инсталиране.
{
    wxLogDebug("InstallThread started for app: %s", m_appId); // Отстраняване на грешки.

    // Create events for simulating installation progress
    for (int progress = 10; progress <= 100; progress += 10)
    {
        if (m_state->cancelFlag || TestDestroy())
        {
            break;
        }

        // Send progress update event
        wxCommandEvent progressEvent(wxEVT_UPDATE_PROGRESS);
        progressEvent.SetInt(progress);
        progressEvent.SetClientData(m_state);
        wxQueueEvent(m_store, progressEvent.Clone());

        // Sleep to simulate work
        wxMilliSleep(500);
    }

    // If installation wasn't cancelled, send completion event
    if (!m_state->cancelFlag && !TestDestroy())
    {
        wxCommandEvent completeEvent(INSTALL_COMPLETE_EVENT);
        completeEvent.SetInt(0); // Success
        completeEvent.SetClientData(m_state);
        wxQueueEvent(m_store, completeEvent.Clone());
    }
    // If the thread was cancelled, send a cancel event
    else if (m_state->cancelFlag)
    {
        wxCommandEvent cancelEvent(INSTALL_CANCEL_EVENT);
        cancelEvent.SetClientData(m_state);
        wxQueueEvent(m_store, cancelEvent.Clone());
    }

    return (ExitCode)0; // Връща 0 (успех).
}

// FlatpakStore Implementation
// Имплементация на FlatpakStore
FlatpakStore::FlatpakStore(wxWindow *parent, const wxString &workDir)
    : wxPanel(parent, wxID_ANY), // Конструктор: инициализира базовия клас wxPanel.
      m_workDir(workDir),        // Инициализира работната директория.
      m_isSearching(false),      // Инициализира флаг за търсене.
      m_isInitialLoading(false), // Инициализира флаг за първоначално зареждане.
      totalResults(0),           // Инициализира общия брой резултати.
      m_stopFlag(false),         // Инициализира флаг за спиране.
      m_searchId(0),             // Инициализира ID на търсенето.
      m_displayedResults(0),     // Инициализира броя на показаните резултати.
      m_pendingBatchSize(0)      // Инициализира размера на чакащата партида.
{
    wxLogDebug("FlatpakStore constructor started"); // Отстраняване на грешки.
    SetDoubleBuffered(true);                        // Активира двойно буфериране за по-гладко рисуване.

    m_threadPool = std::make_unique<ThreadPool>(4); // Създава пул от нишки с 4 нишки.

    m_batchTimer = new wxTimer(this, ID_BATCH_TIMER);   // Създава таймер за обработка на партиди.
    m_layoutTimer = new wxTimer(this, ID_LAYOUT_TIMER); // Създава таймер за оформление.

    SetBackgroundColour(wxColour(40, 44, 52)); // Задава цвета на фона на панела.

    m_mainSizer = new wxBoxSizer(wxVERTICAL); // Създава основен вертикален sizer.

    RoundedSearchPanel *searchPanel = new RoundedSearchPanel(this, wxID_ANY); // Създава заоблен панел за търсене.
    wxBoxSizer *searchSizer = new wxBoxSizer(wxHORIZONTAL);                   // Създава хоризонтален sizer за елементите за търсене.

    m_searchBox = new wxTextCtrl(searchPanel, wxID_ANY, "", wxDefaultPosition,
                                 wxSize(-1, 42), wxTE_PROCESS_ENTER | wxBORDER_NONE); // Създава текстово поле за търсене.
    m_searchBox->SetBackgroundColour(wxColour(55, 65, 81));                           // Задава цвят на фона.
    m_searchBox->SetForegroundColour(wxColour(229, 229, 229));                        // Задава цвят на текста.
    m_searchBox->SetHint("Search for applications...");                               // Задава подкана (hint text).

    wxFont searchFont = m_searchBox->GetFont();             // Взема текущия шрифт.
    searchFont.SetPointSize(searchFont.GetPointSize() + 1); // Увеличава размера на шрифта.
    m_searchBox->SetFont(searchFont);                       // Прилага новия шрифт.

    m_searchButton = new wxButton(searchPanel, wxID_ANY, "Search",
                                  wxDefaultPosition, wxSize(120, 42), wxBORDER_NONE); // Създава бутон "Search".
    m_searchButton->SetBackgroundColour(wxColour(79, 70, 229));                       // Задава цвят на фона.
    m_searchButton->SetForegroundColour(*wxWHITE);                                    // Задава цвят на текста.

    wxFont buttonFont = m_searchButton->GetFont(); // Взема текущия шрифт на бутона.
    buttonFont.SetWeight(wxFONTWEIGHT_BOLD);       // Задава удебелен шрифт.
    m_searchButton->SetFont(buttonFont);           // Прилага новия шрифт.

    m_showInstalledButton = new wxButton(searchPanel, ID_SHOW_INSTALLED_BUTTON, "Show Installed",
                                         wxDefaultPosition, wxSize(120, 42), wxBORDER_NONE); // Създава бутон "Show Installed".
    m_showInstalledButton->SetBackgroundColour(wxColour(79, 70, 229));                       // Задава цвят на фона.
    m_showInstalledButton->SetForegroundColour(*wxWHITE);                                    // Задава цвят на текста.
    m_showInstalledButton->SetFont(buttonFont);                                              // Прилага удебеления шрифт.

    searchSizer->Add(m_searchBox, 1, wxEXPAND | wxALL, 10); // Добавя текстовото поле за търсене към sizer-а.
    searchSizer->Add(m_searchButton, 0, wxALL, 10);         // Добавя бутона "Search" към sizer-а.
    searchSizer->Add(m_showInstalledButton, 0, wxALL, 10);  // Добавя новия бутон към sizer-а.
    searchPanel->SetSizer(searchSizer);                     // Задава sizer-а за панела за търсене.

    m_progressPanel = new wxPanel(this, wxID_ANY);              // Създава панел за прогрес.
    m_progressPanel->SetBackgroundColour(wxColour(40, 44, 52)); // Задава цвят на фона.

    wxBoxSizer *progressSizer = new wxBoxSizer(wxVERTICAL); // Създава вертикален sizer за елементите на прогрес.

    m_progressText = new wxStaticText(m_progressPanel, wxID_ANY, ""); // Създава статичен текст за прогреса.
    m_progressText->SetForegroundColour(wxColour(229, 229, 229));     // Задава цвят на текста.
    progressSizer->Add(m_progressText, 0, wxEXPAND | wxBOTTOM, 5);    // Добавя текста за прогрес.

    m_progressBar = new wxGauge(m_progressPanel, wxID_ANY, 100, wxDefaultPosition, wxSize(-1, 8)); // Създава прогрес бар.
    m_progressBar->SetBackgroundColour(wxColour(55, 65, 81));                                      // Задава цвят на фона на прогрес бара.
    m_progressBar->SetForegroundColour(wxColour(79, 70, 229));                                     // Задава цвят на запълване на прогрес бара.
    progressSizer->Add(m_progressBar, 0, wxEXPAND);                                                // Добавя прогрес бара.

    m_progressPanel->SetSizer(progressSizer); // Задава sizer-а за панела за прогрес.
    m_progressPanel->Hide();                  // Скрива панела за прогрес по подразбиране.

    m_resultsPanel = new wxScrolledWindow(this, wxID_ANY);     // Създава скролиращ прозорец за резултатите.
    m_resultsPanel->SetDoubleBuffered(true);                   // Активира двойно буфериране.
    m_resultsPanel->SetBackgroundColour(wxColour(40, 44, 52)); // Задава цвят на фона.
    m_resultsPanel->SetScrollRate(0, 20);                      // Задава скорост на скролиране (само вертикално).

    wxBoxSizer *resultsSizer = new wxBoxSizer(wxVERTICAL);                  // Създава вертикален sizer за резултатите.
    m_gridSizer = new wxWrapSizer(wxHORIZONTAL, wxWRAPSIZER_DEFAULT_FLAGS); // Създава sizer, който обвива елементите хоризонтално.
    resultsSizer->Add(m_gridSizer, 1, wxEXPAND | wxALL, 15);                // Добавя grid sizer-а към sizer-а за резултати.
    m_resultsPanel->SetSizer(resultsSizer);                                 // Задава sizer-а за панела с резултати.

    m_mainSizer->Add(searchPanel, 0, wxEXPAND | wxALL, 15);                           // Добавя панела за търсене към основния sizer.
    m_mainSizer->Add(m_progressPanel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 15); // Добавя панела за прогрес.
    m_mainSizer->Add(m_resultsPanel, 1, wxEXPAND);                                    // Добавя панела с резултати.

    SetSizer(m_mainSizer); // Задава основния sizer за FlatpakStore.

    m_searchButton->Bind(wxEVT_BUTTON, &FlatpakStore::OnSearchButtonClicked, this);               // Свързва събитие за кликване на бутона за търсене.
    m_showInstalledButton->Bind(wxEVT_BUTTON, &FlatpakStore::OnShowInstalledButtonClicked, this); // Свързва събитие за кликване на бутона "Show Installed".

    wxLogDebug("Loading initial apps"); // Отстраняване на грешки.
    LoadInitialApps();                  // Зарежда първоначалните приложения.

    wxLogDebug("FlatpakStore constructor completed"); // Отстраняване на грешки.
}

void FlatpakStore::ClearResults() // Изчиства панела с резултати.
{
    wxLogDebug("Clearing results panel"); // Отстраняване на грешки.
    m_resultsPanel->Freeze();             // Замразява панела, за да избегне прерисуване по време на промени.
    m_gridSizer->Clear(true);             // Изчиства всички елементи от grid sizer-а и ги изтрива.
    m_displayedResults = 0;               // Нулира броя на показаните резултати.
    m_resultsPanel->Scroll(0, 0);         // Скролира панела до началото.
    m_resultsPanel->Layout();             // Актуализира оформлението.
    m_resultsPanel->Refresh();            // Прерисува панела.
    m_resultsPanel->Thaw();               // Размразява панела.

    std::lock_guard<std::mutex> lock(m_resultsMutex); // Заключва мютекс за резултатите.
    m_pendingResults.clear();                         // Изчиства чакащите резултати.
    m_pendingBatchSize = 0;                           // Нулира размера на чакащата партида.
    wxLogDebug("Results cleared");                    // Отстраняване на грешки.
}

void FlatpakStore::LoadInitialApps() // Зарежда първоначалните приложения (наскоро добавени).
{
    try // Започва блок за прихващане на изключения.
    {
        wxLogDebug("Starting LoadInitialApps");          // Отстраняване на грешки.
        std::lock_guard<std::mutex> lock(m_searchMutex); // Заключва мютекс за търсене.
        m_stopFlag = true;                               // Задава флаг за спиране на всяко текущо търсене.
        wxMilliSleep(50);                                // Кратка пауза за сигурност.

        ClearResults();                                             // Изчиства предишни резултати.
        m_progressBar->SetValue(0);                                 // Нулира прогрес бара.
        m_progressText->SetLabel("Loading recently added apps..."); // Актуализира текста за прогрес.
        m_progressPanel->Show();                                    // Показва панела за прогрес.
        Layout();                                                   // Актуализира оформлението.

        totalResults = 0;          // Нулира общия брой резултати.
        m_stopFlag = false;        // Изчиства флага за спиране.
        m_isInitialLoading = true; // Задава флаг, че се извършва първоначално зареждане.

        m_layoutTimer->Start(100); // Стартира таймер за актуализация на оформлението.

        InitialLoadThread *thread = new InitialLoadThread(this, m_stopFlag); // Създава нова нишка за първоначално зареждане.
        if (thread->Run() != wxTHREAD_NO_ERROR)                              // Опитва се да стартира нишката.
        {
            delete thread;                                                   // Изтрива нишката, ако не може да стартира.
            wxLogDebug("Failed to start InitialLoadThread");                 // Отстраняване на грешки.
            throw std::runtime_error("Failed to start initial load thread"); // Хвърля изключение.
        }
        wxLogDebug("InitialLoadThread started successfully"); // Отстраняване на грешки.
    }
    catch (const std::exception &e) // Прихваща стандартни изключения.
    {
        m_isInitialLoading = false;                               // Отменя флага за първоначално зареждане.
        m_progressPanel->Hide();                                  // Скрива панела за прогрес.
        Layout();                                                 // Актуализира оформлението.
        wxLogDebug("Exception in LoadInitialApps: %s", e.what()); // Отстраняване на грешки.
        wxMessageBox(wxString::Format("Initial load failed: %s", e.what()),
                     "Error", wxOK | wxICON_ERROR); // Показва съобщение за грешка.
    }
}

void FlatpakStore::OnSearch(wxCommandEvent &event) // Обработва събитие за търсене (от текстово поле или бутон).
{
    try // Започва блок за прихващане на изключения.
    {
        wxLogDebug("OnSearch triggered");                // Отстраняване на грешки.
        std::lock_guard<std::mutex> lock(m_searchMutex); // Заключва мютекс за търсене.
        wxString query = m_searchBox->GetValue();        // Взема текста от полето за търсене.
        if (query.IsEmpty())                             // Проверява дали заявката е празна.
        {
            wxLogDebug("Search query is empty");                                                   // Отстраняване на грешки.
            wxMessageBox("Please enter a search term", "Search Error", wxOK | wxICON_INFORMATION); // Показва съобщение.
            return;                                                                                // Излиза от функцията.
        }

        m_stopFlag = true; // Задава флаг за спиране на всяко текущо търсене.
        wxMilliSleep(50);  // Кратка пауза за сигурност.

        ClearResults();                                                             // Изчиства предишни резултати.
        m_progressBar->SetValue(0);                                                 // Нулира прогрес бара.
        m_progressText->SetLabel(wxString::Format("Searching for '%s'...", query)); // Актуализира текста за прогрес.
        m_progressPanel->Show();                                                    // Показва панела за прогрес.
        Layout();                                                                   // Актуализира оформлението.

        totalResults = 0;     // Нулира общия брой резултати.
        m_stopFlag = false;   // Изчиства флага за спиране.
        m_searchId++;         // Увеличава ID на търсенето.
        m_isSearching = true; // Задава флаг, че се извършва търсене.

        m_layoutTimer->Start(100); // Стартира таймер за актуализация на оформлението.

        SearchThread *thread = new SearchThread(this, query.ToStdString(), m_searchId, m_stopFlag); // Създава нова нишка за търсене.
        if (thread->Run() != wxTHREAD_NO_ERROR)                                                     // Опитва се да стартира нишката.
        {
            delete thread;                                             // Изтрива нишката, ако не може да стартира.
            wxLogDebug("Failed to start SearchThread");                // Отстраняване на грешки.
            throw std::runtime_error("Failed to start search thread"); // Хвърля изключение.
        }
        wxLogDebug("SearchThread started for query: %s", query); // Отстраняване на грешки.
    }
    catch (const std::exception &e) // Прихваща стандартни изключения.
    {
        m_isSearching = false;                             // Отменя флага за търсене.
        m_progressPanel->Hide();                           // Скрива панела за прогрес.
        Layout();                                          // Актуализира оформлението.
        wxLogDebug("Exception in OnSearch: %s", e.what()); // Отстраняване на грешки.
        wxMessageBox(wxString::Format("Search failed: %s", e.what()),
                     "Error", wxOK | wxICON_ERROR); // Показва съобщение за грешка.
    }
}

void FlatpakStore::OnSearchComplete(wxCommandEvent &event) // Обработва събитие за завършване на търсенето.
{
    wxLogDebug("OnSearchComplete triggered with ID: %d", event.GetInt()); // Отстраняване на грешки.
    if (event.GetInt() != m_searchId && event.GetInt() != -1)             // Проверява дали ID на събитието съвпада с текущото търсене или е за първоначално зареждане.
        return;                                                           // Излиза, ако не съвпада.

    m_isSearching = false;      // Отменя флага за търсене.
    m_isInitialLoading = false; // Отменя флага за първоначално зареждане.

    m_progressBar->SetValue(100);                             // Задава прогрес бара на 100%.
    m_progressText->SetLabel("Loading complete!");            // Актуализира текста за прогрес.
    wxLogDebug("Search completed, processing pending batch"); // Отстраняване на грешки.

    ProcessPendingBatch(true); // Обработва всички останали чакащи резултати.

    wxTimer *timer = new wxTimer(this);                    // Създава нов таймер.
    timer->StartOnce(1000);                                // Стартира таймера за еднократно изпълнение след 1 секунда.
    timer->Bind(wxEVT_TIMER, [this, timer](wxTimerEvent &) // Свързва събитието от таймера с lambda функция.
                {
        wxLogDebug("Hiding progress panel after search complete"); // Отстраняване на грешки.
        m_progressPanel->Hide(); // Скрива панела за прогрес.
        Layout(); // Актуализира оформлението.
        delete timer; });                                      // Изтрива таймера.
}

void FlatpakStore::OnResultReady(wxCommandEvent &event) // Обработва събитие за готов резултат.
{
    wxLogDebug("OnResultReady triggered with ID: %d", event.GetInt()); // Отстраняване на грешки.
    if (event.GetInt() != m_searchId && event.GetInt() != -1)          // Проверява ID на търсенето.
        return;                                                        // Излиза, ако не съвпада.

    wxString resultData = event.GetString();            // Взема данните за резултата.
    wxLogDebug("Received result data: %s", resultData); // Отстраняване на грешки.

    std::lock_guard<std::mutex> lock(m_resultsMutex); // Заключва мютекс за резултатите.
    m_pendingResults.push_back(resultData);           // Добавя резултата към чакащите.
    m_pendingBatchSize++;                             // Увеличава размера на чакащата партида.

    if (!m_batchTimer->IsRunning() && m_pendingBatchSize >= 1) // Ако таймерът не работи и има поне един резултат.
    {
        wxLogDebug("Starting batch timer with %d pending results", m_pendingBatchSize); // Отстраняване на грешки.
        m_batchTimer->StartOnce(10);                                                    // Стартира таймера за обработка на партиди.
    }

    m_displayedResults++; // Увеличава броя на показаните резултати.

    if (totalResults > 0) // Ако общият брой резултати е известен.
    {
        int progressPercent = static_cast<int>((m_displayedResults * 100) / totalResults); // Изчислява процент на прогрес.
        m_progressBar->SetValue(progressPercent);                                          // Задава стойността на прогрес бара.
        m_progressText->SetLabel(wxString::Format("Loading applications: %d of %d",
                                                  m_displayedResults, totalResults));                    // Актуализира текста за прогрес.
        wxLogDebug("Progress updated: %d%% (%d/%d)", progressPercent, m_displayedResults, totalResults); // Отстраняване на грешки.
    }
    else // Ако общият брой резултати не е известен.
    {
        m_progressBar->Pulse();                                   // Започва импулсно движение на прогрес бара.
        wxLogDebug("Progress bar pulsing, totalResults not set"); // Отстраняване на грешки.
    }
}

void FlatpakStore::OnBatchTimer(wxTimerEvent &event) // Обработва събитие от таймер за обработка на партида.
{
    wxLogDebug("OnBatchTimer triggered"); // Отстраняване на грешки.
    ProcessPendingBatch(false);           // Обработва част от чакащите резултати.
}

void FlatpakStore::ProcessPendingBatch(bool processAll) // Обработва чакащи резултати на партиди.
{
    wxLogDebug("ProcessPendingBatch started, processAll: %d", processAll); // Отстраняване на грешки.
    std::vector<wxString> currentBatch;                                    // Вектор за текущата партида.
    {
        std::lock_guard<std::mutex> lock(m_resultsMutex); // Заключва мютекс за резултатите.

        int batchSize = processAll ? m_pendingResults.size() : std::min(10, static_cast<int>(m_pendingResults.size())); // Определя размера на партидата.
        if (batchSize <= 0)                                                                                             // Ако няма резултати за обработка.
        {
            wxLogDebug("No pending results to process"); // Отстраняване на грешки.
            return;                                      // Излиза от функцията.
        }

        currentBatch.assign(m_pendingResults.begin(), m_pendingResults.begin() + batchSize);        // Копира елементи в текущата партида.
        m_pendingResults.erase(m_pendingResults.begin(), m_pendingResults.begin() + batchSize);     // Изтрива обработените елементи от чакащите.
        m_pendingBatchSize = static_cast<int>(m_pendingResults.size());                             // Актуализира размера на чакащата партида.
        wxLogDebug("Processing batch of %d results, remaining: %d", batchSize, m_pendingBatchSize); // Отстраняване на грешки.
    }

    if (currentBatch.empty()) // Ако текущата партида е празна.
    {
        wxLogDebug("Current batch is empty"); // Отстраняване на грешки.
        return;                               // Излиза от функцията.
    }

    m_resultsPanel->Freeze(); // Замразява панела с резултати.

    for (const wxString &resultData : currentBatch) // Итерира през елементите на текущата партида.
    {
        wxStringTokenizer tokenizer(resultData, "|");                    // Създава токенизатор за разделяне на низа.
        if (tokenizer.CountTokens() < 4)                                 // Проверява дали има достатъчно токени.
            continue;                                                    // Продължава към следващия елемент.
        wxString name = tokenizer.GetNextToken();                        // Взема името.
        wxString summary = tokenizer.GetNextToken();                     // Взема резюмето.
        wxString iconUrl = tokenizer.GetNextToken();                     // Взема URL на иконата.
        wxString appId = tokenizer.GetNextToken();                       // Взема ID на приложението.
        wxLogDebug("Adding AppCard - Name: %s, AppID: %s", name, appId); // Отстраняване на грешки.

        AppCard *card = new AppCard(m_resultsPanel, name, summary, appId); // Създава нова AppCard.
        m_gridSizer->Add(card, 0, wxALL, 5);                               // Добавя картата към grid sizer-а.

        m_threadPool->Enqueue([this, card, iconUrl]() // Поставя задача за изтегляне на икона в пула от нишки.
                              {
            wxLogDebug("Enqueuing icon download for app: %s", card->GetAppId()); // Отстраняване на грешки.
            std::vector<unsigned char> iconData; // Вектор за двоични данни на иконата.
            if (DownloadImage(iconUrl.ToStdString(), iconData, m_stopFlag)) { // Изтегля изображението.
                wxMemoryInputStream stream(iconData.data(), iconData.size()); // Създава входен поток от паметта.
                wxImage image; // Създава обект wxImage.
                if (image.LoadFile(stream, wxBITMAP_TYPE_PNG)) { // Зарежда изображението от потока.
                    const int target_size = 64; // Желан размер на иконата.
                    float scale = std::min(static_cast<float>(target_size) / image.GetWidth(), 
                                           static_cast<float>(target_size) / image.GetHeight()); // Изчислява мащаба.
                    
                    image.Rescale(static_cast<int>(image.GetWidth() * scale),
                                  static_cast<int>(image.GetHeight() * scale),
                                  wxIMAGE_QUALITY_HIGH); // Преоразмерява изображението.

                    if (image.GetWidth() != target_size || image.GetHeight() != target_size) { // Ако размерите не съвпадат.
                        wxImage padded(target_size, target_size); // Създава подплатено изображение.
                        padded.SetRGB(wxRect(0, 0, target_size, target_size), 79, 70, 229); // Запълва с цвят.
                        padded.Paste(image, (target_size - image.GetWidth())/2,
                                          (target_size - image.GetHeight())/2); // Поставя оригиналното изображение.
                        image = padded; // Заменя изображението с подплатеното.
                    }

                    wxBitmap bitmap(image); // Създава wxBitmap от изображението.
                    
                    wxCommandEvent imageEvent(wxEVT_IMAGE_READY); // Създава събитие за готова икона.
                    imageEvent.SetClientData(new IconData{card, bitmap}); // Задава данни за иконата.
                    wxLogDebug("Queuing IMAGE_READY event for app: %s", card->GetAppId()); // Отстраняване на грешки.
                    wxQueueEvent(this, imageEvent.Clone()); // Поставя събитието в опашката.
                } else {
                    wxLogDebug("Failed to load image for app: %s", card->GetAppId()); // Отстраняване на грешки.
                }
            } else {
                wxLogDebug("Image download failed for app: %s", card->GetAppId()); // Отстраняване на грешки.
            } });
    }

    m_gridSizer->Layout();         // Актуализира оформлението на grid sizer-а.
    m_resultsPanel->Layout();      // Актуализира оформлението на панела с резултати.
    m_layoutTimer->StartOnce(100); // Стартира таймер за оформление.

    m_resultsPanel->Thaw();                   // Размразява панела с резултати.
    wxLogDebug("Batch processing completed"); // Отстраняване на грешки.

    if (!m_pendingResults.empty() && !m_batchTimer->IsRunning()) // Ако има още чакащи резултати и таймерът не работи.
    {
        int delay = (currentBatch.size() == 1) ? 100 : 50;           // Определя забавяне.
        wxLogDebug("Starting batch timer with delay: %d ms", delay); // Отстраняване на грешки.
        m_batchTimer->StartOnce(delay);                              // Стартира таймера за обработка на партиди.
    }
}

void FlatpakStore::OnImageReady(wxCommandEvent &event) // Обработва събитие за готова икона.
{
    wxLogDebug("OnImageReady triggered");                            // Отстраняване на грешки.
    IconData *data = static_cast<IconData *>(event.GetClientData()); // Взема клиентските данни.
    if (data)                                                        // Проверява дали данните са валидни.
    {
        wxLogDebug("Setting icon for app: %s", data->card->GetAppId()); // Отстраняване на грешки.
        data->card->SetIcon(data->bitmap);                              // Задава иконата на картата.
        delete data;                                                    // Изтрива данните.
    }
    else
    {
        wxLogDebug("No IconData received"); // Отстраняване на грешки.
    }
}

void FlatpakStore::OnUpdateProgress(wxCommandEvent &event) // Обработва събитие за актуализация на прогреса.
{
    wxLogDebug("OnUpdateProgress triggered"); // Отстраняване на грешки.

    // Handle progress updates for installation
    InstallState *state = static_cast<InstallState *>(event.GetClientData());
    if (state && state->card)
    {
        int progress = event.GetInt();
        state->card->GetProgressGauge()->SetValue(progress);
        wxLogDebug("Updated progress gauge for app: %s to %d%%", state->card->GetAppId(), progress);
        return;
    }

    // Handle progress updates for search
    if (totalResults <= 0) // Ако общият брой резултати не е известен.
    {
        static int pulseVal = 0;                                 // Статична променлива за импулсния прогрес.
        pulseVal = (pulseVal + 5) % 100;                         // Актуализира стойността.
        m_progressBar->SetValue(pulseVal);                       // Задава стойността на прогрес бара.
        wxLogDebug("Pulsing progress bar, value: %d", pulseVal); // Отстраняване на грешки.
        return;                                                  // Излиза от функцията.
    }

    int current = event.GetInt();                                                                        // Взема текущия прогрес.
    int progressPercent = (current * 100) / totalResults;                                                // Изчислява процента.
    m_progressBar->SetValue(progressPercent);                                                            // Задава стойността на прогрес бара.
    m_progressText->SetLabel(wxString::Format("Loading applications: %d of %d", current, totalResults)); // Актуализира текста.
    wxLogDebug("Progress updated: %d%% (%d/%d)", progressPercent, current, totalResults);                // Отстраняване на грешки.
}

void FlatpakStore::OnLayoutTimer(wxTimerEvent &event) // Обработва събитие от таймер за оформление.
{
    wxLogDebug("OnLayoutTimer triggered"); // Отстраняване на грешки.
    m_resultsPanel->Layout();              // Актуализира оформлението на панела с резултати.
    m_resultsPanel->FitInside();           // Оразмерява панела, за да се побере съдържанието.
    m_resultsPanel->Refresh();             // Прерисува панела.
}

void FlatpakStore::OnInstallButtonClicked(wxCommandEvent &event) // Обработва събитие за кликване на бутон "Install".
{
    wxLogDebug("OnInstallButtonClicked triggered");                      // Отстраняване на грешки.
    wxButton *button = dynamic_cast<wxButton *>(event.GetEventObject()); // Взема указател към бутона.
    if (!button)                                                         // Проверява дали бутонът е валиден.
    {
        wxLogDebug("No button found in event"); // Отстраняване на грешки.
        return;                                 // Излиза от функцията.
    }

    AppCard *card = dynamic_cast<AppCard *>(button->GetParent()); // Взема указател към родителската AppCard.
    if (!card)                                                    // Проверява дали AppCard е валидна.
    {
        wxLogDebug("No AppCard found as parent of button"); // Отстраняване на грешки.
        return;                                             // Излиза от функцията.
    }

    wxString *appIdPtr = static_cast<wxString *>(button->GetClientData()); // Взема ID на приложението от клиентските данни.
    if (!appIdPtr)                                                         // Проверява дали ID е валидно.
    {
        wxLogDebug("No app ID found in button client data"); // Отстраняване на грешки.
        return;                                              // Излиза от функцията.
    }

    wxString appId = *appIdPtr;                              // Копира ID на приложението.
    wxLogDebug("Install button clicked for app: %s", appId); // Отстраняване на грешки.

    // Get the app name from the card
    // Вземане на името на приложението от картата
    wxString appName = card->GetName(); // Взема името на приложението.

    // Create installation state
    // Създаване на състояние за инсталация
    InstallState *state = new InstallState(card);          // Създава ново състояние за инсталация.
    wxLogDebug("Created InstallState for app: %s", appId); // Отстраняване на грешки.

    // Update UI and bind cancel button
    // Актуализиране на потребителския интерфейс и свързване на бутона за отказ
    card->ShowInstalling(true);                    // Показва UI за инсталиране.
    auto cancelHandler = [state](wxCommandEvent &) // Дефинира lambda функция за обработка на отказ.
    {
        wxLogDebug("Cancel button clicked for app: %s", state->card->GetAppId()); // Отстраняване на грешки.
        state->cancelFlag = true;                                                 // Задава флага за отказ.
    };
    card->GetCancelButton()->Bind(wxEVT_BUTTON, cancelHandler); // Свързва бутона "Cancel".
    wxLogDebug("Cancel button bound for app: %s", appId);       // Отстраняване на грешки.

    // Start installation thread
    // Стартиране на нишка за инсталация
    InstallThread *thread = new InstallThread(this, appId, state); // Създава нова нишка за инсталация.
    if (thread->Run() != wxTHREAD_NO_ERROR)                        // Опитва се да стартира нишката.
    {
        wxLogDebug("Failed to start InstallThread for app: %s", appId);             // Отстраняване на грешки.
        delete thread;                                                              // Изтрива нишката.
        card->GetCancelButton()->Unbind(wxEVT_BUTTON, cancelHandler);               // Отвързва бутона "Cancel".
        delete state;                                                               // Изтрива състоянието.
        card->ShowInstalling(false);                                                // Скрива UI за инсталиране.
        wxMessageBox("Failed to start installation thread", "Error", wxICON_ERROR); // Показва съобщение за грешка.
        return;                                                                     // Излиза от функцията.
    }
    wxLogDebug("InstallThread started successfully for app: %s", appId); // Отстраняване на грешки.
}

void FlatpakStore::OnInstallComplete(wxCommandEvent &event) // Обработва събитие за завършване на инсталация.
{
    wxLogDebug("OnInstallComplete triggered");                                // Отстраняване на грешки.
    InstallState *state = static_cast<InstallState *>(event.GetClientData()); // Взема състоянието.
    if (!state || !state->card)                                               // Проверява дали състоянието и картата са валидни.
    {
        wxLogDebug("Invalid InstallState or card in OnInstallComplete"); // Отстраняване на грешки.
        return;                                                          // Излиза от функцията.
    }

    int exitCode = event.GetInt(); // Взема кода за изход.
    wxLogDebug("Installation %s for app: %s with exit code: %d",
               (exitCode == 0 ? "successful" : "failed"), state->card->GetAppId(), exitCode); // Отстраняване на грешки.

    // Update UI
    wxString buttonLabel = (exitCode == 0) ? "Installed" : "Failed - Retry"; // Определя етикета на бутона.
    state->card->SetInstallButtonLabel(buttonLabel);                         // Задава етикета на бутона.
    state->card->ShowInstalling(false);                                      // Скрива UI за инсталиране.

    if (exitCode == 0)
    {
        state->card->DisableInstallButton(); // Деактивира бутона за инсталиране при успех.
    }

    // Clean up
    delete state; // Изтрива състоянието.

    wxLogDebug("OnInstallComplete finished");
}

void FlatpakStore::OnInstallCancel(wxCommandEvent &event) // Обработва събитие за отказ на инсталация.
{
    wxLogDebug("OnInstallCancel triggered");                                  // Отстраняване на грешки.
    InstallState *state = static_cast<InstallState *>(event.GetClientData()); // Взема състоянието за инсталация.
    if (!state || !state->card)                                               // Проверява дали състоянието и картата са валидни.
    {
        wxLogDebug("Invalid InstallState or card in OnInstallCancel"); // Отстраняване на грешки.
        return;                                                        // Излиза от функцията.
    }

    AppCard *card = state->card;                                                                      // Взема указател към картата на приложението.
    int cancelReason = event.GetInt();                                                                // Взема причината за отказа.
    wxLogDebug("Cancellation completed for app: %s with reason: %d", card->GetAppId(), cancelReason); // Отстраняване на грешки.

    card->ShowInstalling(false);                   // Скрива UI за инсталиране.
    auto cancelHandler = [state](wxCommandEvent &) // Дефинира lambda функция за обработка на отказ.
    { state->cancelFlag = true; };
    card->GetCancelButton()->Unbind(wxEVT_BUTTON, cancelHandler);      // Отвързва бутона "Cancel".
    wxLogDebug("Unbound cancel button for app: %s", card->GetAppId()); // Отстраняване на грешки.

    if (cancelReason == 1) // Ако е отказано от потребителя.
    {
        wxMessageBox("Installation of " + card->GetAppId() + " was canceled by user.", "Canceled", wxICON_INFORMATION); // Показва съобщение.
    }
    else if (cancelReason == 2) // Ако е изтекло времето.
    {
        wxMessageBox("Installation of " + card->GetAppId() + " timed out.", "Timeout", wxICON_WARNING); // Показва съобщение.
    }

    wxLogDebug("Cleaned up InstallState for app: %s after cancellation", card->GetAppId()); // Отстраняване на грешки.
    delete state;                                                                           // Изтрива състоянието.
}

void FlatpakStore::OnShowInstalledButtonClicked(wxCommandEvent &event) // Обработва събитие за кликване на бутон "Show Installed".
{
    wxLogDebug("OnShowInstalledButtonClicked triggered"); // Отстраняване на грешки.
    try                                                   // Започва блок за прихващане на изключения.
    {
        ClearResults();                                                 // Изчиства текущите резултати.
        m_progressBar->SetValue(0);                                     // Нулира прогрес бара.
        m_progressText->SetLabel("Fetching installed applications..."); // Актуализира текста за прогрес.
        m_progressPanel->Show();                                        // Показва панела за прогрес.
        Layout();                                                       // Актуализира оформлението.

        // Simulate fetching installed apps with a delay
        wxMilliSleep(500);

        // Simulate a list of installed applications
        wxArrayString simulatedApps;
        simulatedApps.Add("org.gnome.Calculator");
        simulatedApps.Add("org.gnome.Evince");
        simulatedApps.Add("org.gnome.FileRoller");
        simulatedApps.Add("org.gnome.Nautilus");
        simulatedApps.Add("org.mozilla.firefox");

        if (simulatedApps.IsEmpty()) // Ако няма изведени резултати.
        {
            wxLogDebug("No installed applications found");                                          // Отстраняване на грешки.
            wxStaticText *noResultsText = new wxStaticText(m_resultsPanel, wxID_ANY, "No results"); // Създава текст "No results".
            noResultsText->SetForegroundColour(wxColour(229, 229, 229));                            // Задава цвят на текста.
            m_gridSizer->Add(noResultsText, 0, wxALIGN_CENTER | wxALL, 15);                         // Добавя текста.
        }
        else // Ако има резултати.
        {
            for (const auto &appId : simulatedApps) // Итерира през всеки ред от изхода.
            {
                wxLogDebug("Adding installed AppCard - AppID: %s", appId);                          // Отстраняване на грешки.
                AppCard *card = new AppCard(m_resultsPanel, appId, "Installed application", appId); // Създава AppCard.
                card->SetInstallButtonLabel("Installed");                                           // Set button text to "Installed"
                card->DisableInstallButton();                                                       // Disable the button
                m_gridSizer->Add(card, 0, wxALL, 5);                                                // Добавя картата.
            }
        }

        m_gridSizer->Layout();    // Актуализира оформлението на grid sizer-а.
        m_resultsPanel->Layout(); // Актуализира оформлението на панела с резултати.
        m_progressPanel->Hide();  // Скрива панела за прогрес.
        Layout();                 // Актуализира оформлението.
    }
    catch (const std::exception &e) // Прихваща стандартни изключения.
    {
        wxLogDebug("Exception in OnShowInstalledButtonClicked: %s", e.what());                                                // Отстраняване на грешки.
        wxMessageBox(wxString::Format("Failed to fetch installed applications: %s", e.what()), "Error", wxOK | wxICON_ERROR); // Показва съобщение за грешка.
        m_progressPanel->Hide();                                                                                              // Скрива панела за прогрес.
        Layout();                                                                                                             // Актуализира оформлението.
    }
}

void FlatpakStore::OnSearchButtonClicked(wxCommandEvent &event) // Обработва събитие за кликване на бутона за търсене.
{
    OnSearch(event); // Извиква функцията OnSearch.
}

FlatpakStore::~FlatpakStore() // Деструктор на FlatpakStore.
{
    wxLogDebug("FlatpakStore destructor started"); // Отстраняване на грешки.
    try                                            // Започва блок за прихващане на изключения.
    {
        std::lock_guard<std::mutex> lock(m_searchMutex); // Заключва мютекс за търсене.
        m_stopFlag = true;                               // Задава флаг за спиране.
        if (m_threadPool)                                // Ако пулът от нишки съществува.
        {
            m_threadPool->CancelAll(); // Отменя всички задачи.
            m_threadPool.reset();      // Освобождава паметта на пула.
        }

        if (m_batchTimer) // Ако таймерът за партиди съществува.
        {
            m_batchTimer->Stop(); // Спира таймера.
            delete m_batchTimer;  // Изтрива таймера.
        }

        if (m_layoutTimer) // Ако таймерът за оформление съществува.
        {
            m_layoutTimer->Stop(); // Спира таймера.
            delete m_layoutTimer;  // Изтрива таймера.
        }

        wxLogDebug("FlatpakStore destructor completed"); // Отстраняване на грешки.
    }
    catch (const std::exception &e) // Прихваща стандартни изключения.
    {
        wxLogDebug("Exception in destructor: %s", e.what());            // Отстраняване на грешки.
        std::cerr << "Error during cleanup: " << e.what() << std::endl; // Извежда грешка в stderr.
    }
}

// ThreadPool Implementation (unchanged)
// Имплементация на ThreadPool (непроменена)
ThreadPool::ThreadPool(size_t threads) : stop(false) // Конструктор на ThreadPool.
{
    wxLogDebug("ThreadPool constructor with %zu threads", threads); // Отстраняване на грешки.
    for (size_t i = 0; i < threads; ++i)                            // Създава зададения брой нишки.
    {
        workers.emplace_back([this] // Добавя нова нишка към вектора.
                             {
            while (true) { // Безкраен цикъл за всяка нишка.
                std::function<void()> task; // Променлива за съхранение на задачата.
                {
                    std::unique_lock<std::mutex> lock(queueMutex); // Заключва мютекс на опашката.
                    condition.wait(lock, [this] { return stop || !tasks.empty(); }); // Чака, докато има задачи или е подаден сигнал за спиране.
                    if (stop && tasks.empty()) // Ако е подаден сигнал за спиране и опашката е празна.
                        return; // Нишката излиза.
                    task = std::move(tasks.front()); // Взема първата задача.
                    tasks.pop(); // Изтрива задачата от опашката.
                }
                task(); // Изпълнява задачата.
            } });
    }
}

ThreadPool::~ThreadPool() // Деструктор на ThreadPool.
{
    wxLogDebug("ThreadPool destructor started");   // Отстраняване на грешки.
    std::unique_lock<std::mutex> lock(queueMutex); // Заключва мютекс на опашката.
    stop = true;                                   // Задава флага за спиране.
    condition.notify_all();                        // Известява всички чакащи нишки.
    for (std::thread &worker : workers)            // Итерира през всички работни нишки.
    {
        if (worker.joinable()) // Ако нишката може да бъде свързана (joinable).
            worker.join();     // Изчаква нишката да завърши.
    }
    wxLogDebug("ThreadPool destructor completed"); // Отстраняване на грешки.
}

void ThreadPool::CancelAll() // Отменя всички чакащи задачи.
{
    wxLogDebug("ThreadPool::CancelAll called");    // Отстраняване на грешки.
    std::unique_lock<std::mutex> lock(queueMutex); // Заключва мютекс на опашката.
    while (!tasks.empty())                         // Докато опашката не е празна.
        tasks.pop();                               // Изтрива задачите.
}