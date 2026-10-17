#pragma once // Предпазител от многократно включване на хедър файла

#include <wx/wx.h>       // Основни wxWidgets класове
#include <wx/statbox.h>  // wxStaticBox
#include <wx/checkbox.h> // wxCheckBox
#include <wx/choice.h>   // wxChoice
#include <wx/textctrl.h> // wxTextCtrl
#include <wx/sizer.h>    // Сайзери

// Таб "System": системни настройки на ISO образа (hostname, локализация, потребител, root акаунт).
// Текущите стойности се четат от chroot средата в контейнера, а бутонът "Apply to ISO" ги записва там.
class SystemTab : public wxPanel
{
public:
    // projectDir: директорията на проекта (съдържа container_id.txt)
    SystemTab(wxWindow *parent, const wxString &projectDir = wxEmptyString);

private:
    void CreateControls();               // Създава контролите
    void ApplyThemeColors();             // Прилага тъмната цветова схема
    void LoadSettings();                 // Чете текущите стойности от chroot средата
    void UpdateRootPasswordVisibility(); // Показва полетата за root парола само ако root не е заключен

    void OnApply(wxCommandEvent &event);         // Записва настройките в ISO образа
    void OnLockRootCheck(wxCommandEvent &event); // Превключва полетата за root парола

    bool ValidateInput(wxString &error) const; // Проверява въведените данни
    wxString BuildApplyScript() const;         // Bash скрипт, който прилага настройките в chroot

    static void SelectByPrefix(wxChoice *choice, const wxString &value); // Избира елемента, който започва с value
    static wxString ChoiceValue(const wxChoice *choice);                 // Стойността преди първия интервал

    // --- Контроли ---
    wxTextCtrl *m_hostnameInput;
    wxChoice *m_languageChoice;
    wxChoice *m_keyboardLayoutChoice;
    wxChoice *m_timezoneChoice;
    wxTextCtrl *m_usernameInput;
    wxTextCtrl *m_fullNameInput;
    wxTextCtrl *m_userPasswordInput;
    wxTextCtrl *m_confirmUserPasswordInput;
    wxCheckBox *m_adminCheckbox;
    wxCheckBox *m_lockRootCheckbox;
    wxStaticText *m_rootPasswordLabel;
    wxTextCtrl *m_rootPasswordInput;
    wxStaticText *m_confirmRootPasswordLabel;
    wxTextCtrl *m_confirmRootPasswordInput;
    wxFlexGridSizer *m_rootPasswordSizer;
    wxButton *m_applyButton;

    wxString m_projectDir; // Директория на проекта

    // --- Цветове ---
    wxColour m_bgColor;
    wxColour m_panelBgColor;
    wxColour m_textColor;
    wxColour m_inputBgColor;
    wxColour m_inputFgColor;
    wxColour m_labelColor;

    wxDECLARE_EVENT_TABLE();
};
