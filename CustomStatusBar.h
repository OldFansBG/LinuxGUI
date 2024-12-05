#ifndef CUSTOMSTATUSBAR_H
#define CUSTOMSTATUSBAR_H

#include <wx/wx.h>

class CustomStatusBar : public wxPanel {
public:
    CustomStatusBar(wxWindow* parent);
    void SetStatusText(const wxString& text);

private:
    void OnPaint(wxPaintEvent& event);
    wxStaticText* m_statusText;
    
    DECLARE_EVENT_TABLE()
};

#endif // CUSTOMSTATUSBAR_H