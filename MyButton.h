#pragma once

#include <wx/wx.h>
#include <wx/graphics.h>
#include <memory>
#include "ThemeSystem.h"
#include "ThemeRoles.h"  // Add this include

class MyButton : public wxWindow {
public:
    MyButton(wxWindow *parent, wxWindowID id, const wxString& imagePath, 
             const wxPoint &pos = wxDefaultPosition, 
             const wxSize &size = wxDefaultSize, 
             long style = 0, 
             const wxString &name = wxPanelNameStr)
        : wxWindow(), m_alwaysShow(false)
    {
        ThemeSystem::Get().RegisterControl(this);
        SetBackgroundStyle(wxBG_STYLE_TRANSPARENT);
        wxWindow::Create(parent, id, pos, size, style, name);

        if (!imagePath.IsEmpty()) {
            m_image.LoadFile(imagePath, wxBITMAP_TYPE_PNG);
            if (m_image.IsOk()) {
                if (size == wxDefaultSize) {
                    SetSize(m_image.GetWidth(), m_image.GetHeight());
                }
                m_originalImage = m_image.Copy();
            }
        }

#if defined(__WXMSW__)
        int extendedStyle = GetWindowLong(GetHWND(), GWL_EXSTYLE);
        SetWindowLong(GetHWND(), GWL_EXSTYLE, extendedStyle | WS_EX_TRANSPARENT);
#endif

        Bind(wxEVT_PAINT, &MyButton::OnPaint, this);
        Bind(wxEVT_LEFT_DOWN, &MyButton::OnMouseDown, this);
        Bind(wxEVT_LEFT_UP, &MyButton::OnMouseUp, this);
        Bind(wxEVT_ENTER_WINDOW, &MyButton::OnMouseEnter, this);
        Bind(wxEVT_LEAVE_WINDOW, &MyButton::OnMouseLeave, this);
    }

    ~MyButton() {
        ThemeSystem::Get().UnregisterControl(this);
    }

    void SetAlwaysShowButton(bool always) { m_alwaysShow = always; }

    void OnPaint(wxPaintEvent &event) {
        wxPaintDC dc(this);
        std::unique_ptr<wxGraphicsContext> gc{wxGraphicsContext::Create(dc)};
        if (gc) {
            DrawOnContext(*gc);
        }
    }

    void DrawOnContext(wxGraphicsContext &gc) {
        auto buttonRect = GetClientRect();

        if (m_image.IsOk()) {
            wxImage tempImage = m_originalImage.Copy();
            wxColour tintColor;

            if (m_isPressed) {
                tintColor = ThemeSystem::Get().GetColor(ColorRole::ButtonPressed);
            } else if (m_isHovered) {
                tintColor = ThemeSystem::Get().GetColor(ColorRole::ButtonHover);
            } else {
                tintColor = ThemeSystem::Get().GetColor(ColorRole::Button);
            }

            for (int x = 0; x < tempImage.GetWidth(); x++) {
                for (int y = 0; y < tempImage.GetHeight(); y++) {
                    if (tempImage.HasAlpha() && tempImage.GetAlpha(x, y) > 0) {
                        int r = tempImage.GetRed(x, y);
                        int g = tempImage.GetGreen(x, y);
                        int b = tempImage.GetBlue(x, y);

                        r = (r + tintColor.Red()) / 2;
                        g = (g + tintColor.Green()) / 2;
                        b = (b + tintColor.Blue()) / 2;

                        tempImage.SetRGB(x, y, r, g, b);
                    }
                }
            }

            wxBitmap bitmap(tempImage);
            
            double scaleX = static_cast<double>(buttonRect.GetWidth()) / tempImage.GetWidth();
            double scaleY = static_cast<double>(buttonRect.GetHeight()) / tempImage.GetHeight();
            double scale = std::min(scaleX, scaleY);

            double width = tempImage.GetWidth() * scale;
            double height = tempImage.GetHeight() * scale;
            double x = (buttonRect.GetWidth() - width) / 2.0;
            double y = (buttonRect.GetHeight() - height) / 2.0;

            gc.DrawBitmap(bitmap, x, y, width, height);
        }
    }

private:
    void OnMouseDown(wxMouseEvent& event) {
        m_isPressed = true;
        Refresh();
    }

    void OnMouseUp(wxMouseEvent& event) {
        if (m_isPressed) {
            m_isPressed = false;
            Refresh();
            wxCommandEvent clickEvent(wxEVT_BUTTON, GetId());
            clickEvent.SetEventObject(this);
            ProcessEvent(clickEvent);
        }
    }

    void OnMouseEnter(wxMouseEvent& event) {
        m_isHovered = true;
        Refresh();
    }

    void OnMouseLeave(wxMouseEvent& event) {
        m_isHovered = false;
        m_isPressed = false;
        Refresh();
    }

    wxImage m_image;
    wxImage m_originalImage;
    bool m_isHovered = false;
    bool m_isPressed = false;
    bool m_alwaysShow;
};