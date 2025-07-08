// SettingsDialog.h
#ifndef SETTINGSDIALOG_H // Ако SETTINGSDIALOG_H не е дефиниран
#define SETTINGSDIALOG_H // Дефинирай SETTINGSDIALOG_H (предпазител срещу многократно включване)

#include <wx/wx.h>       // Включва основната библиотека на wxWidgets
#include <wx/statbox.h>  // Включва библиотека за статични кутии (wxStaticBox)
#include <wx/checkbox.h> // Включва библиотека за чекбоксове (wxCheckBox), въпреки че не се използва директно за ToggleSwitch
#include "ThemeConfig.h" // Включва заглавния файл ThemeConfig.h (вероятно дефинира конфигурация на темата)

// --- ToggleSwitch Declaration --- // --- Декларация на ToggleSwitch ---
class ToggleSwitch : public wxControl                                                          // Декларация на клас ToggleSwitch, наследяващ wxControl
{                                                                                              // Начало на тялото на класа ToggleSwitch
public:                                                                                        // Публична секция
    ToggleSwitch(wxWindow *parent, wxWindowID id = wxID_ANY, bool initialState = false,        // Конструктор на ToggleSwitch
                 const wxPoint &pos = wxDefaultPosition, const wxSize &size = wxSize(50, 24)); // Параметри на конструктора: родител, ID, начално състояние, позиция, размер
    bool GetValue() const { return m_value; }                                                  // Метод за получаване на текущата стойност на превключвателя
    void SetValue(bool value);                                                                 // Метод за задаване на стойността на превключвателя

private:                               // Частна секция
    bool m_value;                      // Променлива за съхранение на текущата стойност (включено/изключено)
    wxColour m_trackOnColor;           // Colors will be set based on theme // Цвят на "пистата" при включено състояние (цветовете ще се задават въз основа на темата)
    wxColour m_trackOffColor;          // Цвят на "пистата" при изключено състояние
    wxColour m_thumbColor;             // Цвят на "палеца" (плъзгача)
    void OnPaint(wxPaintEvent &event); // Обработчик на събитието за прерисуване
    void OnMouse(wxMouseEvent &event); // Обработчик на събития от мишката
    wxDECLARE_EVENT_TABLE();           // Макрос за деклариране на таблицата на събитията
}; // Край на тялото на класа ToggleSwitch

// --- SettingsDialog Declaration --- // --- Декларация на SettingsDialog ---
class SettingsDialog : public wxDialog         // Декларация на клас SettingsDialog, наследяващ wxDialog
{                                              // Начало на тялото на класа SettingsDialog
public:                                        // Публична секция
    explicit SettingsDialog(wxWindow *parent); // Експлицитен конструктор, приемащ родителски прозорец
    wxString GetSelectedTheme() const;         // Returns the theme selected *within* the dialog // Метод за получаване на избраната тема (връща темата, избрана *вътре* в диалога)
    bool UseDocker() const;                    // Метод за проверка дали е избран Docker
    bool UseWSL() const;                       // Метод за проверка дали е избран WSL

private: // Частна секция
    // Event Handlers // Обработчици на събития
    void OnOK(wxCommandEvent &event);               // Обработчик на събитието при натискане на бутон "OK"
    void OnCancel(wxCommandEvent &event);           // Обработчик на събитието при натискане на бутон "Cancel"
    void OnThemeButtonClick(wxCommandEvent &event); // Обработчик на събитието при натискане на бутон за избор на тема
    void OnToggleChanged(wxCommandEvent &event);    // Обработчик на събитието при промяна на ToggleSwitch
    void OnPaint(wxPaintEvent &event);              // Обработчик на събитието за прерисуване на диалога

    // UI Creation & Theme Application // Създаване на UI и прилагане на тема
    void CreateContent();                   // Метод за създаване на съдържанието (контролите) на диалога
    void ApplyTheme(const wxString &theme); // Applies theme to dialog controls // Метод за прилагане на тема към контролите на диалога

    // UI Elements References // Референции към UI елементи
    wxPanel *m_mainPanel;               // Указател към главния панел на диалога
    wxStaticText *m_titleText;          // Указател към статичния текст за заглавие
    wxStaticBoxSizer *m_containerSizer; // Keep if needed for styling box // Указател към сайзер за контейнери (запазете, ако е нужен за стилизиране на кутията)
    wxButton *m_lightButton;            // Указател към бутона за светла тема
    wxButton *m_darkButton;             // Указател към бутона за тъмна тема
    ToggleSwitch *m_dockerToggle;       // Указател към превключвателя за Docker
#ifdef __WXMSW__                        // Ако се компилира за Windows (MSW - Microsoft Windows)
    ToggleSwitch *m_wslToggle;          // Указател към превключвателя за WSL (наличен само за Windows)
#endif                                  // Край на условната компилация
    wxButton *m_cancelButton;           // Указател към бутона "Cancel"
    wxButton *m_okButton;               // Указател към бутона "OK"

    // State within the dialog // Състояние вътре в диалога
    wxString m_selectedThemeInDialog; // Tracks selection before OK is pressed // Променлива за проследяване на избраната тема преди натискане на "OK"

    // Cached theme colors (optional, could get directly from ThemeConfig) // Кеширани цветове на темата (опционално, могат да се вземат директно от ThemeConfig)
    wxColour m_bgColor;        // Цвят на фона
    wxColour m_textColor;      // Цвят на текста
    wxColour m_labelColor;     // Цвят на етикетите
    wxColour m_primaryColor;   // Основен цвят
    wxColour m_secondaryColor; // Вторичен цвят
    wxColour m_inputBgColor;   // Цвят на фона на полета за въвеждане
    wxColour m_inputFgColor;   // Цвят на текста в полета за въвеждане

    wxDECLARE_EVENT_TABLE(); // Макрос за деклариране на таблицата на събитията
}; // Край на тялото на класа SettingsDialog

wxDECLARE_EVENT(EVT_TOGGLE_SWITCH, wxCommandEvent); // Деклариране на нов тип събитие EVT_TOGGLE_SWITCH, което е wxCommandEvent

#endif // SETTINGSDIALOG_H // Край на предпазителя срещу многократно включване