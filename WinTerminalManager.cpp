// WinTerminalManager.cpp
#include "WinTerminalManager.h"

wxBEGIN_EVENT_TABLE(WinTerminalManager, wxEvtHandler)
    EVT_SIZE(WinTerminalManager::OnParentResize)
wxEND_EVENT_TABLE()

WinTerminalManager::WinTerminalManager(wxPanel* parentPanel)
    : m_parentPanel(parentPanel), m_hwndCmd(nullptr) 
{
    memset(&m_pi, 0, sizeof(PROCESS_INFORMATION));
    if (m_parentPanel) {
        m_parentPanel->Bind(wxEVT_SIZE, &WinTerminalManager::OnParentResize, this);
    }
}

WinTerminalManager::~WinTerminalManager() {
    CleanupProcess();
    if (m_parentPanel) {
        m_parentPanel->Unbind(wxEVT_SIZE, &WinTerminalManager::OnParentResize, this);
    }
}

bool WinTerminalManager::Initialize() {
    std::wstring cmd = L"cmd.exe /k title EMBEDDED_CMD";
    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    if (CreateProcessW(NULL, const_cast<wchar_t*>(cmd.c_str()),
        NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &m_pi)) {
        
        return FindAndEmbedTerminal();
    }
    return false;
}

bool WinTerminalManager::FindAndEmbedTerminal() {
    int retries = 20;  // Increased retries for slower systems
    while (retries-- > 0) {
        m_hwndCmd = FindWindowW(L"ConsoleWindowClass", L"EMBEDDED_CMD");
        if (m_hwndCmd) break;
        Sleep(150);  // Increased sleep time
    }

    if (!m_hwndCmd) return false;

    // Ensure parent window is valid
    if (!::IsWindow((HWND)m_parentPanel->GetHWND())) {
        return false;
    }

    // Modify window styles
    LONG_PTR style = GetWindowLongPtrW(m_hwndCmd, GWL_STYLE);
    style &= ~(WS_POPUP | WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
    style |= WS_CHILD;  // Make it a child window
    SetWindowLongPtrW(m_hwndCmd, GWL_STYLE, style);

    // Reparent and resize
    ::SetParent(m_hwndCmd, (HWND)m_parentPanel->GetHWND());
    UpdateTerminalSize();
    
    // Force redraw
    ::ShowWindow(m_hwndCmd, SW_SHOW);
    ::UpdateWindow(m_hwndCmd);
    return true;
}

void WinTerminalManager::UpdateTerminalSize() {
    if (m_hwndCmd && m_parentPanel) {
        ::SetWindowPos(m_hwndCmd, NULL, 0, 0,
            m_parentPanel->GetClientSize().GetWidth(),
            m_parentPanel->GetClientSize().GetHeight(),
            SWP_NOZORDER | SWP_FRAMECHANGED);
    }
}

void WinTerminalManager::OnParentResize(wxSizeEvent& event) {
    UpdateTerminalSize();
    event.Skip();  // Allow other handlers to process the event
}

void WinTerminalManager::CleanupProcess() {
    if (m_pi.hProcess) {
        // Gracefully terminate the process
        TerminateProcess(m_pi.hProcess, 0);
        CloseHandle(m_pi.hProcess);
        CloseHandle(m_pi.hThread);
    }
}

// New method to send commands to the terminal
bool WinTerminalManager::SendCommand(const std::wstring& command) {
    if (!m_hwndCmd) return false;

    // Simulate typing the command into the terminal
    for (wchar_t c : command) {
        ::PostMessageW(m_hwndCmd, WM_CHAR, c, 0);
    }

    // Simulate pressing Enter
    ::PostMessageW(m_hwndCmd, WM_KEYDOWN, VK_RETURN, 0);
    ::PostMessageW(m_hwndCmd, WM_KEYUP, VK_RETURN, 0);

    return true;
}