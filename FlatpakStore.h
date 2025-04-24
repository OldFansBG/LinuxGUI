#ifndef FLATPAKSTORE_H
#define FLATPAKSTORE_H

#include <wx/wx.h>
#include <wx/sizer.h>
#include <wx/wrapsizer.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/listbox.h>
#include <wx/statbmp.h>
#include <wx/gauge.h>
#include <wx/mstream.h>
#include <wx/image.h>
#include <wx/thread.h>
#include <wx/stattext.h>
#include <wx/panel.h>
#include <wx/gbsizer.h>
#include <wx/progdlg.h>
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/process.h>
#include <curl/curl.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <string>
#include <iostream>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>
#include <sstream>
#include <algorithm>
#include <atomic>
#include <memory>
#include <map> // Added for std::map

// Forward declarations
class AppCard;
class FlatpakStore;
class SearchThread;
class InitialLoadThread;
class InstallThread;
class RoundedSearchPanel;

// Custom event types
wxDECLARE_EVENT(wxEVT_SEARCH_COMPLETE, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_RESULT_READY, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_IMAGE_READY, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_UPDATE_PROGRESS, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_BATCH_PROCESS, wxCommandEvent);
wxDECLARE_EVENT(INSTALL_START_EVENT, wxCommandEvent);
wxDECLARE_EVENT(INSTALL_COMPLETE_EVENT, wxCommandEvent);
wxDECLARE_EVENT(INSTALL_CANCEL_EVENT, wxCommandEvent);

// ThreadPool class declaration
class ThreadPool
{
public:
    ThreadPool(size_t threads);
    ~ThreadPool();

    template <class F>
    void Enqueue(F &&f);

    void CancelAll();

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queueMutex;
    std::condition_variable condition;
    bool stop;
};

// RoundedSearchPanel class declaration
class RoundedSearchPanel : public wxPanel
{
public:
    RoundedSearchPanel(wxWindow *parent, wxWindowID id = wxID_ANY,
                       const wxPoint &pos = wxDefaultPosition,
                       const wxSize &size = wxDefaultSize);

private:
    void OnPaint(wxPaintEvent &event);

    DECLARE_EVENT_TABLE()
};

// AppCard class declaration
class AppCard : public wxPanel
{
public:
    AppCard(wxWindow *parent, const wxString &name, const wxString &summary, const wxString &appId);
    virtual ~AppCard();

    void SetIcon(const wxBitmap &bitmap);
    wxString GetAppId() const { return m_appId; }
    wxString GetName() const { return m_nameText->GetLabel(); }
    wxGauge *GetProgressGauge() { return m_progressGauge; }
    wxButton *GetCancelButton() { return m_cancelButton; }
    void ShowInstalling(bool show);
    void SetInstallButtonLabel(const wxString &label);
    void DisableInstallButton();

private:
    wxStaticBitmap *m_iconBitmap;
    wxStaticText *m_nameText;
    wxStaticText *m_summaryText;
    wxButton *m_installButton;
    wxGauge *m_progressGauge;
    wxButton *m_cancelButton;
    wxBoxSizer *m_controlSizer;
    wxString m_appId;
    bool m_isLoading;

    void OnPaint(wxPaintEvent &event);
    void OnMouseEnter(wxMouseEvent &event);
    void OnMouseLeave(wxMouseEvent &event);

    DECLARE_EVENT_TABLE()
};

// SearchThread class declaration
class SearchThread : public wxThread
{
public:
    SearchThread(FlatpakStore *store, const std::string &query, int searchId, std::atomic<bool> &stopFlag)
        : wxThread(wxTHREAD_DETACHED), m_store(store), m_query(query),
          m_searchId(searchId), m_stopFlag(stopFlag) {}

protected:
    virtual ExitCode Entry();

private:
    FlatpakStore *m_store;
    std::string m_query;
    int m_searchId;
    std::atomic<bool> &m_stopFlag;
};

// InitialLoadThread class declaration
class InitialLoadThread : public wxThread
{
public:
    InitialLoadThread(FlatpakStore *store, std::atomic<bool> &stopFlag)
        : wxThread(wxTHREAD_DETACHED), m_store(store), m_stopFlag(stopFlag) {}

protected:
    virtual ExitCode Entry();

private:
    FlatpakStore *m_store;
    std::atomic<bool> &m_stopFlag;
};

