#ifndef CUSTOMSTATUSBAR_H
#define CUSTOMSTATUSBAR_H

#include <wx/wx.h>
#include "ThemeManager.h"

class CustomStatusBar : public wxPanel {
public:
    CustomStatusBar(wxWindow* parent);
    ~CustomStatusBar() {
        ThemeManager::Get().RemoveObserver(this);
    }
    void SetStatusText(const wxString& text);

private:
    void OnPaint(wxPaintEvent& event);
    void CreateControls();
    wxStaticText* m_statusText;
    
    DECLARE_EVENT_TABLE()
};

#endif // CUSTOMSTATUSBAR_H

