// WinTerminalManager.h
#pragma once          // Директива към компилатора да включи този заглавен файл само веднъж
#include <wx/wx.h>    // Включва основния заглавен файл на wxWidgets
#include <wx/panel.h> // Включва заглавния файл за wxPanel, който ще бъде родител на терминала
#include <windows.h>  // Включва заглавния файл за Windows API функции (за работа с процеси и прозорци)

class WinTerminalManager : public wxEvtHandler
{                                             // Декларация на клас WinTerminalManager, който наследява wxEvtHandler за обработка на събития
public:                                       // Секция с публични членове на класа
    WinTerminalManager(wxPanel *parentPanel); // Конструктор, приема указател към родителския wxPanel
    ~WinTerminalManager();                    // Деструктор

    bool Initialize();                             // Метод за инициализация на терминала, връща true при успех
    bool SendCommand(const std::wstring &command); // Нов метод за изпращане на команди към терминала, приема команда като широк низ

private:                      // Секция с частни членове на класа
    wxPanel *m_parentPanel;   // Указател към родителския wxPanel, в който ще бъде вграден терминалът
    PROCESS_INFORMATION m_pi; // Структура, съдържаща информация за новосъздадения процес (например cmd.exe)
    HWND m_hwndCmd;           // Манипулатор (handle) на прозореца на командния ред (cmd.exe)

    void CleanupProcess();                   // Частен метод за почистване на ресурсите, свързани със стартирания процес
    bool FindAndEmbedTerminal();             // Частен метод за намиране на прозореца на терминала и вграждането му
    void UpdateTerminalSize();               // Частен метод за актуализиране на размера на терминалния прозорец спрямо родителския панел
    void OnParentResize(wxSizeEvent &event); // Метод за обработка на събитието за преоразмеряване на родителския панел

    wxDECLARE_EVENT_TABLE(); // Макрос, който декларира таблицата със събития за този клас
}; // Край на декларацията на клас WinTerminalManager