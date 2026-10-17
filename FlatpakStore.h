#ifndef FLATPAKSTORE_H // Предпазител от многократно включване на хедър файла
#define FLATPAKSTORE_H // Дефиниране на предпазителя

#include <wx/wx.h>             // Основни wxWidgets класове
#include <wx/wrapsizer.h>      // wxWrapSizer за решетката с карти
#include <wx/thread.h>         // wxThread за търсенето във Flathub
#include <wx/dcbuffer.h>       // wxAutoBufferedPaintDC
#include <wx/graphics.h>       // wxGraphicsContext за заоблените ъгли
#include <string>              // std::string
#include <vector>              // std::vector
#include <queue>               // std::queue за задачите на ThreadPool
#include <mutex>               // std::mutex
#include <condition_variable>  // std::condition_variable
#include <thread>              // std::thread
#include <functional>          // std::function
#include <atomic>              // std::atomic
#include <memory>              // std::shared_ptr / std::unique_ptr

class AppCard;      // Карта на едно приложение
class FlatpakStore; // Панелът с магазина

// Събития, които работните нишки изпращат към FlatpakStore
wxDECLARE_EVENT(wxEVT_SEARCH_COMPLETE, wxCommandEvent); // Търсенето приключи
wxDECLARE_EVENT(wxEVT_RESULT_READY, wxCommandEvent);    // Намерено е едно приложение
wxDECLARE_EVENT(wxEVT_IMAGE_READY, wxCommandEvent);     // Иконата на приложение е изтеглена

// Споделено състояние между магазина и работните нишки. Нишките го държат чрез shared_ptr,
// така че никога не изпращат събития към вече унищожен FlatpakStore.
struct StoreLifetime
{
    std::mutex mutex;  // Пази alive и изпращането на събития
    bool alive = true; // false, след като магазинът е унищожен
};

// Пул от нишки за изтегляне на иконите
class ThreadPool
{
public:
    explicit ThreadPool(size_t threads);
    ~ThreadPool();

    template <class F>
    void Enqueue(F &&f);

    void CancelAll(); // Премахва задачите, които още не са започнали

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queueMutex;
    std::condition_variable condition;
    bool stop;
};

// Панел със заоблени ъгли за полето за търсене
class RoundedSearchPanel : public wxPanel
{
public:
    RoundedSearchPanel(wxWindow *parent, wxWindowID id = wxID_ANY,
                       const wxPoint &pos = wxDefaultPosition,
                       const wxSize &size = wxDefaultSize);

private:
    void OnPaint(wxPaintEvent &event);

    wxDECLARE_EVENT_TABLE();
};

// Карта на едно приложение: икона, име, описание и бутон Install
class AppCard : public wxPanel
{
public:
    AppCard(wxWindow *parent, const wxString &name, const wxString &summary, const wxString &appId);

    void SetIcon(const wxBitmap &bitmap);
    wxString GetAppId() const { return m_appId; }
    wxString GetName() const { return m_nameText->GetLabel(); }
    void ShowInstalling(bool show);                // Показва индикатор, докато flatpak install работи
    void SetInstallButtonLabel(const wxString &label);
    void DisableInstallButton();

private:
    wxStaticBitmap *m_iconBitmap;
    wxStaticText *m_nameText;
    wxStaticText *m_summaryText;
    wxButton *m_installButton;
    wxGauge *m_progressGauge;
    wxBoxSizer *m_controlSizer;
    wxString m_appId;

    void OnPaint(wxPaintEvent &event);
    void OnMouseEnter(wxMouseEvent &event);
    void OnMouseLeave(wxMouseEvent &event);

    wxDECLARE_EVENT_TABLE();
};

