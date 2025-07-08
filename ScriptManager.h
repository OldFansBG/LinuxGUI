// ScriptManager.h // ScriptManager.h
#ifndef SCRIPT_MANAGER_H // Ако SCRIPT_MANAGER_H не е дефиниран
#define SCRIPT_MANAGER_H // Дефинирай SCRIPT_MANAGER_H (предпазител срещу многократно включване)

#include <wx/wx.h> // Включва основната библиотека на wxWidgets
#include <map>     // Включва библиотека за работа с асоциативни масиви (std::map)

class ScriptManager
{                                // Декларация на клас ScriptManager
public:                          // Публична секция
    static ScriptManager &Get(); // Статичен метод за получаване на единствената инстанция на ScriptManager (Singleton)

    bool ExtractScriptToTemp(const wxString &scriptName);         // Метод за извличане на скрипт във временен файл, връща bool (успех/неуспех)
    wxString GetTempScriptPath(const wxString &scriptName) const; // Метод за получаване на пътя до временния файл на скрипта

private:                                           // Частна секция
    ScriptManager();                               // Частен конструктор (за Singleton модела)
    wxString m_tempDir;                            // Променлива за съхранение на пътя до временната директория
    std::map<wxString, wxString> m_scriptContents; // Асоциативен масив за съхранение на съдържанието на скриптовете (име на скрипт -> съдържание)

    bool WriteScriptToFile(const wxString &path, const wxString &content); // Частен метод за записване на съдържанието на скрипт във файл
    bool EnsureTempDirectory();                                            // Частен метод за гарантиране на съществуването на временната директория
    void InitializeScripts();                                              // Частен метод за инициализация на скриптовете (напр. зареждане на съдържанието им)
}; // Край на тялото на класа ScriptManager

#endif // SCRIPT_MANAGER_H // Край на предпазителя срещу многократно включване