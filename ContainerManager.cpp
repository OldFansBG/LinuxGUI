#include "ContainerManager.h" // Включва заглавния файл за класа ContainerManager
#include <wx/filename.h>      // Включва wxFileName за работа с имена на файлове и пътища
#include <wx/stdpaths.h>      // Включва wxStandardPaths за получаване на стандартни пътища на системата
#include <fstream>            // Включва стандартната C++ библиотека за работа с файлови потоци

ContainerManager &ContainerManager::Get()
{                                     // Дефиниция на статичния метод Get, който връща референция към единствения екземпляр на ContainerManager
    static ContainerManager instance; // Създава статичен локален екземпляр на ContainerManager (Singleton модел)
    return instance;                  // Връща референция към екземпляра
}

bool ContainerManager::SaveContainerId(const wxString &containerId, const wxString &isoPath)
{                                              // Дефиниция на метода SaveContainerId, който запазва ID на контейнер и път до ISO
    std::lock_guard<std::mutex> lock(m_mutex); // Създава lock_guard, който автоматично заключва и отключва мютекса m_mutex за осигуряване на нишкова безопасност

    wxString appDataPath = wxStandardPaths::Get().GetUserDataDir(); // Взема пътя до директорията за потребителски данни на приложението
    if (!wxDirExists(appDataPath))
    {                         // Проверява дали директорията съществува
        wxMkdir(appDataPath); // Ако не съществува, създава я
    }

    wxString containerFile = appDataPath + wxFileName::GetPathSeparator() + "containers.dat"; // Формира пълния път до файла "containers.dat" в директорията за потребителски данни
    std::ofstream file(containerFile.ToStdString(), std::ios::app);                           // Отваря файла "containers.dat" в режим на добавяне (append)
    if (!file)
        return false; // Ако отварянето на файла е неуспешно, връща false

    wxString timestamp = wxDateTime::Now().Format("%Y-%m-%d %H:%M:%S"); // Взема текущата дата и час и ги форматира като низ
    file << containerId.ToStdString() << "|"                            // Записва ID-то на контейнера във файла, последвано от разделител "|"
         << isoPath.ToStdString() << "|"                                // Записва пътя до ISO файла, последван от разделител "|"
         << timestamp.ToStdString() << "\n";                            // Записва времевия печат, последван от нов ред

    wxString tempPath = wxFileName::GetTempDir() + wxFileName::GetPathSeparator() + "container_id.txt"; // Формира пълния път до временния файл "container_id.txt"
    std::ofstream tempFile(tempPath.ToStdString());                                                     // Отваря (или създава, ако не съществува) временния файл за запис (презаписва съдържанието)
    if (tempFile)
    {                            // Проверява дали отварянето на временния файл е успешно
        tempFile << containerId; // Записва ID-то на контейнера във временния файл
    }

    m_currentContainerId = containerId; // Задава текущото ID на контейнера в член-променливата m_currentContainerId
    return true;                        // Връща true, за да покаже, че операцията е успешна
}

wxString ContainerManager::GetCurrentContainerId() const
{                                // Дефиниция на метода GetCurrentContainerId, който връща текущото ID на контейнера
    return m_currentContainerId; // Връща стойността на член-променливата m_currentContainerId
}

bool ContainerManager::CleanupContainer(const wxString &containerId)
{ // Дефиниция на метода CleanupContainer, който почиства (премахва) Docker контейнер
    if (containerId.IsEmpty())
        return false; // Ако ID-то на контейнера е празно, връща false

    wxString cmd = "docker rm -f " + containerId; // Формира командата за премахване на Docker контейнер (с опция -f за принудително премахване)
    wxExecute(cmd, wxEXEC_SYNC);                  // Изпълнява командата синхронно (изчаква завършването й)

    wxString tempPath = wxFileName::GetTempDir() + wxFileName::GetPathSeparator() + "container_id.txt"; // Формира пътя до временния файл "container_id.txt"
    if (wxFileExists(tempPath))
    {                           // Проверява дали временният файл съществува
        wxRemoveFile(tempPath); // Ако съществува, изтрива го
    }

    return true; // Връща true (дори ако командата за премахване на контейнера е неуспешна, методът продължава и връща true; може да се наложи проверка на резултата от wxExecute)
}

bool ContainerManager::EnsureOutputDirectory(const wxString &containerId)
{                                                                                    // Дефиниция на метода EnsureOutputDirectory, който гарантира съществуването на /output директория в контейнера
    std::lock_guard<std::mutex> lock(m_mutex);                                       // Осигурява нишкова безопасност при достъп до споделени ресурси (ако има такива, които се променят тук)
    wxLogMessage("Ensuring output directory exists for container: %s", containerId); // Записва информационно съобщение в лога

    wxString mkdirCmd = wxString::Format(                // Формира команда за създаване на директория в Docker контейнера
        "docker exec %s mkdir -p /output", containerId); // Командата използва "mkdir -p", за да създаде директорията, ако не съществува, без грешка, ако вече съществува

    wxArrayString output, errors; // Декларира масиви от низове за съхранение на стандартния изход и грешките от командата
    if (wxExecute(mkdirCmd, output, errors, wxEXEC_SYNC) != 0)
    {                                                    // Изпълнява командата и проверява изходния код (0 означава успех)
        wxLogError("Failed to create output directory"); // Ако изходният код не е 0, записва грешка в лога
        return false;                                    // Връща false, за да покаже неуспех
    }

    wxString chmodCmd = wxString::Format(                 // Формира команда за промяна на правата на достъп на директорията в Docker контейнера
        "docker exec %s chmod 777 /output", containerId); // Командата задава права 777 (четене, запис, изпълнение за всички) на директорията /output

    if (wxExecute(chmodCmd, output, errors, wxEXEC_SYNC) != 0)
    {                                                      // Изпълнява командата и проверява изходния код
        wxLogError("Failed to set directory permissions"); // Ако изходният код не е 0, записва грешка в лога
        return false;                                      // Връща false, за да покаже неуспех
    }

    return true; // Връща true, ако и двете команди са изпълнени успешно
}

void ContainerManager::CleanupAllContainers()
{                                                                                             // Дефиниция на метода CleanupAllContainers, който почиства всички контейнери, записани във файла containers.dat
    wxString appDataPath = wxStandardPaths::Get().GetUserDataDir();                           // Взема пътя до директорията за потребителски данни
    wxString containerFile = appDataPath + wxFileName::GetPathSeparator() + "containers.dat"; // Формира пътя до файла containers.dat

    if (!wxFileExists(containerFile))
        return; // Ако файлът не съществува, прекратява изпълнението на метода

    std::ifstream file(containerFile.ToStdString()); // Отваря файла containers.dat за четене
    std::string line;                                // Декларира низ за съхранение на всеки прочетен ред от файла
    while (std::getline(file, line))
    {                                // Чете файла ред по ред
        size_t pos = line.find('|'); // Намира позицията на първия разделител "|" в реда
        if (pos != std::string::npos)
        {                                                                   // Проверява дали разделителят е намерен (std::string::npos означава, че не е намерен)
            wxString containerId = wxString::FromUTF8(line.substr(0, pos)); // Извлича подниза от началото на реда до позицията на разделителя (това е ID-то на контейнера) и го конвертира към wxString
            CleanupContainer(containerId);                                  // Извиква метода CleanupContainer за извлеченото ID на контейнер
        }
    }

    wxRemoveFile(containerFile); // След обработка на всички контейнери, изтрива файла containers.dat
}