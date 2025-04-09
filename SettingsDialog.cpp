#include "SettingsDialog.h"
#include "CustomTitleBar.h"
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wxSVG/svg.h>
#include <wx/mstream.h>
wxDEFINE_EVENT(EVT_TOGGLE_SWITCH, wxCommandEvent);
wxBEGIN_EVENT_TABLE(ToggleSwitch, wxControl)
    EVT_PAINT(ToggleSwitch::OnPaint)
        EVT_LEFT_DOWN(ToggleSwitch::OnMouse)
            wxEND_EVENT_TABLE()
                ToggleSwitch::ToggleSwitch(wxWindow *parent, wxWindowID id, bool initialState, const wxPoint &pos, const wxSize &size) : wxControl(parent, id, pos, size, wxBORDER_NONE), m_value(initialState)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(size);
}
void ToggleSwitch::SetValue(bool value)
{
    if (m_value != value)
    {
        m_value = value;
        Refresh();
        wxCommandEvent event(EVT_TOGGLE_SWITCH, GetId());
        event.SetInt(m_value ? 1 : 0);
        event.SetEventObject(this);
        GetEventHandler()->ProcessEvent(event);
    }
}

void ToggleSwitch::OnPaint(wxPaintEvent &event)
{
    wxBufferedPaintDC dc(this);
    dc.SetBackground(*wxTRANSPARENT_BRUSH);
    dc.Clear();

    wxSize size = GetSize();
    int width = size.GetWidth();
    int height = size.GetHeight();
    int radius = height / 2;

    wxGraphicsContext *gc = wxGraphicsContext::Create(dc);
    if (gc)
    {
        gc->SetBrush(wxBrush(m_value ? m_trackOnColor : m_trackOffColor));
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->DrawRoundedRectangle(0, 0, width, height, radius);

        gc->SetBrush(wxBrush(m_thumbColor));
        int thumbPos = m_value ? width - height + 2 : 2;
        gc->DrawEllipse(thumbPos, 2, height - 4, height - 4);
        delete gc;
    }
}

void ToggleSwitch::OnMouse(wxMouseEvent &event)
{
    SetValue(!m_value);
}

wxBEGIN_EVENT_TABLE(SettingsDialog, wxDialog)
    EVT_BUTTON(wxID_OK, SettingsDialog::OnOK)
        EVT_BUTTON(wxID_CANCEL, SettingsDialog::OnCancel)
            EVT_BUTTON(wxID_ANY, SettingsDialog::OnThemeButtonClick)
                EVT_PAINT(SettingsDialog::OnPaint)
                    wxEND_EVENT_TABLE()

                        SettingsDialog::SettingsDialog(wxWindow *parent)
    : wxDialog(parent, wxID_ANY, "Settings",
               wxDefaultPosition, wxSize(500, 450),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxFRAME_SHAPED)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    CreateContent();
    if (ThemeConfig::Get().GetCurrentTheme() == "light")
    {
        m_lightButton->SetBackgroundColour(m_primaryColor);
        m_lightButton->SetForegroundColour(*wxWHITE);
        m_darkButton->SetBackgroundColour(m_bgColor);
    }
    else
    {
        m_darkButton->SetBackgroundColour(m_primaryColor);
        m_darkButton->SetForegroundColour(*wxWHITE);
        m_lightButton->SetBackgroundColour(m_bgColor);
    }
    Centre();
}