// InstallState structure
struct InstallState
{
    AppCard *card;
    std::atomic<bool> cancelFlag;
    wxProcess *process;
    long pid;
    InstallState(AppCard *c) : card(c), cancelFlag(false), process(nullptr), pid(0) {}
};

// InstallThread class declaration
class InstallThread : public wxThread
{
public:
    InstallThread(FlatpakStore *store, const wxString &appId, InstallState *state)
        : wxThread(wxTHREAD_DETACHED), m_store(store), m_appId(appId), m_state(state) {}

protected:
    virtual ExitCode Entry() override;

private:
    FlatpakStore *m_store;
    wxString m_appId;
    InstallState *m_state;
};

// FlatpakStore class declaration
class FlatpakStore : public wxPanel
{
public:
    FlatpakStore(wxWindow *parent, const wxString &workDir);
    virtual ~FlatpakStore();

    void SetTotalResults(int total) { totalResults = total; }
    void SetContainerId(const wxString &containerId);
    wxString GetContainerId() const;

    // Timer IDs
    enum
    {
        ID_BATCH_TIMER = wxID_HIGHEST + 1,
        ID_LAYOUT_TIMER,
        ID_SHOW_INSTALLED_BUTTON // Add new button ID
    };

private:
    // UI elements
    wxTextCtrl *m_searchBox;
    wxButton *m_searchButton;
    wxScrolledWindow *m_resultsPanel;
    wxBoxSizer *m_mainSizer;
    wxGauge *m_progressBar;
    wxWrapSizer *m_gridSizer;
    wxPanel *m_progressPanel;
    wxStaticText *m_progressText;
    wxTimer *m_layoutTimer;
    wxTimer *m_batchTimer;
    wxButton *m_showInstalledButton; // Add new button declaration

    // Batch processing members
    std::vector<wxString> m_pendingResults;
    std::mutex m_resultsMutex;
    int m_pendingBatchSize;

    // Search state
    bool m_isSearching;
    bool m_isInitialLoading;
    int totalResults;
    int m_displayedResults;
    std::mutex m_searchMutex;
    std::atomic<bool> m_stopFlag;
    int m_searchId;
    wxString m_containerId;
    wxString m_workDir;

    // Thread pool
    std::unique_ptr<ThreadPool> m_threadPool;

    // Data structures
    struct IconData
    {
        AppCard *card;
        wxBitmap bitmap;
    };

    // Active installations tracking
    std::map<long, InstallState *> m_activeInstalls; // Added declaration

    // Event handlers
    void OnSearch(wxCommandEvent &event);
    void OnSearchComplete(wxCommandEvent &event);
    void OnResultReady(wxCommandEvent &event);
    void OnImageReady(wxCommandEvent &event);
    void OnUpdateProgress(wxCommandEvent &event);
    void OnInstallButtonClicked(wxCommandEvent &event);
    void OnInstallStart(wxCommandEvent &event);
    void OnInstallComplete(wxCommandEvent &event);
    void OnInstallCancel(wxCommandEvent &event);
    void OnProcessTerminated(wxProcessEvent &event);
    void OnLayoutTimer(wxTimerEvent &event);
    void OnBatchTimer(wxTimerEvent &event);
    void OnShowInstalledButtonClicked(wxCommandEvent &event); // Add new event handler declaration
    void OnSearchButtonClicked(wxCommandEvent &event);        // Add new event handler declaration

    // Helper methods
    void ClearResults();
    void LoadInitialApps();
    void ProcessPendingBatch(bool processAll);
    void SaveInstallationPreferences(const wxString &appId, const wxString &command, const wxString &appName); // Updated method declaration

    DECLARE_EVENT_TABLE()
};

// ThreadPool template method implementation
template <class F>
void ThreadPool::Enqueue(F &&f)
{
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        if (stop)
            throw std::runtime_error("Enqueue on stopped ThreadPool");
        tasks.emplace(std::forward<F>(f));
    }
    condition.notify_one();
}

#endif // FLATPAKSTORE_H