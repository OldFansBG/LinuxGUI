#include "SecondWindow.h"

wxBEGIN_EVENT_TABLE(SecondWindow, wxFrame)
    EVT_CLOSE(SecondWindow::OnClose)
wxEND_EVENT_TABLE()

SecondWindow::SecondWindow(wxWindow* parent, const wxString& title, const wxString& isoPath)
    : wxFrame(parent, wxID_ANY, title, wxDefaultPosition, wxSize(800, 650)),
      m_isoPath(isoPath), m_cmdPanel(nullptr), m_terminalPanel(nullptr)
{
    wxPanel* panel = new wxPanel(this);
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    wxStaticText* osText = new wxStaticText(panel, wxID_ANY, title);
    mainSizer->Add(osText, 0, wxALIGN_CENTER | wxALL, 10);

    OSDetector::OS currentOS = m_osDetector.GetCurrentOS();

    if (currentOS == OSDetector::OS::Windows) {
        m_cmdPanel = new WindowsCmdPanel(panel);
        m_cmdPanel->SetISOPath(m_isoPath);
        mainSizer->Add(m_cmdPanel, 1, wxEXPAND | wxALL, 5);
    } else {
        m_terminalPanel = new LinuxTerminalPanel(panel);
        m_terminalPanel->SetMinSize(wxSize(680, 400));
        mainSizer->Add(m_terminalPanel, 1, wxEXPAND | wxALL, 5);
    }

    panel->SetSizer(mainSizer);
    SetMinSize(wxSize(700, 500));
    CenterOnScreen();
    panel->Layout();
    Layout();
}

void SecondWindow::OnClose(wxCloseEvent& event)
{
    if (GetParent()) {
        GetParent()->Show();
    }
    Destroy();
}