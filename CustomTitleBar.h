#ifndef CUSTOMTITLEBAR_H
#define CUSTOMTITLEBAR_H

#include <wx/wx.h>
#include "MyButton.h"
#include "ThemeSystem.h"
#include "ThemeRoles.h"  // Add this include

class CustomTitleBar : public wxPanel {
private:
    enum {
        ID_MINIMIZE = wxID_HIGHEST + 1000,
        ID_MAXIMIZE,
        ID_CLOSE_WINDOW
    };

public:
    CustomTitleBar(wxWindow* parent);
    ~CustomTitleBar() {
        ThemeSystem::Get().UnregisterControl(this);
    }

private:
    void CreateControls();
    void OnPaint(wxPaintEvent& event);
    void OnMouseLeftDown(wxMouseEvent& event);
    void OnMouseLeftUp(wxMouseEvent& event);
    void OnMouseMove(wxMouseEvent& event);
    void OnMouseCaptureLost(wxMouseCaptureLostEvent& event);
    void OnMouseLeave(wxMouseEvent& event);
    void OnMinimize(wxCommandEvent& event);
    void OnMaximize(wxCommandEvent& event);
    void OnClose(wxCommandEvent& event);
    void OnThemeChanged(ThemeSystem::ThemeVariant theme);

    MyButton* m_minimizeButton;
    MyButton* m_maximizeButton;
    MyButton* m_closeButton;
    bool m_isDragging;
    wxPoint m_dragStart;

    DECLARE_EVENT_TABLE()
};

#endif // CUSTOMTITLEBAR_H