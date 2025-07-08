#ifndef FLATPAKSTORE_H // Ако FLATPAKSTORE_H не е дефиниран,
#define FLATPAKSTORE_H // дефинирай FLATPAKSTORE_H. Това предотвратява многократно включване на заглавния файл.

#include <wx/wx.h>              // Включва основния заглавен файл на wxWidgets.
#include <wx/sizer.h>           // Включва класовете за управление на оформлението (sizers).
#include <wx/wrapsizer.h>       // Включва wxWrapSizer, който подрежда елементи и ги пренася на нов ред, ако е необходимо.
#include <wx/textctrl.h>        // Включва wxTextCtrl, контрола за текстово поле.
#include <wx/button.h>          // Включва wxButton, контрола за бутон.
#include <wx/listbox.h>         // Включва wxListBox, контрола за списък.
#include <wx/statbmp.h>         // Включва wxStaticBitmap, контрола за показване на статично изображение.
#include <wx/gauge.h>           // Включва wxGauge, контрола за прогрес бар.
#include <wx/mstream.h>         // Включва wxMemoryInputStream, поток за четене от паметта.
#include <wx/image.h>           // Включва wxImage, клас за работа с изображения.
#include <wx/thread.h>          // Включва wxThread, клас за работа с нишки.
#include <wx/stattext.h>        // Включва wxStaticText, контрола за статичен текст.
#include <wx/panel.h>           // Включва wxPanel, основен контейнер за контроли.
#include <wx/gbsizer.h>         // Включва wxGridBagSizer, гъвкав сайзър, базиран на мрежа.
#include <wx/progdlg.h>         // Включва wxProgressDialog, диалог за показване на напредък.
#include <wx/dcbuffer.h>        // Включва wxBufferedDC, за двойно буфериране при рисуване.
#include <wx/graphics.h>        // Включва wxGraphicsContext, за разширени възможности за 2D рисуване.
#include <wx/process.h>         // Включва wxProcess, за стартиране и управление на външни процеси.
#include <curl/curl.h>          // Включва библиотеката libcurl за HTTP заявки.
#include <rapidjson/document.h> // Включва rapidjson за парсване на JSON данни.
#include <rapidjson/error/en.h> // Включва rapidjson за съобщения за грешки при парсване на JSON.
#include <string>               // Включва стандартния клас std::string.
#include <iostream>             // Включва стандартния входно-изходен поток.
#include <vector>               // Включва стандартния контейнер std::vector.
#include <queue>                // Включва стандартния контейнер std::queue (опашка).
#include <mutex>                // Включва std::mutex за синхронизация на нишки.
#include <condition_variable>   // Включва std::condition_variable за синхронизация на нишки.
#include <thread>               // Включва std::thread за работа с нишки.
#include <functional>           // Включва std::function за обвиване на извикваеми обекти.
#include <sstream>              // Включва std::stringstream за работа с низови потоци.
#include <algorithm>            // Включва стандартни алгоритми.
#include <atomic>               // Включва std::atomic за атомарни операции.
#include <memory>               // Включва за умни указатели като std::unique_ptr.
#include <map>                  // Добавено за std::map (контейнер тип асоциативен масив).

// Forward declarations
// Предварителни декларации на класове
class AppCard;            // Предварителна декларация на клас AppCard.
class FlatpakStore;       // Предварителна декларация на клас FlatpakStore.
class SearchThread;       // Предварителна декларация на клас SearchThread.
class InitialLoadThread;  // Предварителна декларация на клас InitialLoadThread.
class InstallThread;      // Предварителна декларация на клас InstallThread.
class RoundedSearchPanel; // Предварителна декларация на клас RoundedSearchPanel.

// Custom event types
// Декларация на персонализирани типове събития
wxDECLARE_EVENT(wxEVT_SEARCH_COMPLETE, wxCommandEvent);  // Декларира тип събитие за завършване на търсене.
wxDECLARE_EVENT(wxEVT_RESULT_READY, wxCommandEvent);     // Декларира тип събитие, когато резултат е готов.
wxDECLARE_EVENT(wxEVT_IMAGE_READY, wxCommandEvent);      // Декларира тип събитие, когато изображение е готово.
wxDECLARE_EVENT(wxEVT_UPDATE_PROGRESS, wxCommandEvent);  // Декларира тип събитие за актуализиране на прогреса.
wxDECLARE_EVENT(wxEVT_BATCH_PROCESS, wxCommandEvent);    // Декларира тип събитие за пакетна обработка.
wxDECLARE_EVENT(INSTALL_START_EVENT, wxCommandEvent);    // Декларира тип събитие за начало на инсталация.
wxDECLARE_EVENT(INSTALL_COMPLETE_EVENT, wxCommandEvent); // Декларира тип събитие за завършване на инсталация.
wxDECLARE_EVENT(INSTALL_CANCEL_EVENT, wxCommandEvent);   // Декларира тип събитие за отмяна на инсталация.

