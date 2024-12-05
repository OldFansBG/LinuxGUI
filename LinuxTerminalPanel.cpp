#include "LinuxTerminalPanel.h"
#include <wx/sizer.h>

#ifdef __WXGTK__
    #include <gtk/gtk.h>
    #include <gdk/gdkx.h>
    #include <sys/ioctl.h>
    #include <termios.h>
    #include <fcntl.h>
    #include <unistd.h>
#endif

// Define the static constants
const int LinuxTerminalPanel::TERM_CHAR_WIDTH = 8;
const int LinuxTerminalPanel::TERM_CHAR_HEIGHT = 16;
const int LinuxTerminalPanel::TERM_SCROLLBAR_WIDTH = 12;
const int LinuxTerminalPanel::TERM_PADDING = 2;
const int LinuxTerminalPanel::TERM_MIN_COLS = 80;
const int LinuxTerminalPanel::TERM_MIN_ROWS = 24;

wxBEGIN_EVENT_TABLE(LinuxTerminalPanel, wxPanel)
#ifdef __WXGTK__
    EVT_SIZE(LinuxTerminalPanel::OnSize)
    EVT_MOUSE_EVENTS(LinuxTerminalPanel::OnMouseEvent)
    EVT_KEY_DOWN(LinuxTerminalPanel::OnKeyEvent)
#endif
wxEND_EVENT_TABLE()

LinuxTerminalPanel::LinuxTerminalPanel(wxWindow *parent)
    : wxPanel(parent, wxID_ANY)
{
#ifdef __WXGTK__
    panelId = 0;
    terminalProcess = nullptr;
    m_display = nullptr;
    m_initialized = false;
    
    SetBackgroundColour(*wxBLACK);
    
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    m_terminalContainer = new wxPanel(this, wxID_ANY);
    m_terminalContainer->SetBackgroundColour(*wxBLACK);
    
    mainSizer->Add(m_terminalContainer, 1, wxEXPAND | wxALL, 0);
    
    SetSizer(mainSizer);
    
    m_display = XOpenDisplay(NULL);
    
    CallAfter(&LinuxTerminalPanel::InitializeTerminal);
#else
    // Create a simple text control as fallback for non-Linux systems
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    m_textCtrl = new wxTextCtrl(this, wxID_ANY, 
                               "Terminal emulation is only available on Linux systems.\n"
                               "This is a placeholder text control.",
                               wxDefaultPosition, wxDefaultSize,
                               wxTE_MULTILINE | wxTE_READONLY);
    sizer->Add(m_textCtrl, 1, wxEXPAND | wxALL, 5);
    SetSizer(sizer);
#endif
}

LinuxTerminalPanel::~LinuxTerminalPanel()
{
#ifdef __WXGTK__
    CleanupTerminal();
    if (m_display) {
        XCloseDisplay(m_display);
        m_display = nullptr;
    }
#endif
}

#ifdef __WXGTK__
void LinuxTerminalPanel::InitializeTerminal()
{
    if (!m_terminalContainer || !m_display) return;

    gtk_widget_realize(GTK_WIDGET(m_terminalContainer->GetHandle()));
    panelId = GDK_WINDOW_XID(gtk_widget_get_window(GTK_WIDGET(m_terminalContainer->GetHandle())));

    if (panelId == 0) {
        wxLogError("Failed to get the window ID for embedding xterm.");
        return;
    }

    wxSize containerSize = m_terminalContainer->GetClientSize();
    
    // Calculate columns and rows
    int cols = (containerSize.GetWidth() - TERM_SCROLLBAR_WIDTH) / TERM_CHAR_WIDTH;
    int rows = containerSize.GetHeight() / TERM_CHAR_HEIGHT;

    cols = std::max(cols, TERM_MIN_COLS);
    rows = std::max(rows, TERM_MIN_ROWS);

    // Build xterm command with specific settings for better text wrapping
    wxString command = wxString::Format(
        "xterm "
        "-into %ld "
        "-bg black "
        "-fg white "
        "-fa 'Monospace' "
        "-fs 10 "
        "-geometry %dx%d "
        "-b 0 "
        "+sb " // Enable scrollbar
        "-rightbar "
        "-aw "  // Enable autowrap
        "-mk "  // Enable margin bell
        "-si "  // Enable scroll on tty output
        "-sk "  // Enable scroll on keypress
        "-sl 5000 " // Scrollback lines
        "-bc "  // Enable text cursor blinking
        "-bcf 500 " // Blink frequency
        "-wf "  // Enable wide font
        "-cc 33:48,35-38:48,40-58:48,63-255:48 " // Character classes
        "-e bash -c '"
        "export COLUMNS=%d; "
        "export LINES=%d; "
        "cd \"%s\"; "
        "stty columns $COLUMNS rows $LINES; "
        "echo \"Terminal size: $COLUMNS x $LINES\"; "
        "docker pull ubuntu && "
        "docker run --privileged -it "
        "--rm "
        "-e TERM=xterm-256color "
        "-e COLUMNS=$COLUMNS "
        "-e LINES=$LINES "
        "ubuntu bash --login'",
        panelId,
        cols, rows,
        cols, rows,
        wxGetCwd());

    terminalProcess = wxProcess::Open(command);
    
    if (!terminalProcess) {
        wxLogError("Failed to launch xterm process.");
        return;
    }

    m_initialized = true;
    wxMilliSleep(100);
    ResizeTerminal(containerSize);
}

