#include "CustomStatusBar.h"

BEGIN_EVENT_TABLE(CustomStatusBar, wxPanel)
    EVT_PAINT(CustomStatusBar::OnPaint)
END_EVENT_TABLE()

CustomStatusBar::CustomStatusBar(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, 24))
{
    CreateControls();
}

void CustomStatusBar::CreateControls() {
    wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    
    m_statusText = new wxStaticText(this, wxID_ANY, "Ready");
    wxFont statusFont = m_statusText->GetFont();
    statusFont.SetPointSize(9);
    m_statusText->SetFont(statusFont);
    
    SetBackgroundColour(*wxLIGHT_GREY);
    m_statusText->SetForegroundColour(*wxBLACK);
    
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
    dc.SetPen(wxPen(*wxBLACK));
    dc.DrawLine(0, 0, GetSize().GetWidth(), 0);
}