// WinTerminalManager.h
#pragma once
#include <wx/wx.h>
#include <wx/panel.h>
#include <windows.h>

class WinTerminalManager : public wxEvtHandler {
public:
    WinTerminalManager(wxPanel* parentPanel);
    ~WinTerminalManager();

    bool Initialize();
    bool SendCommand(const std::wstring& command); // New method to send commands

private:
    wxPanel* m_parentPanel;
    PROCESS_INFORMATION m_pi;
    HWND m_hwndCmd;

    void CleanupProcess();
    bool FindAndEmbedTerminal();
    void UpdateTerminalSize();
    void OnParentResize(wxSizeEvent& event);

    wxDECLARE_EVENT_TABLE();
};