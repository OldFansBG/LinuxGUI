#ifndef WINDOWSCMDPANEL_H
#define WINDOWSCMDPANEL_H

#include <memory>
#include <wx/wx.h>
#include <wx/process.h>
#include "DockerTransfer.h"
#include "ScriptManager.h"
#include <iostream>
#include "CustomEvents.h" // Include the shared header

#ifdef __WINDOWS__
#include <windows.h>
#endif

// Forward declaration for the timer class
class InitTimer;

class WindowsCmdPanel : public wxPanel {
public:
   WindowsCmdPanel(wxWindow* parent);
   virtual ~WindowsCmdPanel();
   void SetISOPath(const wxString& path);
   void ContinueInitialization();
   wxString GetFileSizeString(const wxString& filePath); // Add this line
   void ShowCompletionDialog(const wxString& isoPath);

private:
   void CreateCmdWindow();
   void OnSize(wxSizeEvent& event);
   void CleanupTimer();
   bool m_step6Completed; // Add this line
   bool ExecuteHiddenCommand(const wxString& cmd, wxArrayString* output = nullptr);
   bool ExecuteDockerCommand(const wxString& cmd, wxArrayString* output = nullptr);
   bool WaitForISOToBeCopied(const wxString& containerId);  // Add this line
   bool CheckISOExistsInContainer(const wxString& containerId);  // Add this line

#ifdef __WINDOWS__
   HWND m_hwndCmd;
#endif
   wxString m_isoPath;
   wxString m_containerId;
   std::unique_ptr<DockerTransfer> m_dockerTransfer;
   InitTimer* m_initTimer;
   int m_initStep;
   
   friend class InitTimer;

   wxDECLARE_EVENT_TABLE();
};

// Timer class for handling initialization sequence
class InitTimer : public wxTimer {
public:
    InitTimer(WindowsCmdPanel* panel) : m_panel(panel) {}
    void Notify() override {
        m_panel->ContinueInitialization();
    }
private:
    WindowsCmdPanel* m_panel;
};

#endif // WINDOWSCMDPANEL_H