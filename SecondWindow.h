#ifndef SECONDWINDOW_H
#define SECONDWINDOW_H

#include <wx/wx.h>
#include "OSDetector.h"
#include "WindowsCmdPanel.h"
#include "LinuxTerminalPanel.h"
#include <wx/notebook.h>

class SecondWindow : public wxFrame {
public:
    SecondWindow(wxWindow* parent, const wxString& title, const wxString& isoPath);

private:
    void CreateControls();
    void OnClose(wxCloseEvent& event);
    void OnTabChanged(wxCommandEvent& event);
    void OnNext(wxCommandEvent& event);

    OSDetector m_osDetector;
    WindowsCmdPanel* m_cmdPanel;
    LinuxTerminalPanel* m_terminalPanel;
    wxString m_isoPath;
    wxPanel* m_mainPanel;
    wxPanel* m_terminalTab;
    wxPanel* m_sqlTab;
    wxSizer* m_terminalSizer;
    wxSizer* m_sqlSizer;
    
    enum {
        ID_TERMINAL_TAB = wxID_HIGHEST + 1,
        ID_SQL_TAB,
        ID_NEXT_BUTTON
    };

    DECLARE_EVENT_TABLE()
};

#endif // SECONDWINDOW_H