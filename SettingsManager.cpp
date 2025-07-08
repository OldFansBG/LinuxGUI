#include "SettingsManager.h" // Включва заглавния файл SettingsManager.h
#include <wx/file.h>         // Включва библиотека на wxWidgets за работа с файлове
#include <wx/log.h>          // Включва библиотека на wxWidgets за логиране на съобщения
#include <wx/filename.h>     // Needed for wxFileName // Включва библиотека на wxWidgets за работа с имена на файлове (необходимо за wxFileName)

// In SettingsManager.cpp // В SettingsManager.cpp (коментар, указващ името на файла)

bool SettingsManager::SaveSettings(const AppSettings &settings, const wxString &configPath)
{                                                       // Дефиниция на метода SaveSettings на класа SettingsManager
    rapidjson::Document doc = SettingsToJson(settings); // Конвертира настройките (обект settings) в JSON документ

    rapidjson::StringBuffer buffer;                                  // Създава буфер за съхранение на JSON низ
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer); // Now valid // Създава обект PrettyWriter за форматирано записване на JSON в буфера (сега е валидно)
    writer.SetIndent(' ', 4);                                        // Задава отстъп от 4 интервала за форматирания JSON
    doc.Accept(writer);                                              // Записва JSON документа във форматиран вид в буфера чрез writer

    wxFile file(configPath, wxFile::write); // Отваря файл на указания път (configPath) в режим за запис
    if (!file.IsOpened())
        return false; // Ако файлът не е отворен успешно, връща false (неуспех)

    return file.Write(buffer.GetString(), buffer.GetSize()); // Записва съдържанието на буфера (JSON низ) във файла и връща резултата от операцията (true при успех)
}

// Add to SettingsManager.cpp // Добавяне към SettingsManager.cpp (коментар за местоположението на кода)
SettingsManager::SettingsManager()
{ // Дефиниция на конструктора на класа SettingsManager
    // Constructor can be empty if no initialization is needed // Конструкторът може да е празен, ако не е необходима инициализация
    // Or add initialization logic here if required // Или добавете логика за инициализация тук, ако е необходимо
}

bool SettingsManager::LoadSettings(AppSettings &settings, const wxString &configPath)
{ // Дефиниция на метода LoadSettings на класа SettingsManager
    if (!wxFileExists(configPath))
    {                                                              // Проверява дали файлът на указания път (configPath) съществува
        wxLogMessage("No settings file found at: %s", configPath); // Логира съобщение, че файлът с настройки не е намерен
        return false;                                              // Връща false (неуспех), тъй като файлът не съществува
    }

    wxFile file(configPath); // Отваря файл на указания път (configPath) в режим за четене (по подразбиране)
    if (!file.IsOpened())
        return false; // Ако файлът не е отворен успешно, връща false (неуспех)

    wxString jsonStr;       // Декларира низ за съхранение на прочетеното JSON съдържание
    file.ReadAll(&jsonStr); // Чете цялото съдържание на файла в низа jsonStr

    rapidjson::Document doc;            // Създава JSON документ (обект на rapidjson)
    doc.Parse(jsonStr.ToUTF8().data()); // Парсва JSON низа (конвертиран към UTF-8) и зарежда данните в документа
    if (doc.HasParseError())
        return false; // Ако има грешка при парсването на JSON, връща false (неуспех)

    settings = JsonToSettings(doc); // Конвертира JSON документа в обект AppSettings и го присвоява на референцията settings
    return true;                    // Връща true (успех), тъй като настройките са заредени успешно
}

