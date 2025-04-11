#include "CustomTitleBar.h"
#include "WindowIDs.h" // Include for IDs
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/dcbuffer.h>
#include <wx/graphics.h> // Include wxGraphicsContext
#include <algorithm>     // Include for std::min

// --- MyButton Event Table ---
BEGIN_EVENT_TABLE(MyButton, wxWindow)
EVT_PAINT(MyButton::OnPaint)
EVT_LEFT_DOWN(MyButton::OnMouseDown)
EVT_LEFT_UP(MyButton::OnMouseUp)
EVT_ENTER_WINDOW(MyButton::OnMouseEnter)
EVT_LEAVE_WINDOW(MyButton::OnMouseLeave)
END_EVENT_TABLE()

// --- MyButton Constructor Implementation ---
MyButton::MyButton(wxWindow *parent, wxWindowID id, const wxString &imagePath,
                   const wxPoint &pos, const wxSize &size,
                   long style, const wxString &name)
    : wxWindow(),
      m_alwaysShow(false), m_isHovered(false), m_isPressed(false) // Initialize state
{
    SetBackgroundStyle(wxBG_STYLE_PAINT); // Use transparent background
    wxWindow::Create(parent, id, pos, size, style | wxFULL_REPAINT_ON_RESIZE, name);

    if (!imagePath.IsEmpty())
    {
        wxString exePath = wxStandardPaths::Get().GetExecutablePath();
        wxFileName fName(exePath);
        fName.SetFullName("");        // Remove executable name
        fName.AppendDir("resources"); // Go into resources directory
        fName.SetFullName(imagePath); // Set the image filename

        wxLogDebug("MyButton: Attempting to load image from: %s", fName.GetFullPath());

        if (fName.FileExists())
        {
            m_image.LoadFile(fName.GetFullPath(), wxBITMAP_TYPE_PNG);
            if (m_image.IsOk())
            {
                if (size == wxDefaultSize)
                {
                    SetSize(m_image.GetWidth(), m_image.GetHeight());
                }
                m_originalImage = m_image.Copy();
            }
            else
            {
                wxLogWarning("MyButton: Failed to load image file '%s' (invalid format or read error).", fName.GetFullPath());
            }
        }
        else
        {
            wxLogWarning("MyButton: Image file not found at '%s'.", fName.GetFullPath());
        }
    }
    else
    {
        wxLogWarning("MyButton: Empty imagePath provided.");
    }
}

// --- MyButton Implementation ---
void MyButton::OnPaint(wxPaintEvent &event)
{
    wxAutoBufferedPaintDC dc(this);
    wxGraphicsContext *gc = wxGraphicsContext::Create(dc);

    if (gc && m_originalImage.IsOk())
    {
        const ThemeColors &colors = ThemeConfig::Get().GetThemeColors(ThemeConfig::Get().GetCurrentTheme());
        wxRect rect = GetClientRect();
        wxColour buttonBg = GetParent()->GetBackgroundColour(); // Default to parent
        if (m_isPressed)
            buttonBg = colors.button.pressed;
        else if (m_isHovered)
            buttonBg = colors.button.hover;

        gc->SetBrush(wxBrush(buttonBg));
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->DrawRectangle(rect.x, rect.y, rect.width, rect.height);

        wxBitmap bmp(m_originalImage);
        gc->SetInterpolationQuality(wxINTERPOLATION_BEST);

        wxSize bmpSize = bmp.GetSize();
        wxSize btnSize = GetClientSize();
        double scaleX = (double)btnSize.GetWidth() / bmpSize.GetWidth();
        double scaleY = (double)btnSize.GetHeight() / bmpSize.GetHeight();
        double scale = std::min({scaleX, scaleY, 1.0}) * 0.6;

        double drawWidth = bmpSize.GetWidth() * scale;
        double drawHeight = bmpSize.GetHeight() * scale;
        double drawX = (btnSize.GetWidth() - drawWidth) / 2.0;
        double drawY = (btnSize.GetHeight() - drawHeight) / 2.0;

        gc->DrawBitmap(bmp, drawX, drawY, drawWidth, drawHeight);

        delete gc;
    }
    else
    {
        wxPaintDC dcFallback(this);
        dcFallback.SetBackground(*wxRED_BRUSH);
        dcFallback.Clear();
    }
}

