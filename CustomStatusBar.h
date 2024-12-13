#ifndef CUSTOMSTATUSBAR_H
#define CUSTOMSTATUSBAR_H

#include <wx/wx.h>
#include "ThemeSystem.h"
#include "ThemeRoles.h"  // Add this include

class CustomStatusBar : public wxPanel {
public:
    CustomStatusBar(wxWindow* parent);
    ~CustomStatusBar() {
        ThemeSystem::Get().UnregisterControl(this);
    }
    void SetStatusText(const wxString& text);

private:
    void OnPaint(wxPaintEvent& event);
    void CreateControls();
    void OnThemeChanged(ThemeSystem::ThemeVariant theme);
    
    wxStaticText* m_statusText;
    
    DECLARE_EVENT_TABLE()
};

#endif // CUSTOMSTATUSBAR_H