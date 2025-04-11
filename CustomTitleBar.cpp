#include "CustomTitleBar.h"
#include "WindowIDs.h" // Include for IDs
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/dcbuffer.h>
#include <wx/graphics.h> // Include wxGraphicsContext

// --- MyButton Event Table ---
BEGIN_EVENT_TABLE(MyButton, wxWindow)
EVT_PAINT(MyButton::OnPaint)
EVT_LEFT_DOWN(MyButton::OnMouseDown)
EVT_LEFT_UP(MyButton::OnMouseUp)
EVT_ENTER_WINDOW(MyButton::OnMouseEnter)
EVT_LEAVE_WINDOW(MyButton::OnMouseLeave)
END_EVENT_TABLE()

// --- MyButton Constructor Implementation --- <<< ADD THIS
MyButton::MyButton(wxWindow *parent, wxWindowID id, const wxString &imagePath,
                   const wxPoint &pos, const wxSize &size,
                   long style, const wxString &name)
    : wxWindow(), m_alwaysShow(false), m_isHovered(false), m_isPressed(false) // Initialize state
{
    SetBackgroundStyle(wxBG_STYLE_PAINT); // Use transparent background
    // Add FULL_REPAINT_ON_RESIZE to ensure OnPaint is called correctly after size changes
    wxWindow::Create(parent, id, pos, size, style | wxFULL_REPAINT_ON_RESIZE, name);

    if (!imagePath.IsEmpty())
    {
        // Construct the full path assuming images are in a 'resources' subdir relative to executable
        wxString exePath = wxStandardPaths::Get().GetExecutablePath();
        wxFileName fName(exePath);
        fName.SetFullName("");        // Remove executable name
        fName.AppendDir("resources"); // Go into resources directory
        fName.SetFullName(imagePath); // Set the image filename

        // Use wxLogDebug for path issues
        wxLogDebug("MyButton: Attempting to load image from: %s", fName.GetFullPath());

        if (fName.FileExists())
        {
            m_image.LoadFile(fName.GetFullPath(), wxBITMAP_TYPE_PNG);
            if (m_image.IsOk())
            {
                if (size == wxDefaultSize)
                {
                    // Set initial size based on image if not specified
                    SetSize(m_image.GetWidth(), m_image.GetHeight());
                }
                m_originalImage = m_image.Copy(); // Store the unmodified image
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

    // Event bindings (Already done via EVENT_TABLE macros, no need to Bind() here unless specific cases)
    // Bind(wxEVT_PAINT, &MyButton::OnPaint, this);
    // Bind(wxEVT_LEFT_DOWN, &MyButton::OnMouseDown, this);
    // Bind(wxEVT_LEFT_UP, &MyButton::OnMouseUp, this);
    // Bind(wxEVT_ENTER_WINDOW, &MyButton::OnMouseEnter, this);
    // Bind(wxEVT_LEAVE_WINDOW, &MyButton::OnMouseLeave, this);
}

// --- MyButton Implementation ---
void MyButton::OnPaint(wxPaintEvent &event)
{
    wxAutoBufferedPaintDC dc(this);
    wxGraphicsContext *gc = wxGraphicsContext::Create(dc);

    if (gc && m_originalImage.IsOk())
    {
        const ThemeColors &colors = ThemeConfig::Get().GetThemeColors(ThemeConfig::Get().GetCurrentTheme());
        wxColour tintColor;
        double opacity = 1.0;

        if (m_isPressed)
        {
            tintColor = colors.button.pressed; // Use pressed color
            opacity = 0.8;                     // Make slightly more transparent when pressed
        }
        else if (m_isHovered)
        {
            tintColor = colors.button.hover; // Use hover color
            opacity = 0.9;                   // Make slightly transparent on hover
        }
        else
        {
            tintColor = colors.button.text; // Default icon color (usually text color of the theme)
        }

        // Draw Background (optional, can be transparent)
        wxRect rect = GetClientRect();
        // Let's adjust the *button background* instead of tinting the icon for simplicity
        wxColour buttonBg = GetParent()->GetBackgroundColour(); // Default to parent
        if (m_isPressed)
            buttonBg = colors.button.pressed;
        else if (m_isHovered)
            buttonBg = colors.button.hover;

        gc->SetBrush(wxBrush(buttonBg));
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->DrawRectangle(rect.x, rect.y, rect.width, rect.height);

        // Draw the icon, potentially tinted
        wxBitmap bmp(m_originalImage); // Use the original image for drawing
        gc->SetInterpolationQuality(wxINTERPOLATION_BEST);

        // Calculate scaling factor to fit the image within the button size
        wxSize bmpSize = bmp.GetSize();
        wxSize btnSize = GetClientSize();
        double scaleX = (double)btnSize.GetWidth() / bmpSize.GetWidth();
        double scaleY = (double)btnSize.GetHeight() / bmpSize.GetHeight();
        double scale = std::min({scaleX, scaleY, 1.0}) * 0.6; // Use 60% of button size

        double drawWidth = bmpSize.GetWidth() * scale;
        double drawHeight = bmpSize.GetHeight() * scale;
        double drawX = (btnSize.GetWidth() - drawWidth) / 2.0;
        double drawY = (btnSize.GetHeight() - drawHeight) / 2.0;

        // Now draw the image centered
        gc->DrawBitmap(bmp, drawX, drawY, drawWidth, drawHeight);

        delete gc;
    }
    else
    {
        // Fallback or error drawing if gc or image fails
        wxPaintDC dcFallback(this);
        dcFallback.SetBackground(*wxRED_BRUSH); // Indicate error
        dcFallback.Clear();
    }
}

void MyButton::OnMouseDown(wxMouseEvent &event)
{
    m_isPressed = true;
    CaptureMouse();
    Refresh();
    event.Skip(); // Allow parent (like title bar) to potentially handle too
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

    // Check if the mouse is still within the button bounds when released
    wxPoint mousePos = event.GetPosition();
    wxRect clientRect = GetClientRect();
    if (wasPressed && clientRect.Contains(mousePos))
    {
        // Trigger a button click event manually
        wxCommandEvent clickEvent(wxEVT_BUTTON, GetId());
        clickEvent.SetEventObject(this);
        ProcessWindowEvent(clickEvent);
    }
    else
    {
        event.Skip(); // Allow parent processing if not a click
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
    m_isPressed = false; // Ensure pressed state is reset if mouse leaves
    Refresh();
    event.Skip();
}

// Add the missing definition for DrawOnContext (even if empty for now)
void MyButton::DrawOnContext(wxGraphicsContext &gc)
{
    // This function is declared but not used in the current OnPaint logic.
    // Add drawing logic here if needed later, or remove the declaration.
}

// --- CustomTitleBar Event Table and Implementation ---
BEGIN_EVENT_TABLE(CustomTitleBar, wxPanel)
EVT_PAINT(CustomTitleBar::OnPaint)
EVT_LEFT_DOWN(CustomTitleBar::OnMouseLeftDown)
EVT_LEFT_UP(CustomTitleBar::OnMouseLeftUp)
EVT_MOTION(CustomTitleBar::OnMouseMove)
EVT_MOUSE_CAPTURE_LOST(CustomTitleBar::OnMouseCaptureLost)
EVT_LEAVE_WINDOW(CustomTitleBar::OnMouseLeave)
EVT_BUTTON(ID_MINIMIZE, CustomTitleBar::OnMinimize)  // Use IDs from WindowIDs.h
EVT_BUTTON(ID_CLOSE_WINDOW, CustomTitleBar::OnClose) // Use IDs from WindowIDs.h
END_EVENT_TABLE()

CustomTitleBar::CustomTitleBar(wxWindow *parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, 30)), // Adjusted height slightly
      m_titleText(nullptr),                                         // Initialize pointer
      m_isDragging(false)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    CreateControls();
    ThemeConfig::Get().ApplyTheme(this, ThemeConfig::Get().GetCurrentTheme());
}

void CustomTitleBar::CreateControls()
{
    wxBoxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
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
    sizer->Add(m_minimizeButton, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT, 0);
    sizer->Add(m_closeButton, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT, 0);
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
    if (clickedWindow == m_minimizeButton || clickedWindow == m_closeButton ||
        // Check if the click originated from one of the buttons directly
        dynamic_cast<MyButton *>(clickedWindow))
    {
        event.Skip();
        return;
    }

    m_dragStart = ClientToScreen(event.GetPosition());
    wxPoint framePos = GetParent()->GetPosition();
    m_dragStart = m_dragStart - framePos;
    m_isDragging = true;
    CaptureMouse();
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
    }
    event.Skip();
}

void CustomTitleBar::OnMouseMove(wxMouseEvent &event)
{
    if (m_isDragging && event.Dragging() && event.LeftIsDown())
    {
        wxPoint currentPos = ClientToScreen(event.GetPosition());
        wxPoint newFramePos = currentPos - m_dragStart;
        GetParent()->Move(newFramePos);
    }
    event.Skip();
}

void CustomTitleBar::OnMouseCaptureLost(wxMouseCaptureLostEvent &event)
{
    m_isDragging = false;
}

void CustomTitleBar::OnMouseLeave(wxMouseEvent &event)
{
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