void MyButton::OnMouseDown(wxMouseEvent &event)
{
    m_isPressed = true;
    CaptureMouse();
    Refresh();
    event.Skip();
}

void MyButton::OnMouseUp(wxMouseEvent &event)
{
    bool wasPressed = m_isPressed;
    m_isPressed = false;
    if (HasCapture())
    {
        ReleaseMouse();
    }
    Refresh();

    wxPoint mousePos = event.GetPosition();
    wxRect clientRect = GetClientRect();
    if (wasPressed && clientRect.Contains(mousePos))
    {
        wxCommandEvent clickEvent(wxEVT_BUTTON, GetId());
        clickEvent.SetEventObject(this);
        ProcessWindowEvent(clickEvent);
    }
    else
    {
        event.Skip();
    }
}

void MyButton::OnMouseEnter(wxMouseEvent &event)
{
    m_isHovered = true;
    Refresh();
    event.Skip();
}

void MyButton::OnMouseLeave(wxMouseEvent &event)
{
    m_isHovered = false;
    m_isPressed = false;
    Refresh();
    event.Skip();
}

void MyButton::DrawOnContext(wxGraphicsContext &gc)
{
    // Implementation not strictly needed based on current OnPaint
}

// --- CustomTitleBar Event Table and Implementation ---
BEGIN_EVENT_TABLE(CustomTitleBar, wxPanel)
EVT_PAINT(CustomTitleBar::OnPaint)
EVT_LEFT_DOWN(CustomTitleBar::OnMouseLeftDown)
EVT_LEFT_UP(CustomTitleBar::OnMouseLeftUp)
EVT_MOTION(CustomTitleBar::OnMouseMove)
EVT_MOUSE_CAPTURE_LOST(CustomTitleBar::OnMouseCaptureLost)
EVT_LEAVE_WINDOW(CustomTitleBar::OnMouseLeave)
EVT_BUTTON(ID_MINIMIZE, CustomTitleBar::OnMinimize)
EVT_BUTTON(ID_CLOSE_WINDOW, CustomTitleBar::OnClose)
END_EVENT_TABLE()

CustomTitleBar::CustomTitleBar(wxWindow *parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, 30)),
      m_titleText(nullptr),
      m_isDragging(false)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    CreateControls();
    ThemeConfig::Get().ApplyTheme(this, ThemeConfig::Get().GetCurrentTheme());
}

