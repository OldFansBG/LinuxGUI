#ifndef SECONDWINDOW_H // Ако SECONDWINDOW_H не е дефиниран
#define SECONDWINDOW_H // Дефинирай SECONDWINDOW_H (предпазител срещу многократно включване)

#include <wx/wx.h>     // Включва основната библиотека на wxWidgets
#include <wx/thread.h> // Включва библиотека за работа с нишки на wxWidgets
// #include <wx/listctrl.h> // No longer needed directly here after separating MongoDBPanel // Вече не е необходим директно тук след отделянето на MongoDBPanel
#include "DesktopTab.h"          // Включва заглавния файл DesktopTab.h
#include "ConfigTab.h"           // Включва заглавния файл ConfigTab.h
#include "LinuxTerminalPanel.h"  // Включва заглавния файл LinuxTerminalPanel.h
#include "OSDetector.h"          // Включва заглавния файл OSDetector.h
#include "ContainerManager.h"    // Включва заглавния файл ContainerManager.h
#include "FlatpakStore.h"        // Включва заглавния файл FlatpakStore.h
#include "WindowIDs.h"           // <<< Make sure this is included for the IDs // <<< Уверете се, че това е включено за идентификаторите

#ifdef _WIN32                   // Ако се компилира за Windows (_WIN32 е дефиниран)
#include "WinTerminalManager.h" // Включва заглавния файл WinTerminalManager.h
#endif                          // Край на условната компилация за Windows

// Forward declarations // Предварителни декларации (за да се избегнат циклични зависимости)
class OverlayFrame; // Предварителна декларация на клас OverlayFrame
class MongoDBPanel; // <<< Forward Declaration // <<< Предварителна декларация на клас MongoDBPanel

// --- SecondWindow Definition --- // --- Дефиниция на SecondWindow ---
class SecondWindow : public wxFrame          // Декларация на клас SecondWindow, наследяващ wxFrame
{                                            // Начало на тялото на класа SecondWindow
public:                                      // Публична секция
    SecondWindow(wxWindow *parent,           // Конструктор на SecondWindow
                 const wxString &title,      // Параметър: заглавие на прозореца
                 const wxString &isoPath,    // Параметър: път до ISO файла
                 const wxString &projectDir, // Параметър: директория на проекта
                 DesktopTab *desktopTab);    // Параметър: указател към DesktopTab

    virtual ~SecondWindow(); // Виртуален деструктор

    void CloseOverlay();                                             // Метод за затваряне на OverlayFrame
    wxString GetProjectDir() const { return m_projectDir; }          // Метод за получаване на директорията на проекта
    void ExecuteDockerCommand(const wxString &containerId);          // Метод за изпълнение на Docker команда
    DesktopTab *GetDesktopTab() const { return m_desktopTab; }       // Метод за получаване на указател към DesktopTab
    FlatpakStore *GetFlatpakStore() const { return m_flatpakStore; } // Метод за получаване на указател към FlatpakStore
    SQLTab *GetSQLTab() const { return m_sqlTab; }                   // Метод за получаване на указател към SQLTab

private:                                 // Частна секция
    wxPanel *m_mainPanel;                // Указател към главния панел
    wxPanel *m_terminalTab;              // Указател към панела на терминалния таб
    SQLTab *m_sqlTab;                    // Указател към SQLTab
    LinuxTerminalPanel *m_terminalPanel; // Указател към панела с Linux терминал
    FlatpakStore *m_flatpakStore;        // Указател към FlatpakStore
    MongoDBPanel *m_mongoPanel;          // <<< Pointer to the panel // <<< Указател към панела MongoDBPanel

    wxString m_isoPath;     // Път до ISO файла
    wxString m_projectDir;  // Директория на проекта
    wxString m_containerId; // ID на контейнера
    bool m_threadRunning;   // Флаг, показващ дали нишката работи

    OverlayFrame *m_overlay; // Указател към OverlayFrame
    OSDetector m_osDetector; // Обект от клас OSDetector

    DesktopTab *m_desktopTab; // Указател към DesktopTab

    void CleanupContainerAsync(); // Метод за асинхронно изтриване на контейнера (docker rm -f)

    bool m_isClosing; // Флаг, показващ дали прозорецът се затваря

    wxWindow *m_lastTab; // Указател към последния активен таб

#ifdef _WIN32                                 // Ако се компилира за Windows
    WinTerminalManager *m_winTerminalManager; // Указател към WinTerminalManager
#endif                                        // Край на условната компилация

    void CreateControls();        // Метод за създаване на контролите
    void StartPythonExecutable(); // Метод за стартиране на Python изпълним файл
    void CleanupThread();         // Метод за почистване на нишката

    // Event Handlers for SecondWindow // Обработчици на събития за SecondWindow
    void OnClose(wxCloseEvent &event);               // Обработчик на събитието за затваряне на прозореца
    void OnNext(wxCommandEvent &event);              // Обработчик на събитието "Next"
    void OnTabChanged(wxCommandEvent &event);        // Обработчик на събитието за смяна на таб
    void OnMongoDBButton(wxCommandEvent &event);     // Обработчик на събитието от бутона за MongoDB
    void OnMongoDBPanelClose(wxCommandEvent &event); // Handles close request // Обработчик на заявка за затваряне на MongoDB панела
    void DoDestroy();                                // Метод за извършване на унищожаването на прозореца

    wxDECLARE_CLASS(SecondWindow); // RTTI информация, за да работи wxDynamicCast(..., SecondWindow) коректно
    wxDECLARE_EVENT_TABLE();       // Макрос за деклариране на таблицата на събитията
}; // Край на тялото на класа SecondWindow

#endif // SECONDWINDOW_H // Край на предпазителя срещу многократно включване