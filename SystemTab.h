// SystemTab.h
#pragma once

#include <wx/wx.h>       // Core wxWidgets header
#include <wx/statbox.h>  // For wxStaticBoxSizer
#include <wx/checkbox.h> // For wxCheckBox
#include <wx/choice.h>   // For wxChoice
#include <wx/textctrl.h> // For wxTextCtrl
#include <wx/sizer.h>    // Include the main sizer header (should declare FlexGridSizer)

// --- Forward Declarations (if needed) ---

// --- Class Definition ---

class SystemTab : public wxPanel
{
public:
    /**
     * @brief Constructor for the SystemTab panel.
     * @param parent The parent window.
     */
    SystemTab(wxWindow *parent);

private:
    /**
     * @brief Creates all the UI controls within the tab.
     */
    void CreateControls();

    /**
     * @brief Applies the defined theme colors to the controls.
     *        (Should ideally fetch colors from a central theme manager).
     */
    void ApplyThemeColors();

    // --- Event Handlers ---

    /**
     * @brief Handles the event when the 'Lock root account' checkbox is toggled.
     *        Shows/hides the root password input fields.
     * @param event The command event.
     */
    void OnLockRootCheck(wxCommandEvent &event);

    // --- Control Pointers (Member Variables) ---

    // Localization Section Controls
    wxTextCtrl *m_hostnameInput;      // Input for the system hostname.
    wxChoice *m_languageChoice;       // Dropdown for system language.
    wxChoice *m_keyboardLayoutChoice; // Dropdown for keyboard layout.
    wxChoice *m_timezoneChoice;       // Dropdown for timezone.

    // Default User Account Section Controls
    wxTextCtrl *m_usernameInput;            // Input for the default username.
    wxTextCtrl *m_fullNameInput;            // Input for the user's full name.
    wxTextCtrl *m_userPasswordInput;        // Input for the user's password (masked).
    wxTextCtrl *m_confirmUserPasswordInput; // Confirmation input for user's password (masked).
    wxCheckBox *m_adminCheckbox;            // Checkbox to grant administrator privileges.

    // Root Account Section Controls
    wxCheckBox *m_lockRootCheckbox;           // Checkbox to disable root login.
    wxStaticText *m_rootPasswordLabel;        // Label for the root password field.
    wxTextCtrl *m_rootPasswordInput;          // Input for the root password (masked).
    wxStaticText *m_confirmRootPasswordLabel; // Label for root password confirmation.
    wxTextCtrl *m_confirmRootPasswordInput;   // Confirmation for root password (masked).
    wxFlexGridSizer *m_rootPasswordSizer;     // Sizer holding root password fields (to show/hide).

    // --- Styling Variables ---
    // Note: These should ideally be sourced from a central theme manager (like ThemeConfig)
    wxColour m_bgColor;      // Background color for the entire tab.
    wxColour m_panelBgColor; // Background color for sections/panels within the tab.
    wxColour m_textColor;    // Default text color for labels, checkbox text etc.
    wxColour m_inputBgColor; // Background color for wxTextCtrl, wxChoice.
    wxColour m_inputFgColor; // Foreground (text) color for wxTextCtrl, wxChoice.
    wxColour m_labelColor;   // Specific color for static text labels (often slightly dimmer).

    // --- Event Table Declaration ---
    // Declares that this class handles events. The actual table is defined in the .cpp file.
    wxDECLARE_EVENT_TABLE();
};