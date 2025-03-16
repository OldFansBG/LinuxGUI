#include "FilesystemSelectionDialog.h"
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/checkbox.h>

wxBEGIN_EVENT_TABLE(FilesystemSelectionDialog, wxDialog)
    EVT_BUTTON(wxID_OK, FilesystemSelectionDialog::OnOK)
        EVT_BUTTON(wxID_CANCEL, FilesystemSelectionDialog::OnCancel)
            EVT_PAINT(FilesystemSelectionDialog::OnPaint)
                EVT_CHECKBOX(wxID_ANY, FilesystemSelectionDialog::OnItemCheckbox) // Event for checkboxes
    wxEND_EVENT_TABLE()

        FilesystemSelectionDialog::FilesystemSelectionDialog(wxWindow *parent, const wxString &title, const wxArrayString &choices)
    : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxSize(450, 500), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);

    wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);

    // Title Text with improved styling
    wxStaticText *titleText = new wxStaticText(this, wxID_ANY, title);
    wxFont titleFont = titleText->GetFont();
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);
    titleFont.SetPointSize(titleFont.GetPointSize() + 3);
    titleText->SetFont(titleFont);
    mainSizer->Add(titleText, 0, wxEXPAND | wxALL, 25);

    // Add description text
    wxStaticText *descText = new wxStaticText(this, wxID_ANY, "Please select a filesystem to mount:");
    wxFont descFont = descText->GetFont();
    descFont.SetPointSize(descFont.GetPointSize() + 1);
    descText->SetFont(descFont);
    mainSizer->Add(descText, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 25);

    // Panel to hold the list of items with rounded corners
    m_itemsPanel = new wxScrolledWindow(this, wxID_ANY);
    m_itemsPanel->SetBackgroundColour(wxColour(55, 65, 81)); // Darker background for contrast
    m_itemsPanel->SetScrollRate(0, 10);                      // Set vertical scroll rate
    m_itemsPanel->SetMinSize(wxSize(-1, 300));               // Set a reasonable default size to ensure scrollbars appear when needed
    wxBoxSizer *itemsSizer = new wxBoxSizer(wxVERTICAL);
    m_itemsPanel->SetSizer(itemsSizer);
    mainSizer->Add(m_itemsPanel, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 25);

    CreateItemList(choices);

    // OK and Cancel buttons with improved styling
    wxBoxSizer *buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton *okButton = new wxButton(this, wxID_OK, "Select", wxDefaultPosition, wxSize(100, 40), wxBORDER_NONE);
    wxButton *cancelButton = new wxButton(this, wxID_CANCEL, "Cancel", wxDefaultPosition, wxSize(100, 40), wxBORDER_NONE);

    okButton->SetBackgroundColour(wxColour(79, 70, 229)); // Indigo color
    okButton->SetForegroundColour(*wxWHITE);
    cancelButton->SetBackgroundColour(wxColour(75, 85, 99)); // Slate gray
    cancelButton->SetForegroundColour(*wxWHITE);

    wxFont buttonFont = okButton->GetFont();
    buttonFont.SetWeight(wxFONTWEIGHT_BOLD);
    buttonFont.SetPointSize(buttonFont.GetPointSize() + 1);
    okButton->SetFont(buttonFont);
    cancelButton->SetFont(buttonFont);

    // Add spacer to push buttons to the right
    buttonSizer->Add(0, 0, 1, wxEXPAND);
    buttonSizer->Add(cancelButton, 0, wxRIGHT, 15);
    buttonSizer->Add(okButton, 0);
    mainSizer->Add(buttonSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 25);

    SetSizerAndFit(mainSizer);
    ApplyTheme();
    SetupColors(); // Add this line
    CentreOnParent();
}

wxString FilesystemSelectionDialog::GetSelectedFilesystem() const
{
    return m_selectedFilesystem;
}

void FilesystemSelectionDialog::OnOK(wxCommandEvent &event)
{
    if (!m_selectedFilesystem.IsEmpty())
        EndModal(wxID_OK);
    else
        wxMessageBox("Please select a filesystem.", "Selection Required", wxOK | wxICON_WARNING, this);
}

void FilesystemSelectionDialog::OnCancel(wxCommandEvent &event)
{
    EndModal(wxID_CANCEL);
}