// ThreadPool class declaration
// Декларация на клас ThreadPool (пул от нишки)
class ThreadPool
{
public:                         // Начало на публичната секция.
    ThreadPool(size_t threads); // Конструктор, приемащ броя на нишките.
    ~ThreadPool();              // Деструктор.

    template <class F>   // Шаблонен метод за добавяне на задача към опашката.
    void Enqueue(F &&f); // Приема извикваем обект (функция, ламбда и т.н.).

    void CancelAll(); // Метод за отмяна на всички задачи и спиране на нишките.

private:                                     // Начало на частната секция.
    std::vector<std::thread> workers;        // Вектор, съдържащ работните нишки.
    std::queue<std::function<void()>> tasks; // Опашка от задачи (функции, които да бъдат изпълнени).
    std::mutex queueMutex;                   // Мютекс за синхронизиране на достъпа до опашката със задачи.
    std::condition_variable condition;       // Условна променлива за сигнализиране на работните нишки.
    bool stop;                               // Флаг, указващ дали пулът от нишки трябва да спре.
};

// RoundedSearchPanel class declaration
// Декларация на клас RoundedSearchPanel (панел за търсене със заоблени ъгли)
class RoundedSearchPanel : public wxPanel // Наследява wxPanel.
{
public:                                                            // Начало на публичната секция.
    RoundedSearchPanel(wxWindow *parent, wxWindowID id = wxID_ANY, // Конструктор.
                       const wxPoint &pos = wxDefaultPosition,     // Позиция по подразбиране.
                       const wxSize &size = wxDefaultSize);        // Размер по подразбиране.

private:                               // Начало на частната секция.
    void OnPaint(wxPaintEvent &event); // Обработчик на събитие за прерисуване, за да се нарисуват заоблените ъгли.

    DECLARE_EVENT_TABLE() // Макрос за деклариране на таблицата със събития.
};

// AppCard class declaration
// Декларация на клас AppCard (карта, представяща приложение)
class AppCard : public wxPanel // Наследява wxPanel.
{
public:                                                                                              // Начало на публичната секция.
    AppCard(wxWindow *parent, const wxString &name, const wxString &summary, const wxString &appId); // Конструктор, приема родител, име, резюме и ID на приложението.
    virtual ~AppCard();                                                                              // Виртуален деструктор.

    void SetIcon(const wxBitmap &bitmap);                       // Метод за задаване на иконата на приложението.
    wxString GetAppId() const { return m_appId; }               // Метод за получаване на ID на приложението.
    wxString GetName() const { return m_nameText->GetLabel(); } // Метод за получаване на името на приложението.
    wxGauge *GetProgressGauge() { return m_progressGauge; }     // Метод за получаване на указател към прогрес бара.
    wxButton *GetCancelButton() { return m_cancelButton; }      // Метод за получаване на указател към бутона за отмяна.
    void ShowInstalling(bool show);                             // Метод за показване/скриване на елементите, свързани с инсталацията.
    void SetInstallButtonLabel(const wxString &label);          // Метод за задаване на текста на бутона за инсталиране.
    void DisableInstallButton();                                // Метод за деактивиране на бутона за инсталиране.

private:                          // Начало на частната секция.
    wxStaticBitmap *m_iconBitmap; // Указател към контролата за иконата.
    wxStaticText *m_nameText;     // Указател към контролата за името на приложението.
    wxStaticText *m_summaryText;  // Указател към контролата за резюмето на приложението.
    wxButton *m_installButton;    // Указател към бутона за инсталиране.
    wxGauge *m_progressGauge;     // Указател към прогрес бара за инсталация.
    wxButton *m_cancelButton;     // Указател към бутона за отмяна на инсталацията.
    wxBoxSizer *m_controlSizer;   // Сайзър за контролите (бутон за инсталиране, прогрес бар, бутон за отмяна).
    wxString m_appId;             // ID на приложението.
    bool m_isLoading;             // Флаг, указващ дали се зарежда иконата (изглежда неизползван тук).

    void OnPaint(wxPaintEvent &event);      // Обработчик на събитие за прерисуване (за рисуване на рамка или фон).
    void OnMouseEnter(wxMouseEvent &event); // Обработчик на събитие, когато мишката влезе в картата.
    void OnMouseLeave(wxMouseEvent &event); // Обработчик на събитие, когато мишката напусне картата.

    DECLARE_EVENT_TABLE() // Макрос за деклариране на таблицата със събития.
};

