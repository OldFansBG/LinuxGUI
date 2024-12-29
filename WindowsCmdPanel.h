#ifndef WINDOWSCMDPANEL_H
#define WINDOWSCMDPANEL_H

#include <wx/wx.h>
#include <wx/process.h>
#include "DockerTransfer.h"
#include "ScriptManager.h"
#ifdef __WINDOWS__
#include <windows.h>
#endif

class WindowsCmdPanel : public wxPanel {
public:
   WindowsCmdPanel(wxWindow* parent);
   virtual ~WindowsCmdPanel();
   void SetISOPath(const wxString& path);

private:
   void CreateCmdWindow();
   void OnSize(wxSizeEvent& event);

#ifdef __WINDOWS__
   HWND m_hwndCmd;
#endif
   wxString m_isoPath;
   wxString m_containerId;
   std::unique_ptr<DockerTransfer> m_dockerTransfer;

   wxDECLARE_EVENT_TABLE();
};

#endif // WINDOWSCMDPANEL_H