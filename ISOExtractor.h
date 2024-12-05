#ifndef EXTRACTOR_H
#define EXTRACTOR_H

#include <wx/wx.h>
#include <wx/thread.h>
#include <string>
#include <functional>
#include <atomic>

class ISOExtractor : public wxThread {
public:
    using ProgressCallback = std::function<void(int, const wxString&)>;
    
    ISOExtractor(const wxString& isoPath, 
                 const wxString& destPath,
                 ProgressCallback progressCallback);
                 
    virtual ExitCode Entry() wxOVERRIDE;
    void RequestStop() { m_stopRequested = true; }
    
private:
    bool ExtractTo(const std::string& destPath);
    wxString m_isoPath;
    wxString m_destPath;
    ProgressCallback m_progressCallback;
    std::atomic<bool> m_stopRequested;
};

#endif // EXTRACTOR_H