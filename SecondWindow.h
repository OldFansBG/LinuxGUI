#ifndef SECONDWINDOW_H
#define SECONDWINDOW_H

#include <wx/wx.h>
#include <wx/statline.h>
#include <wx/notebook.h>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/wfstream.h>
#include <wx/progdlg.h>
#include <wx/stdpaths.h>
#include <vector>
#include "OSDetector.h"
#include "WindowsCmdPanel.h"
#include "LinuxTerminalPanel.h"
#include "ContainerManager.h"
#include "SQLTab.h"

class SecondWindow : public wxFrame {
public:
    SecondWindow(wxWindow* parent, const wxString& title, const wxString& isoPath);
    ~SecondWindow();

private:
    void CreateControls();
    void OnClose(wxCloseEvent& event);
    void OnNext(wxCommandEvent& event);
    void OnTabChanged(wxCommandEvent& event);
    void OnPythonWorkComplete(wxCommandEvent& event);  // Add event handler

    OSDetector m_osDetector;
    WindowsCmdPanel* m_cmdPanel;
    LinuxTerminalPanel* m_terminalPanel;
    wxString m_isoPath;
    wxPanel* m_mainPanel;
    wxPanel* m_terminalTab;
    SQLTab* m_sqlTab;
    wxSizer* m_terminalSizer;
    wxString m_containerId;

    enum {
        ID_TERMINAL_TAB = wxID_HIGHEST + 1,
        ID_SQL_TAB,
        ID_NEXT_BUTTON,
        ID_PYTHON_WORK_COMPLETE  // Add event ID
    };

    DECLARE_EVENT_TABLE()
};

#endif // SECONDWINDOW_H