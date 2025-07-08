#ifndef APPSETTINGS_H // Предпазител от многократно включване на хедър файла
#define APPSETTINGS_H // Дефиниране на предпазителя за хедър файла

#include <wx/string.h> // Включване на wxString клас от wxWidgets библиотеката

// Структура, съдържаща настройките на приложението
struct AppSettings
{                            // Дефиниция на структурата за настройки на приложението
    wxString isoPath;        // Път до ISO файла
    wxString workDir;        // Работна директория
    wxString projectName;    // Име на проекта
    wxString version;        // Версия на проекта
    wxString detectedDistro; // Открита Linux дистрибуция
};

#endif // APPSETTINGS_H // Край на предпазителя за хедър файла