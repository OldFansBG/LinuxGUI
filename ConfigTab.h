#ifndef SQLTAB_H // Предпроцесорна директива, проверява дали SQLTAB_H не е дефиниран
#define SQLTAB_H // Дефинира SQLTAB_H, за да се избегне многократно включване на хедър файла

#include <wx/wx.h>        // Включва основния хедър файл на wxWidgets
#include <vector>         // Включва хедър файла за std::vector от стандартната библиотека
#include "FlatpakStore.h" // Включва заглавния файл за класа FlatpakStore
#include "DesktopTab.h"   // Включва заглавния файл за класа DesktopTab
#include "CustomizeTab.h" // Включва новия заглавен файл за CustomizeTab // Включва заглавния файл за класа CustomizeTab

class SQLTab : public wxPanel
{                                                      // Декларация на клас SQLTab, който наследява публично от wxPanel
public:                                                // Начало на публичната секция на класа
    SQLTab(wxWindow *parent, const wxString &workDir); // Декларация на конструктора, който приема указател към родителски прозорец и работна директория
    void ShowTab(int tabId);                           // Декларация на метод за показване на таб по неговото ID
    void SetContainerId(const wxString &containerId);  // Декларация на метод за задаване на ID на контейнера

private:                                            // Начало на частната секция на класа
    void CreateDesktopTab();                        // Декларация на метод за създаване на съдържанието на "Desktop" таба
    void CreateAppsTab();                           // Декларация на метод за създаване на съдържанието на "Applications" таба
    void CreateSystemTab();                         // Декларация на метод за създаване на съдържанието на "System" таба
    void CreateCustomizeTab();                      // Запазва декларацията // Декларация на метод за създаване на съдържанието на "Customize" таба
    void OnConfigTabChanged(wxCommandEvent &event); // Декларация на метод за обработка на събитие при смяна на конфигурационен таб

    wxString m_containerId; // Член-променлива за съхранение на ID-то на контейнера за FlatpakStore
    wxString m_workDir;     // Член-променлива за съхранение на работната директория за FlatpakStore

    wxPanel *m_configContent;                   // Указател към wxPanel, който ще съдържа панела със съдържанието на активния таб
    int m_currentConfigTab;                     // Член-променлива за съхранение на ID-то на текущо активния конфигурационен таб
    std::vector<wxButton *> m_configTabButtons; // Вектор от указатели към wxButton, използвани за превключване между табовете

    // Идентификатори на табовете
    enum
    {                                         // Декларация на анонимно изброяване за дефиниране на ID-та на табовете
        ID_CONFIG_DESKTOP = wxID_HIGHEST + 1, // ID за "Desktop" таба, започва от wxID_HIGHEST + 1, за да се избегнат конфликти с вградените ID-та на wxWidgets
        ID_CONFIG_APPS,                       // ID за "Applications" таба (стойността ще бъде wxID_HIGHEST + 2)
        ID_CONFIG_SYSTEM,                     // ID за "System" таба (стойността ще бъде wxID_HIGHEST + 3)
        ID_CONFIG_CUSTOMIZE                   // ID за "Customize" таба (стойността ще бъде wxID_HIGHEST + 4)
    }; // Край на изброяването

    DECLARE_EVENT_TABLE() // Макрос за деклариране на таблицата със събития за този клас
}; // Край на дефиницията на класа SQLTab

#endif // SQLTAB_H // Край на условната директива #ifndef