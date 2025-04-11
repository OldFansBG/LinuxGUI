#ifndef CUSTOMTITLEBAR_H
#define CUSTOMTITLEBAR_H

#include <wx/wx.h>
#include <wx/graphics.h> // For wxGraphicsContext in MyButton
#include <wx/stdpaths.h> // For finding resources in MyButton
#include <wx/filename.h> // For path manipulation in MyButton
#include <memory>        // For std::unique_ptr in MyButton
#include "ThemeConfig.h" // Needed for theme access
#include "WindowIDs.h"   // Include for button IDs like ID_MINIMIZE, ID_CLOSE_WINDOW

//------------------------------------------------------------------------
// MyButton Class (Used by CustomTitleBar)
//------------------------------------------------------------------------
class MyButton : public wxWindow
{
public:
    MyButton(wxWindow *parent, wxWindowID id, const wxString &imagePath,
             const wxPoint &pos = wxDefaultPosition,
             const wxSize &size = wxDefaultSize,
             long style = 0,
             const wxString &name = wxPanelNameStr);

    void SetAlwaysShowButton(bool always) { m_alwaysShow = always; }

    // --- Event Handlers ---
    void OnPaint(wxPaintEvent &event);
    void OnMouseDown(wxMouseEvent &event);
    void OnMouseUp(wxMouseEvent &event);
    void OnMouseEnter(wxMouseEvent &event);
    void OnMouseLeave(wxMouseEvent &event);

private:
    void DrawOnContext(wxGraphicsContext &gc); // Helper for drawing logic

    // Member Variables
    wxImage m_image;
    wxImage m_originalImage;
    bool m_isHovered;
    bool m_isPressed;
    bool m_alwaysShow;

    wxDECLARE_EVENT_TABLE();
};

//------------------------------------------------------------------------
// CustomTitleBar Class
//------------------------------------------------------------------------
class CustomTitleBar : public wxPanel
{
public:
    CustomTitleBar(wxWindow *parent);

    wxStaticText *GetTitleTextWidget() { return m_titleText; }

    // Changed RefreshButtons to more comprehensive RefreshControls
    void RefreshControls();

private:
    // UI Creation
    void CreateControls();

    // Event Handlers
    void OnPaint(wxPaintEvent &event);
    void OnMouseLeftDown(wxMouseEvent &event);
    void OnMouseLeftUp(wxMouseEvent &event);
    void OnMouseMove(wxMouseEvent &event);
    void OnMouseCaptureLost(wxMouseCaptureLostEvent &event);
    void OnMouseLeave(wxMouseEvent &event);
    void OnMinimize(wxCommandEvent &event);
    void OnClose(wxCommandEvent &event);

    // Member Variables
    MyButton *m_minimizeButton;
    MyButton *m_closeButton;
    wxStaticText *m_titleText;
    bool m_isDragging;
    wxPoint m_dragStart;

    wxDECLARE_EVENT_TABLE();
};

#endif // CUSTOMTITLEBAR_H