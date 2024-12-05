#ifndef WINDOWSCMDPANEL_H
#define WINDOWSCMDPANEL_H

#include <wx/wx.h>
#include <wx/process.h>
#include "DockerTransfer.h"

#ifdef __WINDOWS__
#include <windows.h>
#endif

class WindowsCmdPanel : public wxPanel {
public:
    WindowsCmdPanel(wxWindow* parent);
    virtual ~WindowsCmdPanel();
    void SetISOPath(const wxString& path); // Removed inline implementation

private:
    void CreateCmdWindow();
    void OnSize(wxSizeEvent& event);
    void OnDockerProgress(int progress, const wxString& status);

#ifdef __WINDOWS__
    HWND m_hwndCmd;
#endif
    wxString m_isoPath;
    std::unique_ptr<DockerTransfer> m_dockerTransfer;

    wxDECLARE_EVENT_TABLE();
};

#endif // WINDOWSCMDPANEL_H