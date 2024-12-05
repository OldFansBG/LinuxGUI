#ifndef SECONDWINDOW_H
#define SECONDWINDOW_H

#include <wx/wx.h>
#include "OSDetector.h"
#include "WindowsCmdPanel.h"
#include "LinuxTerminalPanel.h"

class SecondWindow : public wxFrame {
public:
    SecondWindow(wxWindow* parent, const wxString& title, const wxString& isoPath);

private:
    void OnClose(wxCloseEvent& event);

    OSDetector m_osDetector;
    WindowsCmdPanel* m_cmdPanel;
    LinuxTerminalPanel* m_terminalPanel;
    wxString m_isoPath;

    wxDECLARE_EVENT_TABLE();
};

#endif // SECONDWINDOW_H