void FilesystemSelectionDialog::ApplyTheme()
{
    ThemeConfig &themeConfig = ThemeConfig::Get();
    const ThemeColors &colors = themeConfig.GetThemeColors(themeConfig.GetCurrentTheme());

    SetBackgroundColour(colors.panel.background);

    // Update our color system
    SetupColors();

    // Apply theme to all panels
    wxWindowList itemChildren = m_itemsPanel->GetChildren();
    for (wxWindow *child : itemChildren)
    {
        if (wxPanel *panel = wxDynamicCast(child, wxPanel))
        {
            panel->SetBackgroundColour(m_normalItemColor);

            // Apply to all children of the panel
            for (wxWindow *panelChild : panel->GetChildren())
            {
                if (wxCheckBox *cb = wxDynamicCast(panelChild, wxCheckBox))
                {
                    cb->SetForegroundColour(colors.text);
                    cb->SetBackgroundColour(m_normalItemColor);
                }
                else if (wxStaticText *text = wxDynamicCast(panelChild, wxStaticText))
                {
                    text->SetForegroundColour(colors.text);
                }
            }
        }
    }

    // Apply theme to buttons and other controls
    wxWindowList &children = GetChildren();
    for (wxWindow *child : children)
    {
        if (wxButton *button = wxDynamicCast(child, wxButton))
        {
            if (button->GetId() == wxID_OK)
            {
                button->SetBackgroundColour(wxColour(79, 70, 229)); // Keep primary button special
            }
            else
            {
                button->SetBackgroundColour(colors.button.background);
            }
            button->SetForegroundColour(colors.button.text);
        }
        else if (wxStaticText *staticText = wxDynamicCast(child, wxStaticText))
        {
            staticText->SetForegroundColour(colors.text);
        }
    }

    Refresh();
}

void FilesystemSelectionDialog::SetupColors()
{
    // Get theme colors
    ThemeConfig &themeConfig = ThemeConfig::Get();
    const ThemeColors &colors = themeConfig.GetThemeColors(themeConfig.GetCurrentTheme());

    // Set up base and hover colors
    m_normalItemColor = colors.input.background;

    // Create a slightly lighter version of the background for hover
    wxColour baseColor = m_normalItemColor;
    // Make hover color 20% lighter (but not exceeding 255)
    int r = std::min(255, baseColor.Red() + 20);
    int g = std::min(255, baseColor.Green() + 20);
    int b = std::min(255, baseColor.Blue() + 20);
    m_hoverItemColor = wxColour(r, g, b);

    // Apply to the scroll panel
    m_itemsPanel->SetBackgroundColour(m_normalItemColor);
}

void FilesystemSelectionDialog::OnPaint(wxPaintEvent &event)
{
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(wxBrush(GetBackgroundColour()));
    dc.Clear();

    wxGraphicsContext *gc = wxGraphicsContext::Create(dc);
    if (gc)
    {
        wxRect rect = GetClientRect();
        gc->SetBrush(wxBrush(GetBackgroundColour()));
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);
        gc->DrawRoundedRectangle(rect.x, rect.y, rect.width, rect.height, 15);

        // Draw a subtle rounded rectangle around the items panel
        wxRect itemsRect = m_itemsPanel->GetRect();
        gc->SetBrush(wxBrush(m_itemsPanel->GetBackgroundColour()));
        gc->SetPen(wxPen(wxColour(45, 55, 71), 1));
        gc->DrawRoundedRectangle(itemsRect.x, itemsRect.y, itemsRect.width, itemsRect.height, 10);

        delete gc;
    }
}

