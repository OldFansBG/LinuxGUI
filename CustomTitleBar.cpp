#include "CustomTitleBar.h"

BEGIN_EVENT_TABLE(CustomTitleBar, wxPanel)
    EVT_PAINT(CustomTitleBar::OnPaint)
    EVT_LEFT_DOWN(CustomTitleBar::OnMouseLeftDown)
    EVT_LEFT_UP(CustomTitleBar::OnMouseLeftUp)
    EVT_MOTION(CustomTitleBar::OnMouseMove)
    EVT_MOUSE_CAPTURE_LOST(CustomTitleBar::OnMouseCaptureLost)
    EVT_LEAVE_WINDOW(CustomTitleBar::OnMouseLeave)
    EVT_BUTTON(ID_MINIMIZE, CustomTitleBar::OnMinimize)
    EVT_BUTTON(ID_MAXIMIZE, CustomTitleBar::OnMaximize)
    EVT_BUTTON(ID_CLOSE_WINDOW, CustomTitleBar::OnClose)
END_EVENT_TABLE()

CustomTitleBar::CustomTitleBar(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, 28)), 
      m_isDragging(false)
{
    ThemeSystem::Get().RegisterControl(this);
    ThemeSystem::Get().AddThemeChangeListener(this, 
        [this](ThemeSystem::ThemeVariant theme) { OnThemeChanged(theme); });
    
    CreateControls();
}

void CustomTitleBar::CreateControls() {
    wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Set the titlebar background color to blue
    SetBackgroundColour(ThemeSystem::Get().GetColor(ColorRole::TitleBar));  // This should be blue

    wxStaticText* titleText = new wxStaticText(this, wxID_ANY, "ISO Analyzer",
                                              wxDefaultPosition, wxDefaultSize,
                                              wxALIGN_CENTER_HORIZONTAL);
    wxFont titleFont = titleText->GetFont();
    titleFont.SetPointSize(9);
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);
    titleText->SetFont(titleFont);
    titleText->SetForegroundColour(wxColor(255, 255, 255));  // White text
    
    titleText->Bind(wxEVT_LEFT_DOWN, &CustomTitleBar::OnMouseLeftDown, this);
    titleText->Bind(wxEVT_LEFT_UP, &CustomTitleBar::OnMouseLeftUp, this);
    titleText->Bind(wxEVT_MOTION, &CustomTitleBar::OnMouseMove, this);
    
    m_minimizeButton = new MyButton(this, ID_MINIMIZE, "minimize.png", 
                                  wxDefaultPosition, FromDIP(wxSize(36, 28)));
    m_maximizeButton = new MyButton(this, ID_MAXIMIZE, "maximize.png", 
                                  wxDefaultPosition, FromDIP(wxSize(36, 28)));
    m_closeButton = new MyButton(this, ID_CLOSE_WINDOW, "close.png", 
                               wxDefaultPosition, FromDIP(wxSize(24, 16)));
    
    sizer->Add(titleText, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);
    sizer->AddSpacer(2);
    sizer->Add(m_minimizeButton, 0, wxEXPAND);
    sizer->Add(m_maximizeButton, 0, wxEXPAND);
    sizer->Add(m_closeButton, 0, wxEXPAND);
    
    SetSizer(sizer);
}

void CustomTitleBar::OnPaint(wxPaintEvent& event) {
    wxPaintDC dc(this);
    wxRect rect = GetClientRect();
    
    dc.SetBrush(wxBrush(ThemeSystem::Get().GetColor(ColorRole::TitleBar)));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(rect);
}

void CustomTitleBar::OnMouseLeftDown(wxMouseEvent& event) {
    wxWindow* button = wxDynamicCast(event.GetEventObject(), wxWindow);
    if (button && (button == m_minimizeButton || button == m_maximizeButton || button == m_closeButton)) {
        event.Skip();
        return;
    }
    
    m_dragStart = ::wxGetMousePosition();
    wxFrame* frame = wxDynamicCast(GetParent(), wxFrame);
    if (frame) {
        wxPoint framePos = frame->GetPosition();
        m_dragStart.x -= framePos.x;
        m_dragStart.y -= framePos.y;
    }
    m_isDragging = true;
    CaptureMouse();
}

void CustomTitleBar::OnMouseLeftUp(wxMouseEvent& event) {
    if (m_isDragging) {
        ReleaseMouse();
        m_isDragging = false;
    }
    event.Skip();
}

void CustomTitleBar::OnMouseMove(wxMouseEvent& event) {
    if (m_isDragging && event.Dragging() && event.LeftIsDown()) {
        wxPoint currentPos = ::wxGetMousePosition();
        wxFrame* frame = wxDynamicCast(GetParent(), wxFrame);
        if (frame) {
            frame->Move(wxPoint(
                currentPos.x - m_dragStart.x,
                currentPos.y - m_dragStart.y
            ));
        }
    }
    event.Skip();
}

void CustomTitleBar::OnMouseCaptureLost(wxMouseCaptureLostEvent& event) {
    m_isDragging = false;
}

void CustomTitleBar::OnMouseLeave(wxMouseEvent& event) {
    if (!event.LeftIsDown()) {
        m_isDragging = false;
        if (HasCapture()) {
            ReleaseMouse();
        }
    }
    event.Skip();
}

void CustomTitleBar::OnMinimize(wxCommandEvent& event) {
    if (wxFrame* frame = wxDynamicCast(GetParent(), wxFrame)) {
        frame->Iconize();
    }
}

void CustomTitleBar::OnMaximize(wxCommandEvent& event) {
    if (wxFrame* frame = wxDynamicCast(GetParent(), wxFrame)) {
        frame->Maximize(!frame->IsMaximized());
    }   
}

void CustomTitleBar::OnClose(wxCommandEvent& event) {
    if (wxFrame* frame = wxDynamicCast(GetParent(), wxFrame)) {
        frame->Close();
    }
}

void CustomTitleBar::OnThemeChanged(ThemeSystem::ThemeVariant theme) {
    SetBackgroundColour(ThemeSystem::Get().GetColor(ColorRole::TitleBar));
    
    for(wxWindow* child : GetChildren()) {
        if (auto* text = wxDynamicCast(child, wxStaticText)) {
            text->SetForegroundColour(ThemeSystem::Get().GetColor(ColorRole::TitleBarText));
        }
    }
    
    Refresh();
    Update();
}