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

// Forward declarations
class AppCard;
class FlatpakStore;
class SearchThread;
class InitialLoadThread;
class RoundedSearchPanel;

// Custom event types
wxDECLARE_EVENT(wxEVT_SEARCH_COMPLETE, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_RESULT_READY, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_IMAGE_READY, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_UPDATE_PROGRESS, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_BATCH_PROCESS, wxCommandEvent);

// ThreadPool class declaration
class ThreadPool {
public:
    ThreadPool(size_t threads);
    ~ThreadPool();

    template<class F>
    void Enqueue(F&& f);

    void CancelAll();

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queueMutex;
    std::condition_variable condition;
    bool stop;
};

// RoundedSearchPanel class declaration
class RoundedSearchPanel : public wxPanel {
public:
    RoundedSearchPanel(wxWindow* parent, wxWindowID id = wxID_ANY,
                     const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize);

private:
    void OnPaint(wxPaintEvent& event);
    
    DECLARE_EVENT_TABLE()
};

// FlatpakStore class declaration
class FlatpakStore : public wxPanel {
public:
    FlatpakStore(wxWindow* parent, const wxString& workDir); // Updated constructor
    virtual ~FlatpakStore();

    void SetTotalResults(int total) { totalResults = total; }
    void SetContainerId(const wxString& containerId);
    
    // Timer IDs
    enum {
        ID_BATCH_TIMER = wxID_HIGHEST + 1,
        ID_LAYOUT_TIMER
    };

private:
    // UI elements
    wxTextCtrl* m_searchBox;
    wxButton* m_searchButton;
    wxScrolledWindow* m_resultsPanel;
    wxBoxSizer* m_mainSizer;
    wxGauge* m_progressBar;
    wxWrapSizer* m_gridSizer;
    wxPanel* m_progressPanel;
    wxStaticText* m_progressText;
    wxTimer* m_layoutTimer;    // Timer for periodic layout updates
    wxTimer* m_batchTimer;     // Timer for batch processing
    
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
    wxString m_workDir; // Added to store the working directory

    // Thread pool
    std::unique_ptr<ThreadPool> m_threadPool;

    // Data structures
    struct IconData {
        AppCard* card;
        wxBitmap bitmap;
    };

    // Event handlers
    void OnSearch(wxCommandEvent& event);
    void OnSearchComplete(wxCommandEvent& event);
    void OnResultReady(wxCommandEvent& event);
    void OnImageReady(wxCommandEvent& event);
    void OnUpdateProgress(wxCommandEvent& event);
    void OnInstallButtonClicked(wxCommandEvent& event);
    void OnLayoutTimer(wxTimerEvent& event);  // Timer event
    void OnBatchTimer(wxTimerEvent& event);   // Batch timer event

    // Helper methods
    void HandleInstallClick(const wxString& appId);
    void ClearResults();
    void LoadInitialApps();
    void ProcessPendingBatch(bool processAll);

    DECLARE_EVENT_TABLE()
};

// AppCard class declaration
class AppCard : public wxPanel {
public:
    AppCard(wxWindow* parent, const wxString& name, const wxString& summary, const wxString& appId);
    virtual ~AppCard();

    void SetIcon(const wxBitmap& bitmap);
    wxString GetAppId() const { return m_appId; }

private:
    wxStaticBitmap* m_iconBitmap;
    wxStaticText* m_nameText;
    wxStaticText* m_summaryText;
    wxButton* m_installButton;
    wxString m_appId;
    bool m_isLoading;

    void OnPaint(wxPaintEvent& event);
    void OnMouseEnter(wxMouseEvent& event);
    void OnMouseLeave(wxMouseEvent& event);

    DECLARE_EVENT_TABLE()
};

// SearchThread class declaration
class SearchThread : public wxThread {
public:
    SearchThread(FlatpakStore* store, const std::string& query, int searchId, std::atomic<bool>& stopFlag)
        : wxThread(wxTHREAD_DETACHED), m_store(store), m_query(query),
          m_searchId(searchId), m_stopFlag(stopFlag) {}

protected:
    virtual ExitCode Entry();

private:
    FlatpakStore* m_store;
    std::string m_query;
    int m_searchId;
    std::atomic<bool>& m_stopFlag;
};

// InitialLoadThread class declaration
class InitialLoadThread : public wxThread {
public:
    InitialLoadThread(FlatpakStore* store, std::atomic<bool>& stopFlag)
        : wxThread(wxTHREAD_DETACHED), m_store(store), m_stopFlag(stopFlag) {}

protected:
    virtual ExitCode Entry();

private:
    FlatpakStore* m_store;
    std::atomic<bool>& m_stopFlag;
};

// ThreadPool template method implementation
template<class F>
void ThreadPool::Enqueue(F&& f) {
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        if (stop) throw std::runtime_error("Enqueue on stopped ThreadPool");
        tasks.emplace(std::forward<F>(f));
    }
    condition.notify_one();
}

#endif // FLATPAKSTORE_H