void FilesystemSelectionDialog::CreateItemList(const wxArrayString &choices)
{
    wxBoxSizer *itemsSizer = new wxBoxSizer(wxVERTICAL);
    m_itemsPanel->SetSizer(itemsSizer);
    m_checkboxes.clear();

    // Add padding at the top
    itemsSizer->AddSpacer(10);

    // Create panels for each item
    for (const wxString &choiceText : choices)
    {
        wxPanel *itemPanel = CreateItemPanel(m_itemsPanel, choiceText);
        itemsSizer->Add(itemPanel, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
        itemsSizer->AddSpacer(8); // Space between items
    }

    // Add padding at the bottom
    itemsSizer->AddSpacer(10);

    // Configure scrolling
    m_itemsPanel->FitInside();
    m_itemsPanel->SetScrollRate(0, 10);
}

wxPanel *FilesystemSelectionDialog::CreateItemPanel(wxWindow *parent, const wxString &choiceText)
{
    wxPanel *panel = new wxPanel(parent, wxID_ANY);
    panel->SetBackgroundColour(m_normalItemColor);
    panel->SetMinSize(wxSize(-1, 50));

    wxBoxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->AddSpacer(15);

    // Create checkbox
    wxCheckBox *checkBox = new wxCheckBox(panel, wxID_ANY, "");
    checkBox->SetForegroundColour(*wxWHITE);
    checkBox->SetBackgroundColour(panel->GetBackgroundColour());
    checkBox->SetClientData(new wxString(choiceText)); // Correctly set client data
    sizer->Add(checkBox, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 15);
    m_checkboxes.push_back(checkBox);

    // Create text label
    wxStaticText *text = new wxStaticText(panel, wxID_ANY, choiceText);
    text->SetForegroundColour(*wxWHITE);
    wxFont textFont = text->GetFont();
    textFont.SetPointSize(textFont.GetPointSize() + 1);
    text->SetFont(textFont);
    sizer->Add(text, 1, wxALIGN_CENTER_VERTICAL);

    // Add right padding
    sizer->AddSpacer(15);
    panel->SetSizer(sizer);

    // Bind checkbox event
    checkBox->Bind(wxEVT_CHECKBOX, &FilesystemSelectionDialog::OnItemCheckbox, this);

    // Enhanced hover effect with persistent tracking
    panel->Bind(wxEVT_ENTER_WINDOW, [this, panel](wxMouseEvent &event)
                {
        panel->SetBackgroundColour(m_hoverItemColor);
        panel->Refresh(true);
        event.Skip(); });

    panel->Bind(wxEVT_LEAVE_WINDOW, [this, panel](wxMouseEvent &event)
                {
        panel->SetBackgroundColour(m_normalItemColor);
        panel->Refresh(true);
        event.Skip(); });

    // Make checkbox children inherit hover events
    for (wxWindow *child : panel->GetChildren())
    {
        child->Bind(wxEVT_ENTER_WINDOW, [this, panel](wxMouseEvent &event)
                    {
            panel->SetBackgroundColour(m_hoverItemColor);
            panel->Refresh(true);
            event.Skip(); });

        child->Bind(wxEVT_LEAVE_WINDOW, [this, panel](wxMouseEvent &event)
                    {
            // Don't change color if mouse is actually moving between child and parent
            wxPoint mousePos = wxGetMousePosition();
            wxRect panelRect = panel->GetScreenRect();
            if (!panelRect.Contains(mousePos)) {
                panel->SetBackgroundColour(m_normalItemColor);
                panel->Refresh(true);
            }
            event.Skip(); });
    }

    // Make the entire panel clickable
    panel->Bind(wxEVT_LEFT_DOWN, [this, checkBox](wxMouseEvent &event)
                {
        checkBox->SetValue(!checkBox->GetValue());
        
        // Create and process a checkbox event manually
        wxCommandEvent checkEvent(wxEVT_CHECKBOX, checkBox->GetId());
        checkEvent.SetEventObject(checkBox);
        checkEvent.SetInt(checkBox->GetValue());
        ProcessEvent(checkEvent);
        
        event.Skip(); });

    return panel;
}

void FilesystemSelectionDialog::OnItemCheckbox(wxCommandEvent &event)
{
    wxCheckBox *clickedCheckBox = dynamic_cast<wxCheckBox *>(event.GetEventObject());
    if (!clickedCheckBox)
        return;

    if (clickedCheckBox->GetValue()) // If the clicked checkbox is now checked
    {
        m_selectedFilesystem = *static_cast<wxString *>(clickedCheckBox->GetClientData()); // Update selected filesystem

        for (wxCheckBox *cb : m_checkboxes) // Iterate through all checkboxes
        {
            if (cb != clickedCheckBox) // Uncheck other checkboxes
            {
                cb->SetValue(false);
            }
        }
    }
    else
    {
        m_selectedFilesystem = wxEmptyString; // No checkbox is selected
    }
    event.Skip();
}