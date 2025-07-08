#include "MainFrame.h" // Включва заглавния файл за MainFrame (основния прозорец на приложението)
#include <wx/wx.h>     // Включва основния заглавен файл на wxWidgets
#include <windows.h>   // Включва основния заглавен файл на Windows API (за функции като RegOpenKeyExW, CreateFileW и др.)
#include <string>      // Включва стандартната C++ библиотека за работа с низове (въпреки че тук не се използва директно std::string, може да е полезно)

// Функция за проверка дали Docker Desktop е инсталиран
bool isDockerDesktopInstalled()
{                                                                                                        // Дефиниция на функция isDockerDesktopInstalled, която връща булева стойност
    HKEY hKey;                                                                                           // Декларира променлива от тип HKEY (handle към ключ в регистъра)
    const wchar_t *regPath = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Docker Desktop"; // Път в регистъра, където се намира информацията за инсталацията на Docker Desktop (широки символи)

    // Опит за отваряне на ключа в регистъра
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, regPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {                      // Опитва да отвори ключа в HKEY_LOCAL_MACHINE (за инсталации за всички потребители)
        RegCloseKey(hKey); // Затваря отворения ключ в регистъра
        return true;       // Връща true, ако ключът е намерен (Docker е инсталиран)
    } // Край на if блока

    // Проверка и за текущия потребител, в случай че е инсталиран само за него
    if (RegOpenKeyExW(HKEY_CURRENT_USER, regPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {                      // Опитва да отвори ключа в HKEY_CURRENT_USER (за инсталации само за текущия потребител)
        RegCloseKey(hKey); // Затваря отворения ключ в регистъра
        return true;       // Връща true, ако ключът е намерен
    } // Край на if блока

    return false; // Връща false, ако ключът не е намерен нито в HKEY_LOCAL_MACHINE, нито в HKEY_CURRENT_USER
} // Край на функция isDockerDesktopInstalled

// Функция за проверка дали Docker Engine работи
bool isDockerEngineRunning()
{                                                              // Дефиниция на функция isDockerEngineRunning, която връща булева стойност
    const wchar_t *dockerPipe = L"\\\\.\\pipe\\docker_engine"; // Име на именуваната тръба (named pipe), използвана от Docker Engine (широки символи)

    HANDLE hPipe = CreateFileW(       // Опитва да отвори файл (в случая, именуваната тръба)
        dockerPipe,                   // Named pipe path // Път до именуваната тръба
        GENERIC_READ | GENERIC_WRITE, // Access mode // Режим на достъп (четене и запис)
        0,                            // No sharing // Без споделяне
        nullptr,                      // Default security attributes // Атрибути за сигурност по подразбиране
        OPEN_EXISTING,                // Open existing pipe // Отваряне на съществуваща тръба
        0,                            // Default attributes // Атрибути по подразбиране
        nullptr                       // No template file // Без шаблон за файл
    );                                // Край на извикването на CreateFileW

    if (hPipe == INVALID_HANDLE_VALUE)
    {                                 // Ако връщането на CreateFileW е INVALID_HANDLE_VALUE (неуспешно отваряне)
        DWORD error = GetLastError(); // Взема кода на последната грешка
        if (error == ERROR_PIPE_BUSY)
        {                // Ако грешката е ERROR_PIPE_BUSY (тръбата е заета)
            return true; // Docker is running but busy // Връща true (Docker работи, но е зает)
        } // Край на if блока
        return false; // Docker is not running // Връща false (Docker не работи)
    } // Край на if блока

    CloseHandle(hPipe); // Затваря дръжката към именуваната тръба
    return true;        // Връща true (Docker работи, тъй като тръбата е била успешно отворена)
} // Край на функция isDockerEngineRunning

// Клас ISOAnalyzerApp, наследяващ wxApp
class ISOAnalyzerApp : public wxApp
{       // Дефиниция на клас ISOAnalyzerApp
public: // Публична секция на класа
    bool OnInit()
    {                             // Метод OnInit, който се извиква при инициализация на приложението
        wxInitAllImageHandlers(); // Инициализира всички налични обработчици на изображения (за формати като PNG, JPG и др.)

        // First check if Docker Desktop is installed // Коментар: Първо проверка дали Docker Desktop е инсталиран
        if (!isDockerDesktopInstalled())
        {                                                                                                             // Ако Docker Desktop не е инсталиран
            int answer = wxMessageBox(                                                                                // Показва диалогов прозорец със съобщение
                "Docker Desktop is not installed. This application requires Docker Desktop to function properly.\n\n" // Текст на съобщението
                "Would you like to abort the application?",                                                           // Въпрос към потребителя
                "Docker Not Found",                                                                                   // Заглавие на диалоговия прозорец
                wxYES_NO | wxICON_ERROR                                                                               // Бутони "Да"/"Не" и икона за грешка
            );                                                                                                        // Край на извикването на wxMessageBox

            if (answer == wxYES)
            {                 // Ако потребителят избере "Да"
                return false; // Exit application // Връща false (приложението се прекратява)
            } // Край на if блока
            // Ако потребителят избере "Не", продължава без Docker, но това не е отразено с return true тук, което е леко объркващо.
            // Ще се изпълни return true; по-долу.
        } // Край на if блока за проверка на инсталацията на Docker

        // Then check if Docker is running // Коментар: След това проверка дали Docker работи
        if (!isDockerEngineRunning())
        {                                                                                                  // Ако Docker Engine не работи
            int answer = wxMessageBox(                                                                     // Показва диалогов прозорец със съобщение
                "Docker Desktop is not running. The application requires Docker to function properly.\n\n" // Текст на съобщението
                "Please start Docker Desktop and try again.\n\n"                                           // Инструкции към потребителя
                "Would you like to abort the application?",                                                // Въпрос към потребителя
                "Docker Not Running",                                                                      // Заглавие на диалоговия прозорец
                wxYES_NO | wxICON_ERROR                                                                    // Бутони "Да"/"Не" и икона за грешка
            );                                                                                             // Край на извикването на wxMessageBox

            if (answer == wxYES)
            {                 // Ако потребителят избере "Да"
                return false; // Exit application // Връща false (приложението се прекратява)
            } // Край на if блока
        } // Край на if блока за проверка дали Docker работи

        MainFrame *frame = new MainFrame("LinuxISOPro"); // Създава нов обект от клас MainFrame (основния прозорец) със заглавие "LinuxISOPro"
        frame->Show(true);                               // Показва основния прозорец
        return true;                                     // Връща true (инициализацията на приложението е успешна)
    } // Край на метод OnInit
}; // Край на клас ISOAnalyzerApp

wxIMPLEMENT_APP(ISOAnalyzerApp); // Макрос, който дефинира входната точка на приложението (еквивалент на main функцията) и създава инстанция на ISOAnalyzerApp