#include "CustomStatusBar.h"
#include "ThemeColors.h"

BEGIN_EVENT_TABLE(CustomStatusBar, wxPanel)
    EVT_PAINT(CustomStatusBar::OnPaint)
END_EVENT_TABLE()

CustomStatusBar::CustomStatusBar(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, 24))
{
    bool isDarkMode = parent->GetBackgroundColour().GetLuminance() < 0.5;
    SetBackgroundColour(isDarkMode ? ThemeColors::DARK_BACKGROUND : ThemeColors::LIGHT_BACKGROUND);
    SetForegroundColour(isDarkMode ? ThemeColors::DARK_TEXT : ThemeColors::LIGHT_TEXT);

    wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    
    m_statusText = new wxStaticText(this, wxID_ANY, "Ready");
    wxFont statusFont = m_statusText->GetFont();
    statusFont.SetPointSize(9);
    m_statusText->SetFont(statusFont);
    m_statusText->SetForegroundColour(GetForegroundColour());
    
    sizer->Add(m_statusText, 0, wxALIGN_CENTER_VERTICAL | wxALL, 4);
    
    SetSizer(sizer);
}

void CustomStatusBar::SetStatusText(const wxString& text) {
    if (m_statusText) {
        m_statusText->SetLabel(text);
        Layout();
    }
}

void CustomStatusBar::OnPaint(wxPaintEvent& event) {
    wxPaintDC dc(this);
    
    bool isDarkMode = GetBackgroundColour().GetLuminance() < 0.5;
    wxColour borderColor = isDarkMode ? ThemeColors::DARK_BORDER : ThemeColors::LIGHT_BORDER;
    dc.SetPen(wxPen(borderColor));
    dc.DrawLine(0, 0, GetSize().GetWidth(), 0);
}