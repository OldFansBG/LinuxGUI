#ifndef FLATPAKSTORE_H
#define FLATPAKSTORE_H

#include <wx/wx.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/listbox.h>
#include <wx/statbmp.h>
#include <wx/gauge.h>
#include <wx/mstream.h>
#include <wx/image.h>
#include <wx/thread.h>
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

// Define custom events
wxDECLARE_EVENT(wxEVT_SEARCH_COMPLETE, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_RESULT_READY, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_IMAGE_READY, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_UPDATE_PROGRESS, wxCommandEvent);

// ThreadPool class definition
class ThreadPool {
public:
    // Constructor: Initializes the thread pool with a given number of threads
    ThreadPool(size_t threads) : stop(false) {
        for (size_t i = 0; i < threads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;

                    {
                        // Lock the queue to safely access tasks
                        std::unique_lock<std::mutex> lock(queueMutex);

                        // Wait until there is a task to execute or the pool is stopped
                        condition.wait(lock, [this] { return stop || !tasks.empty(); });

                        // If the pool is stopped and no tasks are left, exit the thread
                        if (stop && tasks.empty()) {
                            return;
                        }

                        // Get the next task from the queue
                        task = std::move(tasks.front());
                        tasks.pop();
                    }

                    // Execute the task
                    task();
                }
            });
        }
    }

    // Enqueue a new task to be executed by the thread pool
    template<class F>
    void Enqueue(F&& f) {
        {
            // Lock the queue to safely add a new task
            std::unique_lock<std::mutex> lock(queueMutex);

            // Do not allow new tasks if the pool is stopped
            if (stop) {
                throw std::runtime_error("Enqueue on stopped ThreadPool");
            }

            // Add the task to the queue
            tasks.emplace(std::forward<F>(f));
        }

        // Notify one thread that a new task is available
        condition.notify_one();
    }

    // Cancel all pending tasks in the queue
    void CancelAll() {
        std::unique_lock<std::mutex> lock(queueMutex);
        while (!tasks.empty()) {
            tasks.pop();
        }
    }

    // Destructor: Stops the thread pool and joins all worker threads
    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            stop = true; // Signal all threads to stop
        }

        // Notify all threads to wake up and check the stop condition
        condition.notify_all();

        // Join all worker threads to ensure they finish execution
        for (std::thread& worker : workers) {
            worker.join();
        }
    }

private:
    std::vector<std::thread> workers; // Worker threads
    std::queue<std::function<void()>> tasks; // Task queue
    std::mutex queueMutex; // Mutex to protect the task queue
    std::condition_variable condition; // Condition variable for task notification
    bool stop; // Flag to stop the thread pool
};

// Main store class
class FlatpakStore : public wxPanel {
public:
    FlatpakStore(wxWindow* parent);
    virtual ~FlatpakStore();

    // Public methods
    void SetTotalResults(int total) { totalResults = total; }

private:
    // UI elements
    wxTextCtrl* searchBox;
    wxButton* searchButton;
    wxScrolledWindow* resultsPanel;
    wxBoxSizer* resultsSizer;
    wxGauge* progressBar;

    // Search state
    bool isSearching;
    int totalResults;
    wxThread* currentSearchThread;
    std::mutex searchMutex;
    std::atomic<bool> stopFlag;
    int searchId;

    // Thread pool
    std::unique_ptr<ThreadPool> threadPool;

    // Event handlers
    void OnSearch(wxCommandEvent& event);
    void OnSearchComplete(wxCommandEvent& event);
    void OnResultReady(wxCommandEvent& event);
    void OnImageReady(wxCommandEvent& event);
    void OnUpdateProgress(wxCommandEvent& event);

    DECLARE_EVENT_TABLE()
};

#endif // FLATPAKSTORE_H