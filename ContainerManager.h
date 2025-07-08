#ifndef CONTAINER_MANAGER_H // Предпроцесорна директива, проверява дали CONTAINER_MANAGER_H не е дефиниран
#define CONTAINER_MANAGER_H // Дефинира CONTAINER_MANAGER_H, за да се избегне многократно включване на хедър файла

#include <wx/string.h> // Включва хедър файла за wxString на wxWidgets
#include <wx/utils.h>  // За wxExecute // Включва хедър файла за помощни функции на wxWidgets, включително wxExecute
#include <wx/file.h>   // За файлови операции // Включва хедър файла за файлови операции на wxWidgets
#include <wx/log.h>    // За wxLogError // Включва хедър файла за системата за логиране на wxWidgets, включително wxLogError
#include <mutex>       // Включва хедър файла за std::mutex от стандартната библиотека за осигуряване на нишкова безопасност
#include <string>      // Включва хедър файла за std::string от стандартната библиотека

class ContainerManager
{                                   // Декларация на клас ContainerManager
public:                             // Начало на публичната секция на класа
    static ContainerManager &Get(); // Декларация на статичен метод Get, който връща референция към единствения екземпляр на класа (Singleton)

    bool SaveContainerId(const wxString &containerId, const wxString &isoPath); // Декларация на метод за запазване на ID на контейнер и път до ISO файл
    wxString GetCurrentContainerId() const;                                     // Декларация на метод за получаване на текущото ID на контейнера (константен метод)
    bool CleanupContainer(const wxString &containerId);                         // Декларация на метод за почистване (премахване) на Docker контейнер по неговото ID
    void CleanupAllContainers();                                                // Декларация на метод за почистване на всички управлявани Docker контейнери
    bool EnsureOutputDirectory(const wxString &containerId);                    // Декларация на метод за гарантиране на съществуването на изходна директория в контейнер

private:                                                            // Начало на частната секция на класа
    ContainerManager() = default;                                   // Декларира конструктора по подразбиране (празен конструктор) като генериран от компилатора. Това го прави частен, ограничавайки създаването на екземпляри извън класа (ключово за Singleton).
    ContainerManager(const ContainerManager &) = delete;            // Забранява конструктора за копиране, за да се предотврати копиране на екземпляра (Singleton)
    ContainerManager &operator=(const ContainerManager &) = delete; // Забранява оператора за присвояване при копиране, за да се предотврати присвояване на екземпляра (Singleton)

    wxString m_currentContainerId; // Член-променлива за съхранение на ID-то на текущо активния контейнер
    std::mutex m_mutex;            // Член-променлива от тип std::mutex за синхронизация на достъпа до споделени ресурси в многонишкови среди
}; // Край на дефиницията на класа ContainerManager

#endif // CONTAINER_MANAGER_H // Край на условната директива #ifndef