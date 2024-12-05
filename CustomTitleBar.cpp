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
    // Check parent window's background to determine theme
    bool isDarkMode = parent->GetBackgroundColour().GetLuminance() < 0.5;
    
    // Set correct colors based on theme
    if (isDarkMode) {
        SetBackgroundColour(ThemeColors::DARK_TITLEBAR);
        SetForegroundColour(ThemeColors::DARK_TEXT);
    } else {
        SetBackgroundColour(ThemeColors::LIGHT_TITLEBAR);
        SetForegroundColour(ThemeColors::LIGHT_PRIMARY_TEXT);
    }

    wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Add title text
    wxStaticText* titleText = new wxStaticText(this, wxID_ANY, "ISO Analyzer",
                                              wxDefaultPosition, wxDefaultSize,
                                              wxALIGN_CENTER_HORIZONTAL);
    wxFont titleFont = titleText->GetFont();
    titleFont.SetPointSize(9);
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);
    titleText->SetFont(titleFont);
    titleText->SetForegroundColour(GetForegroundColour());

    // Bind mouse events for window dragging
    titleText->Bind(wxEVT_LEFT_DOWN, &CustomTitleBar::OnMouseLeftDown, this);
    titleText->Bind(wxEVT_LEFT_UP, &CustomTitleBar::OnMouseLeftUp, this);
    titleText->Bind(wxEVT_MOTION, &CustomTitleBar::OnMouseMove, this);
    
    // Add bindings for the panel itself
    this->Bind(wxEVT_LEFT_DOWN, &CustomTitleBar::OnMouseLeftDown, this);
    this->Bind(wxEVT_LEFT_UP, &CustomTitleBar::OnMouseLeftUp, this);
    this->Bind(wxEVT_MOTION, &CustomTitleBar::OnMouseMove, this);

    // Create window control buttons
    m_minimizeButton = new MyButton(this, ID_MINIMIZE, "minimize.png", 
                                  wxDefaultPosition, FromDIP(wxSize(36, 28)));
    m_maximizeButton = new MyButton(this, ID_MAXIMIZE, "maximize.png", 
                                  wxDefaultPosition, FromDIP(wxSize(36, 28)));
    m_closeButton = new MyButton(this, ID_CLOSE_WINDOW, "close.png", 
                               wxDefaultPosition, FromDIP(wxSize(24, 16)));
    
    // Style the buttons
    StyleButton(m_minimizeButton);
    StyleButton(m_maximizeButton);
    StyleButton(m_closeButton);
    
    // Special styling for close button
    m_closeButton->SetHoverColor(ThemeColors::CLOSE_BUTTON_HOVER);
    
    // Layout
    sizer->Add(titleText, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);
    sizer->AddSpacer(2);
    sizer->Add(m_minimizeButton, 0, wxEXPAND);
    sizer->Add(m_maximizeButton, 0, wxEXPAND);
    sizer->Add(m_closeButton, 0, wxEXPAND);
    
    SetSizer(sizer);
}

void CustomTitleBar::StyleButton(MyButton* button) {
    bool isDarkMode = GetBackgroundColour().GetLuminance() < 0.5;
    
    if (isDarkMode) {
        button->SetBackgroundColour(ThemeColors::DARK_BUTTON_BG);
        button->SetForegroundColour(ThemeColors::DARK_BUTTON_TEXT);
        button->SetHoverColor(ThemeColors::DARK_BUTTON_HOVER);
    } else {
        button->SetBackgroundColour(ThemeColors::LIGHT_TITLEBAR);
        button->SetForegroundColour(ThemeColors::LIGHT_PRIMARY_TEXT);
        button->SetHoverColor(wxColour(70, 140, 242));
    }
}

void CustomTitleBar::OnPaint(wxPaintEvent& event) {
    wxPaintDC dc(this);
    wxRect rect = GetClientRect();
    
    // Fill background with current theme color
    dc.SetBrush(wxBrush(GetBackgroundColour()));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(rect);
    
    // Draw bottom border
    bool isDarkMode = GetBackgroundColour().GetLuminance() < 0.5;
    wxColour borderColor = isDarkMode ? ThemeColors::DARK_BORDER : ThemeColors::LIGHT_BORDER;
    dc.SetPen(wxPen(borderColor));
    dc.DrawLine(0, GetSize().GetHeight() - 1, GetSize().GetWidth(), GetSize().GetHeight() - 1);
}

void CustomTitleBar::OnMouseLeftDown(wxMouseEvent& event) {
    wxWindow* button = wxDynamicCast(event.GetEventObject(), wxWindow);
    if (button && (button == m_minimizeButton || button == m_maximizeButton || button == m_closeButton)) {
        event.Skip();
        return;
    }
    
    m_dragStart = ::wxGetMousePosition();  // Get global mouse position
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
        wxPoint currentPos = ::wxGetMousePosition(); // Get global mouse position
        wxFrame* frame = wxDynamicCast(GetParent(), wxFrame);
        if (frame) {
            wxPoint newPos(
                currentPos.x - m_dragStart.x,
                currentPos.y - m_dragStart.y
            );
            frame->Move(newPos);
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
    wxFrame* frame = wxDynamicCast(GetParent(), wxFrame);
    if (frame) {
        frame->Iconize();
    }
}

void CustomTitleBar::OnMaximize(wxCommandEvent& event) {
    wxFrame* frame = wxDynamicCast(GetParent(), wxFrame);
    if (frame) {
        frame->Maximize(!frame->IsMaximized());
    }   
}

void CustomTitleBar::OnClose(wxCommandEvent& event) {
    wxFrame* frame = wxDynamicCast(GetParent(), wxFrame);
    if (frame) {
        frame->Close();
    }
}