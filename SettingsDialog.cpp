#include "SettingsDialog.h"
#include "CustomTitleBar.h" // For parent cast
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/stattext.h> // Ensure included

wxDEFINE_EVENT(EVT_TOGGLE_SWITCH, wxCommandEvent);

// --- ToggleSwitch Implementation (Same as before, OnPaint updated) ---
wxBEGIN_EVENT_TABLE(ToggleSwitch, wxControl)
    EVT_PAINT(ToggleSwitch::OnPaint)
        EVT_LEFT_DOWN(ToggleSwitch::OnMouse)
            wxEND_EVENT_TABLE()

                ToggleSwitch::ToggleSwitch(wxWindow *parent, wxWindowID id, bool initialState, const wxPoint &pos, const wxSize &size)
    : wxControl(parent, id, pos, size, wxBORDER_NONE), m_value(initialState),
      m_thumbColor(*wxWHITE) // Thumb is usually white
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(size);
    // Colors will be set dynamically in OnPaint based on theme
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
    wxAutoBufferedPaintDC dc(this); // Use buffered DC
    dc.SetBackground(*wxTRANSPARENT_BRUSH);
    dc.Clear();

    wxSize size = GetSize();
    int width = size.GetWidth();
    int height = size.GetHeight();
    int radius = height / 2;

    // --- Get current theme colors INSIDE OnPaint ---
    const ThemeColors &colors = ThemeConfig::Get().GetThemeColors(ThemeConfig::Get().GetCurrentTheme());
    m_trackOnColor = colors.button.background; // Use button background for ON state
    m_trackOffColor = colors.input.background; // Use input background for OFF state

    wxGraphicsContext *gc = wxGraphicsContext::Create(dc);
    if (gc)
    {
        gc->SetAntialiasMode(wxANTIALIAS_DEFAULT); // Enable anti-aliasing

        // Draw track
        gc->SetBrush(wxBrush(m_value ? m_trackOnColor : m_trackOffColor));
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->DrawRoundedRectangle(0, 0, width, height, radius);

        // Draw thumb
        gc->SetBrush(wxBrush(m_thumbColor));
        // Add a subtle border to the thumb for definition
        gc->SetPen(wxPen(colors.secondaryText, 1)); // Use secondary text color for border
        int thumbDiameter = height - 4;
        int thumbX = m_value ? width - thumbDiameter - 2 : 2;
        int thumbY = 2;
        gc->DrawEllipse(thumbX, thumbY, thumbDiameter, thumbDiameter);

        delete gc;
    }
}

void ToggleSwitch::OnMouse(wxMouseEvent &event)
{
    SetValue(!m_value);
}

// --- SettingsDialog Implementation ---
wxBEGIN_EVENT_TABLE(SettingsDialog, wxDialog)
    EVT_BUTTON(wxID_OK, SettingsDialog::OnOK)
        EVT_BUTTON(wxID_CANCEL, SettingsDialog::OnCancel)
            EVT_BUTTON(wxID_ANY, SettingsDialog::OnThemeButtonClick) // Catches both theme buttons
    EVT_PAINT(SettingsDialog::OnPaint)
#ifdef __WXMSW__
        EVT_COMMAND(wxID_ANY, EVT_TOGGLE_SWITCH, SettingsDialog::OnToggleChanged)
#endif
            wxEND_EVENT_TABLE()

                SettingsDialog::SettingsDialog(wxWindow *parent)
    : wxDialog(parent, wxID_ANY, "Settings",
               wxDefaultPosition, wxSize(500, 450),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxFRAME_SHAPED)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);

    // Initialize the dialog's theme state from the global config
    m_selectedThemeInDialog = ThemeConfig::Get().GetCurrentTheme();

    CreateContent();

    // Apply the initial theme to the dialog itself
    ApplyTheme(m_selectedThemeInDialog);

    Centre();
}

