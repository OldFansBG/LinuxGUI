#ifndef SETTINGS_MANAGER_H // Ако SETTINGS_MANAGER_H не е дефиниран
#define SETTINGS_MANAGER_H // Дефинирай SETTINGS_MANAGER_H (предпазител срещу многократно включване)

#include <wx/string.h>              // Включва библиотека за работа с низове на wxWidgets
#include <rapidjson/document.h>     // Включва библиотека rapidjson за работа с JSON документи
#include <rapidjson/stringbuffer.h> // Включва библиотека rapidjson за буфериране на низове
#include <rapidjson/prettywriter.h> // Add this line // Добавяне на този ред (Включва rapidjson за форматирано записване на JSON)
#include "AppSettings.h"            // Включва заглавния файл AppSettings.h (вероятно дефинира структурата AppSettings)

class SettingsManager
{                      // Декларация на клас SettingsManager
public:                // Публична секция
    SettingsManager(); // Конструктор на класа SettingsManager

    bool LoadSettings(AppSettings &settings, const wxString &configPath);       // Метод за зареждане на настройките, връща bool (успех/неуспех)
    bool SaveSettings(const AppSettings &settings, const wxString &configPath); // Single declaration // Метод за запазване на настройките, връща bool (успех/неуспех) - единична декларация

private:                                                                   // Частна секция
    rapidjson::Document SettingsToJson(const AppSettings &settings) const; // Частен метод за конвертиране на AppSettings към JSON документ
    AppSettings JsonToSettings(const rapidjson::Document &doc) const;      // Частен метод за конвертиране на JSON документ към AppSettings
};

#endif // SETTINGS_MANAGER_H // Край на предпазителя срещу многократно включване