// SearchThread class declaration
// Декларация на клас SearchThread (нишка за извършване на търсене)
class SearchThread : public wxThread // Наследява wxThread.
{
public:                                                                                                    // Начало на публичната секция.
    SearchThread(FlatpakStore *store, const std::string &query, int searchId, std::atomic<bool> &stopFlag) // Конструктор.
        : wxThread(wxTHREAD_DETACHED), m_store(store), m_query(query),                                     // Инициализира базов клас за отделена нишка и членове.
          m_searchId(searchId), m_stopFlag(stopFlag)
    {
    } // Инициализира ID на търсене и флаг за спиране.

protected:                    // Начало на защитената секция.
    virtual ExitCode Entry(); // Основен метод на нишката, където се извършва работата.

private:                           // Начало на частната секция.
    FlatpakStore *m_store;         // Указател към главния обект FlatpakStore, за да може нишката да изпраща събития.
    std::string m_query;           // Низът за търсене.
    int m_searchId;                // Уникален идентификатор за текущото търсене.
    std::atomic<bool> &m_stopFlag; // Атомарен флаг за сигнализиране на нишката да спре.
};

// InitialLoadThread class declaration
// Декларация на клас InitialLoadThread (нишка за първоначално зареждане на приложения)
class InitialLoadThread : public wxThread // Наследява wxThread.
{
public:                                                                 // Начало на публичната секция.
    InitialLoadThread(FlatpakStore *store, std::atomic<bool> &stopFlag) // Конструктор.
        : wxThread(wxTHREAD_DETACHED), m_store(store), m_stopFlag(stopFlag)
    {
    } // Инициализира базов клас и членове.

protected:                    // Начало на защитената секция.
    virtual ExitCode Entry(); // Основен метод на нишката.

private:                           // Начало на частната секция.
    FlatpakStore *m_store;         // Указател към FlatpakStore.
    std::atomic<bool> &m_stopFlag; // Флаг за спиране.
};

// InstallState structure
// Структура InstallState (съхранява състоянието на текуща инсталация)
struct InstallState
{                                                            // Начало на дефиницията на структурата.
    AppCard *card;                                           // Указател към AppCard, за която се извършва инсталацията.
    std::atomic<bool> cancelFlag;                            // Атомарен флаг за отмяна на инсталацията.
    InstallState(AppCard *c) : card(c), cancelFlag(false) {} // Конструктор.
}; // Край на дефиницията на структурата.

// InstallThread class declaration
// Декларация на клас InstallThread (нишка за инсталиране на приложение)
class InstallThread : public wxThread // Наследява wxThread.
{
public:                                                                            // Начало на публичната секция.
    InstallThread(FlatpakStore *store, const wxString &appId, InstallState *state) // Конструктор.
        : wxThread(wxTHREAD_DETACHED), m_store(store), m_appId(appId), m_state(state)
    {
    } // Инициализира базов клас и членове.

protected:                             // Начало на защитената секция.
    virtual ExitCode Entry() override; // Основен метод на нишката, пренаписва базовия метод.

private:                   // Начало на частната секция.
    FlatpakStore *m_store; // Указател към FlatpakStore.
    wxString m_appId;      // ID на приложението за инсталиране.
    InstallState *m_state; // Указател към състоянието на инсталацията.
};

// FlatpakStore class declaration
// Декларация на клас FlatpakStore (основен клас за магазина за Flatpak приложения)
class FlatpakStore : public wxPanel // Наследява wxPanel.
{
public:                                                      // Начало на публичната секция.
    FlatpakStore(wxWindow *parent, const wxString &workDir); // Конструктор, приема родител и работна директория.
    virtual ~FlatpakStore();                                 // Виртуален деструктор.

    void SetTotalResults(int total) { totalResults = total; } // Метод за задаване на общия брой резултати от търсене.

    // Timer IDs
    // Идентификатори за таймери
    enum
    {                                      // Начало на енумерацията.
        ID_BATCH_TIMER = wxID_HIGHEST + 1, // ID за таймера за пакетна обработка.
        ID_LAYOUT_TIMER,                   // ID за таймера за пренареждане на оформлението.
        ID_SHOW_INSTALLED_BUTTON           // Добавяне на нов ID за бутон.
    }; // Край на енумерацията.

private: // Начало на частната секция.
    // UI elements
    // UI елементи
    wxTextCtrl *m_searchBox;          // Текстово поле за търсене.
    wxButton *m_searchButton;         // Бутон за стартиране на търсене.
    wxScrolledWindow *m_resultsPanel; // Панел със скрол за показване на резултатите (AppCard).
    wxBoxSizer *m_mainSizer;          // Основен сайзър за панела.
    wxGauge *m_progressBar;           // Прогрес бар (може би за първоначално зареждане или общ прогрес).
    wxWrapSizer *m_gridSizer;         // Сайзър за подреждане на AppCard-овете в мрежа с пренасяне.
    wxPanel *m_progressPanel;         // Панел за показване на прогрес и текст.
    wxStaticText *m_progressText;     // Текст, показващ информация за прогреса.
    wxTimer *m_layoutTimer;           // Таймер за отложено пренареждане на оформлението.
    wxTimer *m_batchTimer;            // Таймер за пакетна обработка на резултати.
    wxButton *m_showInstalledButton;  // Декларация на нов бутон.