void CustomTitleBar::CreateControls()
{
    wxBoxSizer *sizer = new wxBoxSizer(wxHORIZONTAL); // It's a HORIZONTAL sizer
    SetSizer(sizer);

    m_titleText = new wxStaticText(this, wxID_ANY, "LinuxISOPro",
                                   wxDefaultPosition, wxDefaultSize,
                                   wxALIGN_CENTER_VERTICAL);
    wxFont titleFont = m_titleText->GetFont();
    titleFont.SetPointSize(10);
    titleFont.SetWeight(wxFONTWEIGHT_NORMAL);
    m_titleText->SetFont(titleFont);

    m_titleText->Bind(wxEVT_LEFT_DOWN, &CustomTitleBar::OnMouseLeftDown, this);
    m_titleText->Bind(wxEVT_LEFT_UP, &CustomTitleBar::OnMouseLeftUp, this);
    m_titleText->Bind(wxEVT_MOTION, &CustomTitleBar::OnMouseMove, this);
    this->Bind(wxEVT_LEFT_DOWN, &CustomTitleBar::OnMouseLeftDown, this);
    this->Bind(wxEVT_LEFT_UP, &CustomTitleBar::OnMouseLeftUp, this);
    this->Bind(wxEVT_MOTION, &CustomTitleBar::OnMouseMove, this);

    m_minimizeButton = new MyButton(this, ID_MINIMIZE, "minimize.png",
                                    wxDefaultPosition, wxSize(45, 30));
    m_closeButton = new MyButton(this, ID_CLOSE_WINDOW, "close.png",
                                 wxDefaultPosition, wxSize(45, 30));

    sizer->AddSpacer(5);
    sizer->Add(m_titleText, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
    sizer->Add(m_minimizeButton, 0, wxALIGN_CENTER_VERTICAL, 0);
    sizer->Add(m_closeButton, 0, wxALIGN_CENTER_VERTICAL, 0);
}

void CustomTitleBar::OnPaint(wxPaintEvent &event)
{
    wxAutoBufferedPaintDC dc(this);
    wxRect rect = GetClientRect();
    const ThemeColors &colors = ThemeConfig::Get().GetThemeColors(ThemeConfig::Get().GetCurrentTheme());
    dc.SetBrush(wxBrush(colors.titleBar));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(rect);
}

void CustomTitleBar::OnMouseLeftDown(wxMouseEvent &event)
{
    wxWindow *clickedWindow = dynamic_cast<wxWindow *>(event.GetEventObject());
    // Check if the click happened on a button or the title text itself.
    // If it's the title text, we *do* want to drag.
    // If it's a button, we *don't* want to drag.
    if (dynamic_cast<MyButton *>(clickedWindow))
    {
        event.Skip(); // Let the button handle its own click
        return;
    }

    // If the click is on the title text or the panel background, start dragging
    m_dragStart = ClientToScreen(event.GetPosition());
    wxPoint framePos = GetParent()->GetPosition();
    m_dragStart = m_dragStart - framePos;
    m_isDragging = true;
    CaptureMouse();
    // Don't skip here, we've handled the start of the drag
}

void CustomTitleBar::OnMouseLeftUp(wxMouseEvent &event)
{
    if (m_isDragging)
    {
        if (HasCapture())
        {
            ReleaseMouse();
        }
        m_isDragging = false;
        // Don't skip here if we handled the drag release
    }
    else
    {
        event.Skip(); // Allow button events etc. if not dragging
    }
}

void CustomTitleBar::OnMouseMove(wxMouseEvent &event)
{
    if (m_isDragging && event.Dragging() && event.LeftIsDown())
    {
        wxPoint currentPos = ClientToScreen(event.GetPosition());
        wxPoint newFramePos = currentPos - m_dragStart;
        GetParent()->Move(newFramePos);
        // Don't Skip the event after moving the window
        // event.Skip(); // <<< REMOVE or COMMENT OUT this line
    }
    else
    {
        event.Skip(); // Allow hover events etc. if not dragging
    }
}

void CustomTitleBar::OnMouseCaptureLost(wxMouseCaptureLostEvent &event)
{
    m_isDragging = false;
    // No need to skip here
}

void CustomTitleBar::OnMouseLeave(wxMouseEvent &event)
{
    // Reset hover state on buttons if mouse leaves the title bar entirely
    if (m_minimizeButton)
        m_minimizeButton->Refresh();
    if (m_closeButton)
        m_closeButton->Refresh();
    event.Skip();
}

void CustomTitleBar::OnMinimize(wxCommandEvent &event)
{
    wxFrame *frame = wxDynamicCast(GetParent(), wxFrame);
    if (frame)
    {
        frame->Iconize(true);
    }
}

void CustomTitleBar::OnClose(wxCommandEvent &event)
{
    wxFrame *frame = wxDynamicCast(GetParent(), wxFrame);
    if (frame)
    {
        frame->Close(true);
    }
}

void CustomTitleBar::RefreshButtons()
{
    if (m_minimizeButton)
        m_minimizeButton->Refresh();
    if (m_closeButton)
        m_closeButton->Refresh();
}
