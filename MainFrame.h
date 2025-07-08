#ifndef MAINFRAME_H // Проверява дали MAINFRAME_H вече не е дефиниран, за да предотврати многократно включване на един и същ заглавен файл.
#define MAINFRAME_H // Дефинира MAINFRAME_H.

#include <wx/wx.h>           // Включва основната библиотека wxWidgets, която съдържа основни класове и функции.
#include <wx/statline.h>     // Включва wxStaticLine, който предоставя хоризонтална или вертикална линия.
#include <wx/bmpbuttn.h>     // Включва wxBitmapButton, бутон с изображение.
#include <yaml-cpp/yaml.h>   // Включва YAML-CPP библиотеката за работа с YAML файлове.
#include <vector>            // Включва std::vector за динамични масиви.
#include <string>            // Включва std::string за работа с низове.
#include "CustomTitleBar.h"  // Включва дефиницията на CustomTitleBar и MyButton.
#include "CustomStatusBar.h" // Включва дефиницията на CustomStatusBar.
#include "SecondWindow.h"    // Включва дефиницията на SecondWindow.
#include "WindowIDs.h"       // Включва дефиниции на ID на прозорци.
#include "SettingsManager.h" // Включва дефиницията на SettingsManager.
#include "DesktopTab.h"      // Включва дефиницията на DesktopTab.

#ifdef __WXMSW__                     // Проверява дали компилацията е за Microsoft Windows.
#include <dwmapi.h>                  // Включва DWM API за работа с Desktop Window Manager (напр. Mica ефект).
#include <windows.h>                 // Включва основните Windows API функции.
#pragma comment(lib, "dwmapi.lib")   // Линква dwmapi.lib.
#pragma comment(lib, "advapi32.lib") // Линква advapi32.lib.

#ifndef DWMWA_MICA_EFFECT      // Проверява дали DWMWA_MICA_EFFECT вече не е дефиниран.
#define DWMWA_MICA_EFFECT 1029 // Дефинира константа за Mica ефект (за Windows 11).
#endif
#endif

// Дефиниция на класа MainFrame, който наследява wxFrame.
class MainFrame : public wxFrame
{
public:
   MainFrame(const wxString &title); // Конструктор на класа, приема заглавие на прозореца.
   ~MainFrame();                     // Деструктор на класа.

   void SetStatusText(const wxString &text); // Публична функция за задаване на текст в статус бара.

private:
   // Функции за търсене и откриване
   bool SearchSquashFS(const wxString &isoPath, wxArrayString &foundFiles); // Търси SquashFS файлове в ISO изображение.
   void CreateFrameControls();                                              // Създава контролите (UI елементи) на главния прозорец.
   void OpenSecondWindow();                                                 // Отваря втори прозорец.
   wxString m_currentISOPath;                                               // Член-променлива за текущия път до ISO файла.

   // Обработчици на събития
   void OnBrowseISO(wxCommandEvent &event);     // Обработчик за събитие при кликване на бутон за избор на ISO.
   void OnBrowseWorkDir(wxCommandEvent &event); // Обработчик за събитие при кликване на бутон за избор на работна директория.
   void OnDetect(wxCommandEvent &event);        // Обработчик за събитие при кликване на бутон "Detect".
   void OnNextButton(wxCommandEvent &event);    // Обработчик за събитие при кликване на бутон "Next".
   void OnCancel(wxCommandEvent &event);        // Обработчик за събитие при кликване на бутон "Cancel".
   void OnSettings(wxCommandEvent &event);      // Обработчик за събитие при кликване на бутон "Settings".

   // Функции за откриване на дистрибуция
   bool SearchReleaseFile(const wxString &isoPath, wxString &releaseContent); // Търси 'release' файл в ISO и чете съдържанието му.
   bool SearchGrubEnv(const wxString &isoPath, wxString &distributionName);   // Търси 'grubenv' файл в ISO и извлича името на дистрибуцията.
   wxString ExtractNameFromGrubEnv(const wxString &content);                  // Извлича името на дистрибуцията от съдържанието на 'grubenv'.
   wxString DetectDistribution(const wxString &releaseContent);               // Открива името на дистрибуцията от съдържанието на 'release' файла.

   // Функции за настройки и конфигурация
   bool LoadConfig();         // Зарежда конфигурационни настройки.
   void CreateSettingsMenu(); // Създава менюто за настройки.

   // Функции за създаване на панели
   wxPanel *CreateLogoPanel(wxWindow *parent);      // Създава панел за лого.
   wxPanel *CreateDetectionPanel(wxWindow *parent); // Създава панел за откриване на ISO.
   wxPanel *CreateProjectPanel(wxWindow *parent);   // Създава панел за проектни настройки.
   wxPanel *CreateProgressPanel(wxWindow *parent);  // Създава панел за показване на прогрес.

   // Член-променливи на контролите (UI елементи)
   wxTextCtrl *m_isoPathCtrl;     // Текстово поле за пътя до ISO файла.
   wxTextCtrl *m_distroCtrl;      // Текстово поле за името на дистрибуцията.
   wxTextCtrl *m_projectNameCtrl; // Текстово поле за името на проекта.
   wxTextCtrl *m_versionCtrl;     // Текстово поле за версията.
   wxTextCtrl *m_workDirCtrl;     // Текстово поле за работната директория.
   wxGauge *m_progressGauge;      // Индикатор за прогрес.
   wxStaticText *m_statusText;    // Статичен текст за показване на статус.
   MyButton *m_settingsButton;    // Бутон за настройки (дефиниран чрез CustomTitleBar.h).
   CustomTitleBar *m_titleBar;    // Персонализиран заглавен бар.
   CustomStatusBar *m_statusBar;  // Персонализиран статус бар.
   YAML::Node m_config;           // YAML възел за съхранение на конфигурация.

   // Мениджър на настройките
   SettingsManager m_settingsManager; // Обект на класа SettingsManager.

   // Член DesktopTab
   DesktopTab *m_desktopTab; // Обект на класа DesktopTab.

   void OnGUIDetected(wxCommandEvent &event); // Обработчик за събитие, когато GUI е открито (може би за дебъгване или синхронизация).

   // Декларация на таблицата със събития
   DECLARE_EVENT_TABLE() // Макрос за декларация на таблица със събития (необходим от wxWidgets).
};

#endif // MAINFRAME_H // Завършва проверката за многократно включване.