void LinuxTerminalPanel::CleanupTerminal()
{
    if (terminalProcess) {
        terminalProcess->Detach();
        terminalProcess->Kill(terminalProcess->GetPid(), wxSIGKILL);
        delete terminalProcess;
        terminalProcess = nullptr;
    }
    m_initialized = false;
}

void LinuxTerminalPanel::ResizeTerminal(const wxSize& size)
{
    if (!m_initialized || !m_display || panelId == 0) return;

    // Calculate terminal dimensions accounting for scrollbar
    int cols = std::max(TERM_MIN_COLS, (size.GetWidth() - TERM_SCROLLBAR_WIDTH) / TERM_CHAR_WIDTH);
    int rows = std::max(TERM_MIN_ROWS, size.GetHeight() / TERM_CHAR_HEIGHT);

    // Resize the X window
    XResizeWindow(m_display, panelId, size.GetWidth(), size.GetHeight());
    XFlush(m_display);

    // Update terminal dimensions
    if (terminalProcess) {
        int pid = terminalProcess->GetPid();
        
        // Try all possible TTY paths
        const char* ttyPaths[] = {
            "/proc/%d/fd/0",
            "/proc/%d/fd/1",
            "/proc/%d/fd/2"
        };
        
        bool resizeSuccess = false;
        for (const char* ttyPathFormat : ttyPaths) {
            wxString ttyPath = wxString::Format(ttyPathFormat, pid);
            int fd = open(ttyPath.c_str(), O_RDWR);
            if (fd != -1) {
                // Set terminal size
                struct winsize ws;
                ws.ws_col = cols;
                ws.ws_row = rows;
                ws.ws_xpixel = size.GetWidth();
                ws.ws_ypixel = size.GetHeight();
                
                if (ioctl(fd, TIOCSWINSZ, &ws) == 0) {
                    resizeSuccess = true;

                    // Update environment variables
                    char envColumns[32], envLines[32];
                    snprintf(envColumns, sizeof(envColumns), "COLUMNS=%d", cols);
                    snprintf(envLines, sizeof(envLines), "LINES=%d", rows);
                    putenv(envColumns);
                    putenv(envLines);

                    // Update stty settings
                    wxString sttyCmd = wxString::Format("stty columns %d rows %d\n", cols, rows);
                    write(fd, sttyCmd.c_str(), sttyCmd.length());
                }
                
                close(fd);
                if (resizeSuccess) break;
            }
        }

        if (resizeSuccess) {
            // Send SIGWINCH to terminal and all child processes
            kill(pid, SIGWINCH);
            
            // Find and signal all child processes
            wxString pgrepCmd = wxString::Format("pgrep -P %d", pid);
            wxArrayString output;
            wxExecute(pgrepCmd, output);
            
            for (const wxString& childPid : output) {
                long childPidNum;
                if (childPid.ToLong(&childPidNum)) {
                    kill(childPidNum, SIGWINCH);
                    
                    // Also try to update Docker container if it exists
                    wxString dockerCmd = wxString::Format(
                        "docker exec $(docker ps -q) /bin/sh -c 'stty columns %d rows %d'", 
                        cols, rows);
                    wxExecute(dockerCmd);
                }
            }
        }
    }

    // Update container and refresh
    m_terminalContainer->SetSize(size);
    m_terminalContainer->Refresh();
    Layout();
}

void LinuxTerminalPanel::OnSize(wxSizeEvent& event)
{
    const wxSize& size = event.GetSize();
    ResizeTerminal(size);
    event.Skip();
}

void LinuxTerminalPanel::OnTerminalSize(wxSizeEvent& event)
{
    const wxSize& size = event.GetSize();
    ResizeTerminal(size);
    event.Skip();
}

void LinuxTerminalPanel::FocusOnTerminal()
{
    if (panelId != 0 && m_display) {
        XSetInputFocus(m_display, panelId, RevertToParent, CurrentTime);
        XFlush(m_display);
    }
}

void LinuxTerminalPanel::OnMouseEvent(wxMouseEvent& event)
{
    event.Skip();
    FocusOnTerminal();
}

void LinuxTerminalPanel::OnKeyEvent(wxKeyEvent& event)
{
    event.Skip();
    FocusOnTerminal();
}
#endif // __WXGTK__