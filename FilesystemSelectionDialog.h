#ifndef FILESYSTEM_SELECTION_DIALOG_H // Предпазител за включване: ако FILESYSTEM_SELECTION_DIALOG_H не е дефиниран,
#define FILESYSTEM_SELECTION_DIALOG_H // Дефинира FILESYSTEM_SELECTION_DIALOG_H, за да предотврати многократно включване.

#include <wx/wx.h>       // Включва основния заглавен файл на wxWidgets, който включва много често използвани класове.
#include <wx/panel.h>    // Включва заглавния файл за wxPanel (основен контейнер за контроли).
#include <wx/stattext.h> // Включва заглавния файл за wxStaticText (статичен текстов етикет).
#include <wx/checkbox.h> // Use wxCheckBox to simulate radio behavior // Включва заглавния файл за wxCheckBox (квадратче за отметка), използвано за симулиране на поведение на радио бутони.
#include <wx/scrolwin.h> // Add this include // Включва заглавния файл за wxScrolledWindow (прозорец с възможност за превъртане).
#include "ThemeConfig.h" // Включва заглавния файл за конфигурация на темата (ThemeConfig).

class FilesystemSelectionDialog : public wxDialog                                                     // Декларира клас FilesystemSelectionDialog, който наследява wxDialog.
{                                                                                                     // Начало на тялото на класа.
public:                                                                                               // Начало на публичната секция на класа.
    FilesystemSelectionDialog(wxWindow *parent, const wxString &title, const wxArrayString &choices); // Конструктор на класа, приема родителски прозорец, заглавие и масив от низове за избор.
    wxString GetSelectedFilesystem() const;                                                           // Публичен метод за получаване на избраната файлова система (константен метод).

private:                                               // Начало на частната секция на класа.
    void OnOK(wxCommandEvent &event);                  // Частен метод - обработчик на събитието за натискане на бутона "OK".
    void OnCancel(wxCommandEvent &event);              // Частен метод - обработчик на събитието за натискане на бутона "Cancel".
    void ApplyTheme();                                 // Частен метод за прилагане на визуалната тема.
    void CreateItemList(const wxArrayString &choices); // Частен метод за създаване на списъка с елементи за избор.
    void OnItemCheckbox(wxCommandEvent &event);        // Handler for checkbox clicks // Частен метод - обработчик за кликвания върху квадратчета за отметка.
    void OnPaint(wxPaintEvent &event);                 // Add this line // Частен метод - обработчик на събитието за прерисуване.

    wxScrolledWindow *m_itemsPanel;         // Change Panel to ScrolledWindow // Указател към wxScrolledWindow, който ще съдържа елементите за избор.
    wxString m_selectedFilesystem;          // Низ, съхраняващ името на избраната файлова система.
    std::vector<wxCheckBox *> m_checkboxes; // Store checkboxes for single selection logic // Вектор от указатели към wxCheckBox, за реализиране на логиката за единичен избор.

    wxColour m_normalItemColor; // Base color for items // Цвят за елементите в нормално състояние.
    wxColour m_hoverItemColor;  // Hover color for items // Цвят за елементите при посочване с мишката.

    void SetupColors();                                                     // Setup theme-aware colors // Частен метод за настройка на цветове, съобразени с темата.
    wxPanel *CreateItemPanel(wxWindow *parent, const wxString &choiceText); // Helper to create item panels // Помощен частен метод за създаване на панелите на отделните елементи.

    wxDECLARE_EVENT_TABLE(); // Макрос за деклариране на таблицата със събития за този клас.
}; // Край на тялото на класа.

#endif // FILESYSTEM_SELECTION_DIALOG_H // Край на предпазителя за включване.