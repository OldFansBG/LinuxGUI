// ScriptManager.cpp // Име на файла на изходния код
#include "ScriptManager.h" // Включва заглавния файл за ScriptManager
#include <wx/filename.h>   // Включва заглавния файл на wxWidgets за работа с имена на файлове
#include <wx/dir.h>        // Включва заглавния файл на wxWidgets за работа с директории
                           // Празна линия за по-добра четимост
// Include embedded script contents // Коментар: Указва, че следват декларации на вградено съдържание на скриптове
extern const char *SETUP_CHROOT_SCRIPT; // Декларира външна променлива (указател към низ) за съдържанието на скрипта SETUP_CHROOT_SCRIPT
extern const char *CREATE_ISO_SCRIPT;   // Декларира външна променлива (указател към низ) за съдържанието на скрипта CREATE_ISO_SCRIPT
extern const char *SETUP_OUTPUT_SCRIPT; // Декларира външна променлива (указател към низ) за съдържанието на скрипта SETUP_OUTPUT_SCRIPT
                                        // Празна линия за по-добра четимост
ScriptManager &ScriptManager::Get()
{                                  // Дефиниция на статичен метод Get, който връща референция към единствената инстанция на ScriptManager (шаблон Сингълтън)
    static ScriptManager instance; // Декларира статична локална инстанция на ScriptManager; създава се само веднъж при първото извикване
    return instance;               // Връща референция към единствената инстанция
} // Край на метод Get
  // Празна линия за по-добра четимост
ScriptManager::ScriptManager()
{                                                                                                   // Конструктор на класа ScriptManager
    m_tempDir = wxFileName::GetTempDir() + wxFileName::GetPathSeparator() + "iso_analyzer_scripts"; // Инициализира пътя до временната директория: системна временна директория + разделител + име на поддиректория
    InitializeScripts();                                                                            // Извиква метод за инициализация на съдържанието на скриптовете
} // Край на конструктора
  // Празна линия за по-добра четимост
void ScriptManager::InitializeScripts()
{ // Дефиниция на метод за инициализация на картата със съдържанието на скриптовете
    m_scriptContents = {
        // Инициализира член-променливата m_scriptContents (карта от име на скрипт към съдържание)
        {"setup_chroot.sh", SETUP_CHROOT_SCRIPT}, // Добавя скрипта setup_chroot.sh и неговото съдържание (от SETUP_CHROOT_SCRIPT)
        {"create_iso.sh", CREATE_ISO_SCRIPT},     // Добавя скрипта create_iso.sh и неговото съдържание (от CREATE_ISO_SCRIPT)
        {"setup_output.sh", SETUP_OUTPUT_SCRIPT}  // Добавя скрипта setup_output.sh и неговото съдържание (от SETUP_OUTPUT_SCRIPT)
    }; // Край на инициализацията на картата m_scriptContents
} // Край на метод InitializeScripts
  // Празна линия за по-добра четимост
bool ScriptManager::EnsureTempDirectory()
{ // Дефиниция на метод, който проверява и при нужда създава временната директория
    if (!wxDirExists(m_tempDir))
    { // Проверява дали временната директория (m_tempDir) не съществува
        if (!wxMkdir(m_tempDir))
        {                                                                      // Ако не съществува, опитва да я създаде
            wxLogError("Failed to create temporary directory: %s", m_tempDir); // Ако създаването е неуспешно, записва грешка в лога с пътя на директорията
            return false;                                                      // Връща false, указвайки неуспех при създаването на директорията
        } // Край на вътрешния if блок (проверка за успешно създаване)
    } // Край на външния if блок (проверка за съществуване на директорията)
    return true; // Връща true, указвайки, че директорията съществува или е успешно създадена
} // Край на метод EnsureTempDirectory
  // Празна линия за по-добра четимост
bool ScriptManager::WriteScriptToFile(const wxString &path, const wxString &content)
{                // Дефиниция на метод за записване на съдържание на скрипт във файл
    wxFile file; // Декларира обект от тип wxFile за файлови операции
    if (!file.Create(path, true) || !file.Write(content))
    {                                                      // Опитва да създаде файла (с опция за презаписване, ако съществува) ИЛИ да запише съдържанието в него; ако някоя операция е неуспешна
        wxLogError("Failed to write script to: %s", path); // Записва грешка в лога, указваща неуспешен запис в посочения път
        return false;                                      // Връща false, указвайки неуспех при записването на файла
    } // Край на if блок (проверка за неуспешно създаване/запис)
      // Празна линия за по-добра четимост (преди условната компилация)
#ifndef __WINDOWS__            // Начало на блок за условна компилация: изпълнява се, ако __WINDOWS__ не е дефиниран (т.е. не е Windows система)
    chmod(path.c_str(), 0755); // Make executable on Unix-like systems // Променя правата на файла на 0755 (rwxr-xr-x), за да го направи изпълним в Unix-подобни системи; оригиналният английски коментар е запазен
#endif                         // Край на блок за условна компилация
                               // Празна линия за по-добра четимост (след условната компилация)
    return true;               // Връща true, указвайки, че скриптът е успешно записан във файла (и евентуално направен изпълним)
} // Край на метод WriteScriptToFile
  // Празна линия за по-добра четимост
bool ScriptManager::ExtractScriptToTemp(const wxString &scriptName)
{ // Дефиниция на метод за извличане (запис) на скрипт във временната директория
    if (!EnsureTempDirectory())
        return false;                            // Първо осигурява съществуването на временната директория; ако е неуспешно, връща false
                                                 // Празна линия за по-добра четимост
    auto it = m_scriptContents.find(scriptName); // Търси скрипта по име (scriptName) в картата m_scriptContents; 'auto' извежда типа на итератора
    if (it == m_scriptContents.end())
    {                                                 // Проверява дали скриптът е намерен (ако итераторът сочи към края на картата, значи не е намерен)
        wxLogError("Unknown script: %s", scriptName); // Записва грешка в лога за непознато име на скрипт
        return false;                                 // Връща false, указвайки, че скриптът не е намерен
    } // Край на if блок (проверка за намерен скрипт)
      // Празна линия за по-добра четимост
    wxString path = GetTempScriptPath(scriptName); // Получава пълния път до файла на скрипта във временната директория
    return WriteScriptToFile(path, it->second);    // Записва съдържанието на скрипта (it->second) във файла на посочения път (path) и връща резултата от операцията
} // Край на метод ExtractScriptToTemp
  // Празна линия за по-добра четимост
wxString ScriptManager::GetTempScriptPath(const wxString &scriptName) const
{                                                                   // Дефиниция на const метод за получаване на пълния път до временен скрипт
    return m_tempDir + wxFileName::GetPathSeparator() + scriptName; // Връща конструирания път: временна директория + разделител + име на скрипта
} // Край на метод GetTempScriptPath