rapidjson::Document SettingsManager::SettingsToJson(const AppSettings &settings) const
{                                         // Дефиниция на метода SettingsToJson, който конвертира AppSettings в JSON
    rapidjson::Document doc;              // Създава празен JSON документ
    doc.SetObject();                      // Задава типа на документа да бъде JSON обект
    auto &allocator = doc.GetAllocator(); // Взема алокатор на памет от документа, нужен за създаване на JSON елементи

    // Save ISO path // Запазване на пътя до ISO файла
    doc.AddMember("iso_path",                                                    // Добавя член "iso_path" към JSON обекта
                  rapidjson::Value(settings.isoPath.ToUTF8().data(), allocator), // Стойността е пътят до ISO файла, конвертиран към UTF-8
                  allocator);                                                    // Алокаторът се използва за създаване на стойността

    // Save working directory (last folder name only) // Запазване на работната директория (само името на последната папка)
    wxString workDir = settings.workDir; // Взема пълния път на работната директория от настройките
    wxString workDirName;                // Низ за съхранение само на името на последната папка

    if (!workDir.IsEmpty())
    { // Ако пътят на работната директория не е празен
        // Ensure the path ends with a separator to force directory parsing // Уверява се, че пътят завършва с разделител, за да принуди парсването като директория
        if (!workDir.EndsWith(wxFileName::GetPathSeparator()))
        {                                              // Ако пътят не завършва с разделител за пътища
            workDir += wxFileName::GetPathSeparator(); // Добавя разделител за пътища в края
        }

        wxFileName fn(workDir);                            // Създава обект wxFileName от пълния път
        fn.Normalize(wxPATH_NORM_LONG | wxPATH_NORM_DOTS); // Normalize the path // Нормализира пътя (разрешава дълги пътища и ".."/"." компоненти)

        const wxArrayString &dirs = fn.GetDirs(); // Взема списък с имената на директориите в пътя
        if (!dirs.IsEmpty())
        {                              // Ако списъкът с директории не е празен
            workDirName = dirs.Last(); // Get the last folder (e.g., "MyProject") // Взема името на последната директория (напр. "MyProject")
        }
        else
        { // Ако списъкът с директории е празен (напр. за коренни пътища)
            // Handle root paths like "C:\" or "/" // Обработка на коренни пътища като "C:\" или "/"
            workDirName = fn.GetFullName(); // Returns "C:" for "C:\" // Връща пълното име (напр. "C:" за "C:\")
        }
    }

    doc.AddMember("work_dir",                                               // Добавя член "work_dir" към JSON обекта
                  rapidjson::Value(workDirName.ToUTF8().data(), allocator), // Стойността е само името на последната папка, конвертирано към UTF-8
                  allocator);                                               // Алокатор

    // Save other fields // Запазване на други полета
    doc.AddMember("project_name",                                                       // Добавя член "project_name"
                  rapidjson::Value(settings.projectName.ToUTF8().data(), allocator),    // Стойността е името на проекта
                  allocator);                                                           // Алокатор
    doc.AddMember("version",                                                            // Добавя член "version"
                  rapidjson::Value(settings.version.ToUTF8().data(), allocator),        // Стойността е версията
                  allocator);                                                           // Алокатор
    doc.AddMember("detected_distro",                                                    // Добавя член "detected_distro"
                  rapidjson::Value(settings.detectedDistro.ToUTF8().data(), allocator), // Стойността е засечената дистрибуция
                  allocator);                                                           // Алокатор

    return doc; // Връща конструирания JSON документ
}

AppSettings SettingsManager::JsonToSettings(const rapidjson::Document &doc) const
{                         // Дефиниция на метода JsonToSettings, който конвертира JSON в AppSettings
    AppSettings settings; // Създава празен обект AppSettings

    if (doc.HasMember("iso_path"))                                                        // Ако JSON документът има член "iso_path"
        settings.isoPath = wxString::FromUTF8(doc["iso_path"].GetString());               // Задава стойността на isoPath в настройките от JSON (конвертирайки от UTF-8)
    if (doc.HasMember("work_dir"))                                                        // Ако JSON документът има член "work_dir"
        settings.workDir = wxString::FromUTF8(doc["work_dir"].GetString());               // Задава стойността на workDir
    if (doc.HasMember("project_name"))                                                    // Ако JSON документът има член "project_name"
        settings.projectName = wxString::FromUTF8(doc["project_name"].GetString());       // Задава стойността на projectName
    if (doc.HasMember("version"))                                                         // Ако JSON документът има член "version"
        settings.version = wxString::FromUTF8(doc["version"].GetString());                // Задава стойността на version
    if (doc.HasMember("detected_distro"))                                                 // Ако JSON документът има член "detected_distro"
        settings.detectedDistro = wxString::FromUTF8(doc["detected_distro"].GetString()); // Задава стойността на detectedDistro

    return settings; // Връща попълнения обект AppSettings
}