void SettingsDialog::CreateContent()
{
    m_mainPanel = new wxPanel(this);
    m_mainPanel->SetBackgroundColour(m_bgColor);

    wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);

    wxStaticText *titleText = new wxStaticText(m_mainPanel, wxID_ANY, "Settings");
    titleText->SetForegroundColour(m_textColor);
    wxFont titleFont = titleText->GetFont();
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);
    titleFont.SetPointSize(titleFont.GetPointSize() + 3);
    titleText->SetFont(titleFont);
    mainSizer->Add(titleText, 0, wxALL, 20);

    wxPanel *themePanel = new wxPanel(m_mainPanel);
    themePanel->SetBackgroundColour(m_bgColor);
    wxBoxSizer *themeSizer = new wxBoxSizer(wxVERTICAL);
    wxStaticText *themeLabel = new wxStaticText(themePanel, wxID_ANY, "Theme");
    themeLabel->SetForegroundColour(m_textColor);
    themeSizer->Add(themeLabel, 0, wxALL, 10);

    wxBoxSizer *themeButtonSizer = new wxBoxSizer(wxHORIZONTAL);
    m_lightButton = new wxButton(themePanel, wxID_ANY, "Light",
                                 wxDefaultPosition, wxSize(160, 45), wxBORDER_NONE);
    m_lightButton->SetBackgroundColour(m_bgColor);
    m_lightButton->SetForegroundColour(m_textColor);
    wxFont buttonFont = m_lightButton->GetFont();
    buttonFont.SetFaceName("Segoe UI Symbol");
    m_lightButton->SetFont(buttonFont);

    wxSVGDocument svgDoc;
    const char *svgData = R"(
        <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 512 512">
            <path d="M361.5 1.2c5 2.1 8.6 6.6 9.6 11.9L391 121l107.9 19.8c5.3 1 9.8 4.6 11.9 9.6s1.5 10.7-1.6 15.2L446.9 256l62.3 90.3c3.1 4.5 3.7 10.2 1.6 15.2s-6.6 8.6-11.9 9.6L391 391 371.1 498.9c-1 5.3-4.6 9.8-9.6 11.9s-10.7 1.5-15.2-1.6L256 446.9l-90.3 62.3c-4.5 3.1-10.2 3.7-15.2 1.6s-8.6-6.6-9.6-11.9L121 391 13.1 371.1c-5.3-1-9.8-4.6-11.9-9.6s-1.5-10.7 1.6-15.2L65.1 256 2.8 165.7c-3.1-4.5-3.7-10.2-1.6-15.2s6.6-8.6 11.9-9.6L121 121 140.9 13.1c1-5.3 4.6-9.8 9.6-11.9s10.7-1.5 15.2 1.6L256 65.1 346.3 2.8c4.5-3.1 10.2-3.7 15.2-1.6zM160 256a96 96 0 1 1 192 0 96 96 0 1 1 -192 0zm224 0a128 128 0 1 0 -256 0 128 128 0 1 0 256 0z"/>
        </svg>
    )";
    wxMemoryInputStream svgStream(svgData, strlen(svgData));
    if (svgDoc.Load(svgStream))
    {
        wxImage svgImage = svgDoc.Render(24, 24, nullptr, true, true);
        m_lightButton->SetBitmap(wxBitmap(svgImage));
    }
    m_lightButton->SetLabel(" Light");
    m_lightButton->SetBitmapMargins(8, 0);

    m_darkButton = new wxButton(themePanel, wxID_ANY, "Dark",
                                wxDefaultPosition, wxSize(160, 45), wxBORDER_NONE);
    m_darkButton->SetBackgroundColour(m_primaryColor);
    m_darkButton->SetForegroundColour(*wxWHITE);
    m_darkButton->SetFont(buttonFont);

    // Create a separate document instance for the dark button
    wxSVGDocument darkSvgDoc;
    const char *darkSvgData = R"(
    <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 384 512">
            <path d="M223.5 32C100 32 0 132.3 0 256S100 480 223.5 480c60.6 0 115.5-24.2 155.8-63.4c5-4.9 6.3-12.5 3.1-18.7s-10.1-9.7-17-8.5c-9.8 1.7-19.8 2.6-30.1 2.6c-96.9 0-175.5-78.8-175.5-176c0-65.8 36-123.1 89.3-153.3c6.1-3.5 9.2-10.5 7.7-17.3s-7.3-11.9-14.3-12.5c-6.3-.5-12.6-.8-19-.8z"/>
    </svg>
    )";
    wxMemoryInputStream darkSvgStream(darkSvgData, strlen(darkSvgData));
    if (darkSvgDoc.Load(darkSvgStream))
    {
        wxImage darkSvgImage = darkSvgDoc.Render(24, 24, nullptr, true, true);
        m_darkButton->SetBitmap(wxBitmap(darkSvgImage));
    }
    m_darkButton->SetLabel(" Dark");
    m_darkButton->SetBitmapMargins(8, 0);

    themeButtonSizer->Add(m_lightButton, 1, wxEXPAND | wxRIGHT, 5);
    themeButtonSizer->Add(m_darkButton, 1, wxEXPAND | wxLEFT, 5);
    themeSizer->Add(themeButtonSizer, 0, wxEXPAND | wxALL, 10);
    themePanel->SetSizer(themeSizer);
    mainSizer->Add(themePanel, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);

    mainSizer->AddSpacer(15);

    wxStaticBoxSizer *containerSizer = new wxStaticBoxSizer(wxVERTICAL, m_mainPanel, "Container Options");
    wxStaticBox *containerBox = containerSizer->GetStaticBox();
    containerBox->SetForegroundColour(m_textColor);
    containerBox->SetBackgroundColour(m_bgColor);

    wxPanel *dockerPanel = new wxPanel(containerBox);
    dockerPanel->SetBackgroundColour(m_bgColor);
    wxBoxSizer *dockerSizer = new wxBoxSizer(wxHORIZONTAL);
    wxMemoryInputStream dockerSvgStream(R"(
<svg xmlns="http://www.w3.org/2000/svg" aria-label="Docker" role="img" viewBox="0 0 512 512" fill="#000000"><g id="SVGRepo_bgCarrier" stroke-width="0"></g><g id="SVGRepo_tracerCarrier" stroke-linecap="round" stroke-linejoin="round"></g><g id="SVGRepo_iconCarrier"><rect width="512" height="512" rx="15%" fill="#ffffff"></rect><path stroke="#066da5" stroke-width="38" d="M296 226h42m-92 0h42m-91 0h42m-91 0h41m-91 0h42m8-46h41m8 0h42m7 0h42m-42-46h42"></path><path fill="#066da5" d="m472 228s-18-17-55-11c-4-29-35-46-35-46s-29 35-8 74c-6 3-16 7-31 7H68c-5 19-5 145 133 145 99 0 173-46 208-130 52 4 63-39 63-39"></path></g></svg>
)",
                                        strlen(R"(
<svg xmlns="http://www.w3.org/2000/svg" aria-label="Docker" role="img" viewBox="0 0 512 512" fill="#000000"><g id="SVGRepo_bgCarrier" stroke-width="0"></g><g id="SVGRepo_tracerCarrier" stroke-linecap="round" stroke-linejoin="round"></g><g id="SVGRepo_iconCarrier"><rect width="512" height="512" rx="15%" fill="#ffffff"></rect><path stroke="#066da5" stroke-width="38" d="M296 226h42m-92 0h42m-91 0h42m-91 0h41m-91 0h42m8-46h41m8 0h42m7 0h42m-42-46h42"></path><path fill="#066da5" d="m472 228s-18-17-55-11c-4-29-35-46-35-46s-29 35-8 74c-6 3-16 7-31 7H68c-5 19-5 145 133 145 99 0 173-46 208-130 52 4 63-39 63-39"></path></g></svg>
)"));
    wxSVGDocument dockerSvgDoc;
    wxStaticBitmap *dockerIcon = nullptr;
    if (dockerSvgDoc.Load(dockerSvgStream))
    {
        wxImage dockerSvgImage = dockerSvgDoc.Render(24, 24, nullptr, true, true);
        dockerIcon = new wxStaticBitmap(dockerPanel, wxID_ANY, wxBitmap(dockerSvgImage));
        dockerSizer->Add(dockerIcon, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    }
    wxFont iconFont = dockerIcon->GetFont();
    iconFont.SetPointSize(iconFont.GetPointSize() + 2);
    dockerIcon->SetFont(iconFont);
    wxStaticText *dockerText = new wxStaticText(dockerPanel, wxID_ANY, "Use Docker (Recommended)");
    dockerText->SetForegroundColour(m_textColor);
    m_dockerToggle = new ToggleSwitch(dockerPanel, wxID_ANY, true);
    dockerSizer->Add(dockerText, 1, wxALIGN_CENTER_VERTICAL);
    dockerSizer->Add(m_dockerToggle, 0, wxALIGN_CENTER_VERTICAL);
    dockerPanel->SetSizer(dockerSizer);
    containerSizer->Add(dockerPanel, 0, wxEXPAND | wxALL, 10);

#ifdef __WXMSW__
    wxPanel *wslPanel = new wxPanel(containerBox);
    wslPanel->SetBackgroundColour(m_bgColor);
    wxBoxSizer *wslSizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText *wslIcon = new wxStaticText(wslPanel, wxID_ANY, "\u2325");
    wslIcon->SetForegroundColour(m_textColor);
    wslIcon->SetFont(iconFont);
    wslIcon->SetBackgroundStyle(wxBG_STYLE_PAINT);
    wslIcon->SetBackgroundColour(m_bgColor);

    wxStaticText *wslText = new wxStaticText(wslPanel, wxID_ANY, "Use Windows Subsystem for Linux (WSL)");
    wslText->SetForegroundColour(m_textColor);
    m_wslToggle = new ToggleSwitch(wslPanel, wxID_ANY, false);
    wslSizer->Add(wslIcon, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    wslSizer->Add(wslText, 1, wxALIGN_CENTER_VERTICAL);
    wslSizer->Add(m_wslToggle, 0, wxALIGN_CENTER_VERTICAL);
    wslPanel->SetSizer(wslSizer);
    containerSizer->Add(wslPanel, 0, wxEXPAND | wxALL, 10);

    m_dockerToggle->Bind(EVT_TOGGLE_SWITCH, &SettingsDialog::OnToggleChanged, this);
    m_wslToggle->Bind(EVT_TOGGLE_SWITCH, &SettingsDialog::OnToggleChanged, this);
#endif

    mainSizer->Add(containerSizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(20);

    wxBoxSizer *buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    m_cancelButton = new wxButton(m_mainPanel, wxID_CANCEL, "Cancel",
                                  wxDefaultPosition, wxSize(100, 40), wxBORDER_NONE);
    m_cancelButton->SetBackgroundColour(m_secondaryColor);
    m_cancelButton->SetForegroundColour(*wxWHITE);
    m_okButton = new wxButton(m_mainPanel, wxID_OK, "OK",
                              wxDefaultPosition, wxSize(100, 40), wxBORDER_NONE);
    m_okButton->SetBackgroundColour(m_primaryColor);
    m_okButton->SetForegroundColour(*wxWHITE);
    buttonFont.SetWeight(wxFONTWEIGHT_BOLD);
    m_okButton->SetFont(buttonFont);
    m_cancelButton->SetFont(buttonFont);

    buttonSizer->Add(0, 0, 1, wxEXPAND);
    buttonSizer->Add(m_cancelButton, 0, wxRIGHT, 10);
    buttonSizer->Add(m_okButton, 0);
    mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 20);
    m_mainPanel->SetSizer(mainSizer);

    wxBoxSizer *dialogSizer = new wxBoxSizer(wxVERTICAL);
    dialogSizer->Add(m_mainPanel, 1, wxEXPAND);
    SetSizer(dialogSizer);
}

void SettingsDialog::OnThemeButtonClick(wxCommandEvent &event)
{
    wxObject *obj = event.GetEventObject();
    if (obj == m_lightButton)
    {
        m_lightButton->SetBackgroundColour(m_primaryColor);
        m_lightButton->SetForegroundColour(*wxWHITE);
        m_darkButton->SetBackgroundColour(m_bgColor);
        m_darkButton->SetForegroundColour(m_textColor);
        ApplyTheme("light");
    }
    else if (obj == m_darkButton)
    {
        m_darkButton->SetBackgroundColour(m_primaryColor);
        m_darkButton->SetForegroundColour(*wxWHITE);
        m_lightButton->SetBackgroundColour(m_bgColor);
        m_lightButton->SetForegroundColour(m_textColor);
        ApplyTheme("dark");
    }
}

void SettingsDialog::OnToggleChanged(wxCommandEvent &event)
{
#ifdef __WXMSW__
    wxObject *obj = event.GetEventObject();
    bool value = event.GetInt() != 0;
    if (obj == m_dockerToggle && value)
    {
        m_wslToggle->SetValue(false);
    }
    else if (obj == m_wslToggle && value)
    {
        m_dockerToggle->SetValue(false);
    }
    if (!m_dockerToggle->GetValue() && !m_wslToggle->GetValue())
    {
        if (obj == m_dockerToggle)
            m_wslToggle->SetValue(true);
        else
            m_dockerToggle->SetValue(true);
    }

#endif
}

void SettingsDialog::ApplyTheme(const wxString &theme)
{
}

wxString SettingsDialog::GetSelectedTheme() const
{
    return m_lightButton->GetBackgroundColour() == m_primaryColor ? "light" : "dark";
}

void SettingsDialog::OnPaint(wxPaintEvent &event)
{
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(wxBrush(m_bgColor));
    dc.Clear();

    wxGraphicsContext *gc = wxGraphicsContext::Create(dc);
    if (gc)
    {
        wxRect rect = GetClientRect();
        gc->SetBrush(wxBrush(m_bgColor));
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->DrawRoundedRectangle(rect.x, rect.y, rect.width, rect.height, 10);
        delete gc;
    }
}

void SettingsDialog::OnOK(wxCommandEvent &event)
{
    wxString selectedTheme = m_lightButton->GetBackgroundColour() == m_primaryColor ? "light" : "dark";
    ThemeConfig::Get().SetCurrentTheme(selectedTheme);

    wxFrame *mainFrame = wxDynamicCast(GetParent(), wxFrame);
    if (mainFrame)
    {
        ThemeConfig::Get().ApplyTheme(mainFrame, selectedTheme);

        for (wxWindow *child : mainFrame->GetChildren())
        {
            if (CustomTitleBar *titleBar = dynamic_cast<CustomTitleBar *>(child))
            {
                ThemeConfig::Get().ApplyTheme(titleBar, selectedTheme);
                break;
            }
        }
    }
    EndModal(wxID_OK);
}

void SettingsDialog::OnCancel(wxCommandEvent &event)
{
    EndModal(wxID_CANCEL);
}