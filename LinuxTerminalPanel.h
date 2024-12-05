// LinuxTerminalPanel.h
#ifndef LINUXTERMINALPANEL_H
#define LINUXTERMINALPANEL_H

#include <wx/wx.h>

#ifdef __WXGTK__
    #include <wx/process.h>
    #include <X11/Xlib.h>
    #include <thread>
    #include <chrono>
#endif

class LinuxTerminalPanel : public wxPanel {
public:
    LinuxTerminalPanel(wxWindow* parent);
    virtual ~LinuxTerminalPanel();

private:
    // Terminal constants
    static const int TERM_CHAR_WIDTH;      // Average character width in pixels
    static const int TERM_CHAR_HEIGHT;     // Average character height in pixels
    static const int TERM_SCROLLBAR_WIDTH; // Typical scrollbar width
    static const int TERM_PADDING;         // Padding on each side
    static const int TERM_MIN_COLS;        // Minimum number of columns
    static const int TERM_MIN_ROWS;        // Minimum number of rows

#ifdef __WXGTK__
    void InitializeTerminal();
    void CleanupTerminal();
    void FocusOnTerminal();
    void OnSize(wxSizeEvent& event);
    void OnTerminalSize(wxSizeEvent& event);
    void OnMouseEvent(wxMouseEvent& event);
    void OnKeyEvent(wxKeyEvent& event);
    void ResizeTerminal(const wxSize& size);
    
    Window panelId;
    wxProcess* terminalProcess;
    wxPanel* m_terminalContainer;
    Display* m_display;
    bool m_initialized;
#else
    wxTextCtrl* m_textCtrl; // Fallback for non-Linux systems
#endif

    wxDECLARE_EVENT_TABLE();
};

#endif // LINUXTERMINALPANEL_H