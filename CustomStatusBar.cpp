// CustomStatusBar.cpp
#include "CustomStatusBar.h"

BEGIN_EVENT_TABLE(CustomStatusBar, wxPanel)
    EVT_PAINT(CustomStatusBar::OnPaint)
END_EVENT_TABLE()

CustomStatusBar::CustomStatusBar(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, 24))
{
    ThemeSystem::Get().RegisterControl(this);
    ThemeSystem::Get().AddThemeChangeListener(this, 
        [this](ThemeSystem::ThemeVariant theme) { OnThemeChanged(theme); });
    CreateControls();
}

void CustomStatusBar::CreateControls() {
    wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    
    m_statusText = new wxStaticText(this, wxID_ANY, "Ready");
    wxFont statusFont = m_statusText->GetFont();
    statusFont.SetPointSize(9);
    m_statusText->SetFont(statusFont);
    
    SetBackgroundColour(ThemeSystem::Get().GetColor(ColorRole::Background));
    m_statusText->SetForegroundColour(ThemeSystem::Get().GetColor(ColorRole::Foreground));
    
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
    dc.SetPen(wxPen(ThemeSystem::Get().GetColor(ColorRole::Border)));
    dc.DrawLine(0, 0, GetSize().GetWidth(), 0);
}

void CustomStatusBar::OnThemeChanged(ThemeSystem::ThemeVariant theme) {
    SetBackgroundColour(ThemeSystem::Get().GetColor(ColorRole::Background));
    m_statusText->SetForegroundColour(ThemeSystem::Get().GetColor(ColorRole::Foreground));
    Refresh();
    Update();
}