// Нишка, която търси във Flathub (празна заявка = наскоро добавени приложения)
class SearchThread : public wxThread
{
public:
    SearchThread(FlatpakStore *store, std::shared_ptr<StoreLifetime> lifetime,
                 std::shared_ptr<std::atomic<bool>> stopFlag, const std::string &query, int searchId)
        : wxThread(wxTHREAD_DETACHED), m_store(store), m_lifetime(std::move(lifetime)),
          m_stopFlag(std::move(stopFlag)), m_query(query), m_searchId(searchId)
    {
    }

protected:
    virtual ExitCode Entry() override;

private:
    FlatpakStore *m_store;                          // Използва се само за wxQueueEvent, докато lifetime->alive е true
    std::shared_ptr<StoreLifetime> m_lifetime;      // Споделеното състояние с магазина
    std::shared_ptr<std::atomic<bool>> m_stopFlag;  // true, когато е започнало ново търсене
    std::string m_query;                            // Текст за търсене (UTF-8)
    int m_searchId;                                 // ID на търсенето, за да се игнорират стари резултати
};

// Магазин за Flatpak приложения: търси във Flathub и ги инсталира в ISO образа
class FlatpakStore : public wxPanel
{
public:
    FlatpakStore(wxWindow *parent, const wxString &workDir);
    virtual ~FlatpakStore();

    // Изпраща събитие от работна нишка, освен ако магазинът вече е унищожен (поема собствеността върху event)
    static bool PostFromThread(const std::shared_ptr<StoreLifetime> &lifetime, FlatpakStore *store, wxEvent *event);

private:
    enum
    {
        ID_BATCH_TIMER = wxID_HIGHEST + 1,
        ID_LAYOUT_TIMER
    };

    // --- Контроли ---
    wxTextCtrl *m_searchBox;
    wxButton *m_searchButton;
    wxButton *m_showInstalledButton;
    wxScrolledWindow *m_resultsPanel;
    wxBoxSizer *m_mainSizer;
    wxWrapSizer *m_gridSizer;
    wxPanel *m_progressPanel;
    wxStaticText *m_progressText;
    wxGauge *m_progressBar;
    wxTimer *m_layoutTimer;
    wxTimer *m_batchTimer;

    // --- Състояние (използва се само от главната нишка) ---
    std::vector<wxString> m_pendingResults; // Резултати, които още не са показани като карти
    int m_displayedResults;                 // Брой получени резултати за текущото търсене
    int m_searchId;                         // ID на текущото търсене
    wxString m_workDir;                     // Директорията на проекта (съдържа container_id.txt)

    std::shared_ptr<StoreLifetime> m_lifetime;         // Споделено с работните нишки
    std::shared_ptr<std::atomic<bool>> m_searchStop;   // Флаг за спиране на текущото търсене и иконите му
    std::unique_ptr<ThreadPool> m_threadPool;          // Нишки за изтегляне на икони

    struct IconData
    {
        AppCard *card; // Картата, за която е иконата (може вече да е унищожена)
        wxImage image; // wxImage се използва в нишките; wxBitmap се създава в главната нишка
    };

    // --- Обработчици на събития ---
    void OnSearch(wxCommandEvent &event);
    void OnSearchComplete(wxCommandEvent &event);
    void OnResultReady(wxCommandEvent &event);
    void OnImageReady(wxCommandEvent &event);
    void OnInstallButtonClicked(wxCommandEvent &event);
    void OnLayoutTimer(wxTimerEvent &event);
    void OnBatchTimer(wxTimerEvent &event);
    void OnShowInstalledButtonClicked(wxCommandEvent &event);

    // --- Помощни методи ---
    void StartSearch(const wxString &query); // Празна заявка = наскоро добавени
    void StopCurrentSearch();                // Спира текущото търсене и започва ново "поколение" резултати
    void ClearResults();
    void ShowMessage(const wxString &message); // Показва текст в областта с резултати
    void ProcessPendingBatch(bool processAll);
    AppCard *AddCard(const wxString &name, const wxString &summary, const wxString &iconUrl, const wxString &appId);
    void LoadIconAsync(AppCard *card, const wxString &iconUrl);

    wxDECLARE_EVENT_TABLE();
};

template <class F>
void ThreadPool::Enqueue(F &&f)
{
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        if (stop)
            return; // Пулът се унищожава, задачата се игнорира
        tasks.emplace(std::forward<F>(f));
    }
    condition.notify_one();
}

#endif // FLATPAKSTORE_H