    // Batch processing members
    // Членове за пакетна обработка
    std::vector<wxString> m_pendingResults; // Вектор със заявки за резултати, чакащи обработка.
    std::mutex m_resultsMutex;              // Мютекс за синхронизиране на достъпа до m_pendingResults.
    int m_pendingBatchSize;                 // Размер на текущата партида за обработка.

    // Search state
    // Състояние на търсенето
    bool m_isSearching;           // Флаг, указващ дали в момента се извършва търсене.
    bool m_isInitialLoading;      // Флаг, указващ дали се извършва първоначално зареждане.
    int totalResults;             // Общ брой намерени резултати.
    int m_displayedResults;       // Брой на показаните в момента резултати.
    std::mutex m_searchMutex;     // Мютекс за синхронизиране на операциите по търсене.
    std::atomic<bool> m_stopFlag; // Атомарен флаг за спиране на текущата операция (търсене/зареждане).
    int m_searchId;               // Уникален идентификатор за текущата операция по търсене.
    wxString m_workDir;           // Работна директория.

    // Thread pool
    // Пул от нишки
    std::unique_ptr<ThreadPool> m_threadPool; // Умен указател към обекта ThreadPool.

    // Data structures
    // Структури от данни
    struct IconData      // Структура за съхранение на данни за икона.
    {                    // Начало на дефиницията на структурата.
        AppCard *card;   // Указател към AppCard, за която е иконата.
        wxBitmap bitmap; // Самата икона като wxBitmap.
    }; // Край на дефиницията на структурата.

    // Event handlers
    // Обработчици на събития
    void OnSearch(wxCommandEvent &event);                     // Обработчик за стартиране на търсене (вероятно от Enter в текстовото поле).
    void OnSearchComplete(wxCommandEvent &event);             // Обработчик за събитието wxEVT_SEARCH_COMPLETE.
    void OnResultReady(wxCommandEvent &event);                // Обработчик за събитието wxEVT_RESULT_READY.
    void OnImageReady(wxCommandEvent &event);                 // Обработчик за събитието wxEVT_IMAGE_READY.
    void OnUpdateProgress(wxCommandEvent &event);             // Обработчик за събитието wxEVT_UPDATE_PROGRESS.
    void OnInstallButtonClicked(wxCommandEvent &event);       // Обработчик за натискане на бутона за инсталиране в AppCard.
    void OnInstallComplete(wxCommandEvent &event);            // Обработчик за събитието INSTALL_COMPLETE_EVENT.
    void OnInstallCancel(wxCommandEvent &event);              // Обработчик за събитието INSTALL_CANCEL_EVENT.
    void OnLayoutTimer(wxTimerEvent &event);                  // Обработчик за събитието на таймера за оформление.
    void OnBatchTimer(wxTimerEvent &event);                   // Обработчик за събитието на таймера за пакетна обработка.
    void OnShowInstalledButtonClicked(wxCommandEvent &event); // Декларация на нов обработчик на събитие.
    void OnSearchButtonClicked(wxCommandEvent &event);        // Декларация на нов обработчик на събитие.

    // Helper methods
    // Помощни методи
    void ClearResults();                       // Метод за изчистване на текущите резултати от търсене.
    void LoadInitialApps();                    // Метод за зареждане на първоначалния списък с приложения.
    void ProcessPendingBatch(bool processAll); // Метод за обработка на чакащата партида резултати.

    DECLARE_EVENT_TABLE() // Макрос за деклариране на таблицата със събития.
};

// ThreadPool template method implementation
// Реализация на шаблонен метод на ThreadPool
template <class F>                                                     // Шаблон за тип F.
void ThreadPool::Enqueue(F &&f)                                        // Метод за добавяне на задача.
{                                                                      // Начало на тялото на метода.
    {                                                                  // Начало на блок за заключване.
        std::unique_lock<std::mutex> lock(queueMutex);                 // Заключва мютекса за достъп до опашката.
        if (stop)                                                      // Проверява дали пулът е спрян.
            throw std::runtime_error("Enqueue on stopped ThreadPool"); // Хвърля изключение, ако се опитва да се добави задача към спрян пул.
        tasks.emplace(std::forward<F>(f));                             // Добавя задачата в опашката (използва perfect forwarding).
    } // Край на блок за заключване (мютексът се освобождава).
    condition.notify_one(); // Уведомява една от чакащите работни нишки, че има нова задача.
} // Край на тялото на метода.

#endif // FLATPAKSTORE_H // Край на условния блок за FLATPAKSTORE_H.