// Applies theme colors to the controls WITHIN the settings dialog
void SettingsDialog::ApplyTheme(const wxString &theme)
{
    ThemeConfig &config = ThemeConfig::Get();
    if (!config.HasTheme(theme))
    {
        wxLogError("SettingsDialog: Theme '%s' not found.", theme);
        return;
    }
    m_selectedThemeInDialog = theme.Lower(); // Update internal state

    const ThemeColors &colors = config.GetThemeColors(m_selectedThemeInDialog);

    // --- Update Dialog's Cached Colors ---
    m_bgColor = colors.background;
    m_textColor = colors.text;
    m_labelColor = colors.secondaryText;
    m_primaryColor = colors.button.background;
    m_secondaryColor = colors.input.background;
    m_inputBgColor = colors.input.background;
    m_inputFgColor = colors.input.text;

    // --- Apply to Dialog and Main Panel ---
    SetBackgroundColour(m_bgColor);
    if (m_mainPanel)
        m_mainPanel->SetBackgroundColour(m_bgColor);

    // --- Apply to Specific Controls ---
    if (m_titleText)
        m_titleText->SetForegroundColour(m_textColor);

    // Theme Section Panel and Label
    wxWindow *themePanel = m_lightButton ? m_lightButton->GetParent() : nullptr;
    if (themePanel)
    {
        themePanel->SetBackgroundColour(m_bgColor); // Match dialog background
        wxWindowList children = themePanel->GetChildren();
        for (wxWindow *child : children)
        {
            if (auto *label = dynamic_cast<wxStaticText *>(child))
            {
                label->SetForegroundColour(m_labelColor);
                label->SetBackgroundColour(m_bgColor); // Match panel background
            }
            // Apply to button sizer panel if nested
            else if (auto *panel = dynamic_cast<wxPanel *>(child))
            {
                panel->SetBackgroundColour(m_bgColor);
            }
            else if (auto *sizer = dynamic_cast<wxBoxSizer *>(child->GetSizer()))
            {
                // Could iterate sizer items if needed, but direct children usually sufficient
            }
        }
    }

    // Container Section Box and Inner Panel
    if (m_containerSizer)
    {
        wxStaticBox *box = m_containerSizer->GetStaticBox();
        if (box)
        {
            box->SetForegroundColour(m_labelColor);
            // Find the inner panel to set its background
            wxWindowList boxChildren = box->GetChildren();
            if (!boxChildren.IsEmpty())
            {
                wxPanel *innerPanel = dynamic_cast<wxPanel *>(boxChildren.Item(0)->GetData());
                if (innerPanel)
                {
                    innerPanel->SetBackgroundColour(m_bgColor); // Match dialog BG
                                                                // Style controls inside the inner panel
                    wxSizer *innerSizer = innerPanel->GetSizer();
                    if (innerSizer)
                    {
                        wxSizerItemList items = innerSizer->GetChildren();
                        for (wxSizerItem *item : items)
                        {
                            wxPanel *rowPanel = dynamic_cast<wxPanel *>(item->GetWindow()); // Panels for Docker/WSL rows
                            if (rowPanel)
                            {
                                rowPanel->SetBackgroundColour(m_bgColor);
                                wxSizer *rowSizer = rowPanel->GetSizer();
                                if (rowSizer)
                                {
                                    wxSizerItemList rowItems = rowSizer->GetChildren();
                                    for (wxSizerItem *rowItem : rowItems)
                                    {
                                        wxWindow *widget = rowItem->GetWindow();
                                        if (auto *text = dynamic_cast<wxStaticText *>(widget))
                                        {
                                            text->SetForegroundColour(m_textColor);
                                            text->SetBackgroundColour(m_bgColor);
                                        }
                                        else if (auto *toggle = dynamic_cast<ToggleSwitch *>(widget))
                                        {
                                            toggle->Refresh(); // Toggle redraws using current theme
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // OK/Cancel Buttons
    if (m_okButton)
    {
        m_okButton->SetBackgroundColour(m_primaryColor);
        m_okButton->SetForegroundColour(*wxWHITE);
    }
    if (m_cancelButton)
    {
        m_cancelButton->SetBackgroundColour(m_secondaryColor);
        m_cancelButton->SetForegroundColour(m_textColor);
    }

    // --- Update Theme Button Appearance ---
    if (m_lightButton && m_darkButton)
    {
        if (m_selectedThemeInDialog == "light")
        {
            m_lightButton->SetBackgroundColour(m_primaryColor); // Active button uses primary
            m_lightButton->SetForegroundColour(*wxWHITE);
            m_darkButton->SetBackgroundColour(m_bgColor); // Inactive button matches dialog BG
            m_darkButton->SetForegroundColour(m_textColor);
        }
        else
        {                                                      // Dark or other themes
            m_darkButton->SetBackgroundColour(m_primaryColor); // Active button uses primary
            m_darkButton->SetForegroundColour(*wxWHITE);
            m_lightButton->SetBackgroundColour(m_bgColor); // Inactive button matches dialog BG
            m_lightButton->SetForegroundColour(m_textColor);
        }
        m_lightButton->Refresh();
        m_darkButton->Refresh();
    }

    // --- Refresh Layout and Drawing ---
    if (m_mainPanel)
        m_mainPanel->Layout();
    Layout();
    Refresh();
    Update(); // Ensures immediate redraw
}

// CreateContent remains largely the same, but remove direct color settings
void SettingsDialog::CreateContent()
{
    // Get initial colors just for reference if needed, but don't apply here
    const ThemeColors &initialColors = ThemeConfig::Get().GetThemeColors(m_selectedThemeInDialog);
    // Example: wxColour initialBg = initialColors.background;

    m_mainPanel = new wxPanel(this);
    wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);

    m_titleText = new wxStaticText(m_mainPanel, wxID_ANY, "Settings");
    wxFont titleFont = m_titleText->GetFont();
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);
    titleFont.SetPointSize(titleFont.GetPointSize() + 3);
    m_titleText->SetFont(titleFont);
    mainSizer->Add(m_titleText, 0, wxALL | wxEXPAND, 20);

    wxPanel *themePanel = new wxPanel(m_mainPanel);
    wxBoxSizer *themeSizer = new wxBoxSizer(wxVERTICAL);
    wxStaticText *themeLabel = new wxStaticText(themePanel, wxID_ANY, "Theme");
    themeSizer->Add(themeLabel, 0, wxLEFT | wxTOP | wxRIGHT, 10);
    wxBoxSizer *themeButtonSizer = new wxBoxSizer(wxHORIZONTAL);
    m_lightButton = new wxButton(themePanel, wxID_ANY, "Light", wxDefaultPosition, wxSize(160, 45), wxBORDER_NONE);
    m_darkButton = new wxButton(themePanel, wxID_ANY, "Dark", wxDefaultPosition, wxSize(160, 45), wxBORDER_NONE);
    wxFont buttonFont = m_lightButton->GetFont();
    buttonFont.SetWeight(wxFONTWEIGHT_BOLD);
    m_lightButton->SetFont(buttonFont);
    m_darkButton->SetFont(buttonFont);
    themeButtonSizer->Add(m_lightButton, 1, wxEXPAND | wxRIGHT, 5);
    themeButtonSizer->Add(m_darkButton, 1, wxEXPAND | wxLEFT, 5);
    themeSizer->Add(themeButtonSizer, 0, wxEXPAND | wxALL, 10);
    themePanel->SetSizer(themeSizer);
    mainSizer->Add(themePanel, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);

    mainSizer->AddSpacer(15);

    m_containerSizer = new wxStaticBoxSizer(wxVERTICAL, m_mainPanel, "Container Options");
    wxStaticBox *containerBox = m_containerSizer->GetStaticBox();
    wxPanel *containerInnerPanel = new wxPanel(containerBox);
    wxBoxSizer *containerInnerSizer = new wxBoxSizer(wxVERTICAL);
    wxPanel *dockerPanel = new wxPanel(containerInnerPanel);
    wxBoxSizer *dockerSizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText *dockerText = new wxStaticText(dockerPanel, wxID_ANY, "Use Docker (Recommended)");
    m_dockerToggle = new ToggleSwitch(dockerPanel, wxID_ANY, true);
    dockerSizer->Add(dockerText, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    dockerSizer->Add(m_dockerToggle, 0, wxALIGN_CENTER_VERTICAL);
    dockerPanel->SetSizer(dockerSizer);
    containerInnerSizer->Add(dockerPanel, 0, wxEXPAND | wxBOTTOM, 5);

#ifdef __WXMSW__
    wxPanel *wslPanel = new wxPanel(containerInnerPanel);
    wxBoxSizer *wslSizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText *wslText = new wxStaticText(wslPanel, wxID_ANY, "Use Windows Subsystem for Linux (WSL)");
    m_wslToggle = new ToggleSwitch(wslPanel, wxID_ANY, false);
    wslSizer->Add(wslText, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    wslSizer->Add(m_wslToggle, 0, wxALIGN_CENTER_VERTICAL);
    wslPanel->SetSizer(wslSizer);
    containerInnerSizer->Add(wslPanel, 0, wxEXPAND);
    m_dockerToggle->Bind(EVT_TOGGLE_SWITCH, &SettingsDialog::OnToggleChanged, this);
    m_wslToggle->Bind(EVT_TOGGLE_SWITCH, &SettingsDialog::OnToggleChanged, this);
#endif

    containerInnerPanel->SetSizer(containerInnerSizer);
    m_containerSizer->Add(containerInnerPanel, 1, wxEXPAND | wxALL, 10);
    mainSizer->Add(m_containerSizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);

    mainSizer->AddStretchSpacer(1);

    wxBoxSizer *buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    m_cancelButton = new wxButton(m_mainPanel, wxID_CANCEL, "Cancel", wxDefaultPosition, wxSize(100, 40), wxBORDER_NONE);
    m_okButton = new wxButton(m_mainPanel, wxID_OK, "OK", wxDefaultPosition, wxSize(100, 40), wxBORDER_NONE);
    m_okButton->SetFont(buttonFont);
    m_cancelButton->SetFont(buttonFont);
    buttonSizer->AddStretchSpacer(1);
    buttonSizer->Add(m_cancelButton, 0, wxRIGHT, 10);
    buttonSizer->Add(m_okButton, 0);
    mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 20);

    m_mainPanel->SetSizer(mainSizer);
    wxBoxSizer *dialogSizer = new wxBoxSizer(wxVERTICAL);
    dialogSizer->Add(m_mainPanel, 1, wxEXPAND);
    SetSizer(dialogSizer);
    Layout();
}

void SettingsDialog::OnThemeButtonClick(wxCommandEvent &event)
{
    wxObject *obj = event.GetEventObject();
    wxString newTheme;
    if (obj == m_lightButton)
    {
        newTheme = "light";
    }
    else if (obj == m_darkButton)
    {
        newTheme = "dark";
    }
    else
    {
        return; // Not a theme button
    }

    // Only apply if the theme actually changed within the dialog's context
    if (newTheme != m_selectedThemeInDialog)
    {
        ApplyTheme(newTheme);
    }
}

void SettingsDialog::OnToggleChanged(wxCommandEvent &event)
{
#ifdef __WXMSW__
    wxObject *obj = event.GetEventObject();
    bool value = event.GetInt() != 0;

    if (!m_dockerToggle || !m_wslToggle)
        return; // Safety

    // Mutual Exclusivity Logic: If one is turned ON, turn the other OFF.
    if (obj == m_dockerToggle && value)
    {
        m_wslToggle->SetValue(false);
    }
    else if (obj == m_wslToggle && value)
    {
        m_dockerToggle->SetValue(false);
    }
    // Prevent both being turned OFF.
    // If the user action *would* result in both being off, revert the action
    // by setting the one they just interacted with back to true.
    else if (!value && !m_dockerToggle->GetValue() && !m_wslToggle->GetValue())
    {
        // This state should ideally not be reached if the above logic works,
        // but as a fallback, force one back on.
        if (obj == m_dockerToggle)
            m_dockerToggle->SetValue(true); // Revert docker toggle
        else if (obj == m_wslToggle)
            m_wslToggle->SetValue(true); // Revert wsl toggle
    }

#endif
}

// Returns the theme currently selected *in the dialog*, not necessarily the global one yet
wxString SettingsDialog::GetSelectedTheme() const
{
    return m_selectedThemeInDialog;
}

bool SettingsDialog::UseDocker() const
{
    return m_dockerToggle ? m_dockerToggle->GetValue() : true;
}

bool SettingsDialog::UseWSL() const
{
#ifdef __WXMSW__
    return m_wslToggle ? m_wslToggle->GetValue() : false;
#else
    return false;
#endif
}

void SettingsDialog::OnPaint(wxPaintEvent &event)
{
    wxAutoBufferedPaintDC dc(this);
    // Use colors based on the dialog's *currently selected* theme state
    const ThemeColors &colors = ThemeConfig::Get().GetThemeColors(m_selectedThemeInDialog);
    dc.SetBackground(wxBrush(colors.background));
    dc.Clear();

    // Optional rounded corners
    wxGraphicsContext *gc = wxGraphicsContext::Create(dc);
    if (gc)
    {
        gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);
        wxRect rect = GetClientRect();
        gc->SetBrush(wxBrush(colors.background));
        gc->SetPen(*wxTRANSPARENT_PEN);
        // gc->DrawRoundedRectangle(rect.x, rect.y, rect.width, rect.height, 10); // Uncomment if desired
        delete gc;
    }
}

void SettingsDialog::OnOK(wxCommandEvent &event)
{
    // 1. Get the theme selected *in the dialog* when OK was clicked
    wxString finalSelectedTheme = GetSelectedTheme();

    // 2. Update the global ThemeConfig state
    ThemeConfig::Get().SetCurrentTheme(finalSelectedTheme);

    // 3. Apply the final theme globally (to parent and potentially other registered windows)
    wxWindow *parent = GetParent();
    if (parent)
    {
        // This will trigger ApplyTheme on MainFrame and its children (including TitleBar)
        ThemeConfig::Get().ApplyTheme(parent, finalSelectedTheme);
    }
    // Optionally trigger an update for all other registered windows if needed
    // ThemeConfig::Get().UpdateSystemTheme(); // Or a similar method if not tied to system event

    // 4. Close the dialog
    EndModal(wxID_OK);
}

void SettingsDialog::OnCancel(wxCommandEvent &event)
{
    // Discard changes made within the dialog
    EndModal(wxID_CANCEL);
}