#ifndef SYSTEMTHEME_H // Ако SYSTEMTHEME_H не е дефиниран (предпазва от многократно включване)
#define SYSTEMTHEME_H // Дефинира SYSTEMTHEME_H, за да се избегне повторно включване на този хедър файл

#include <wx/wx.h>       // Включва основния заглавен файл на wxWidgets
#include "ThemeConfig.h" // Включва заглавния файл за ThemeConfig, за да може да се извика UpdateSystemTheme

#ifdef __WXMSW__     // Ако се компилира за Windows (MSW - Microsoft Windows)
#include <windows.h> // Включва заглавния файл на Windows API (необходим за достъп до регистъра)
#endif               // Край на условната компилация за Windows

class SystemTheme
{       // Декларация на клас SystemTheme, който предоставя функционалност, свързана със системната тема
public: // Секция с публични членове на класа
    static bool IsDarkMode()
    {            // Статичен публичен метод, който проверява дали системната тема е тъмна
#ifdef __WXMSW__ // Ако се компилира за Windows
        // Windows dark mode detection // Откриване на тъмен режим в Windows
        HKEY hKey;                                                                                    // Променлива за съхранение на манипулатор към отворен ключ в регистъра
        const wchar_t *subKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"; // Път до ключа в регистъра, съдържащ настройките за персонализация
        const wchar_t *valueName = L"AppsUseLightTheme";                                              // Име на стойността в регистъра, която указва дали приложенията използват светла тема
        DWORD value = 0;                                                                              // Променлива за съхранение на прочетената стойност от регистъра (0 за тъмна, 1 за светла)
        DWORD size = sizeof(value);                                                                   // Размер на променливата 'value' в байтове

        if (RegOpenKeyExW(HKEY_CURRENT_USER, subKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {                                                     // Опитва се да отвори указания ключ в регистъра за четене
            if (RegQueryValueExW(hKey, valueName, NULL, NULL, // Опитва се да прочете стойността 'AppsUseLightTheme'
                                 reinterpret_cast<LPBYTE>(&value), &size) == ERROR_SUCCESS)
            {                      // Ако четенето е успешно
                RegCloseKey(hKey); // Затваря ключа в регистъра
                return value == 0; // Връща true, ако стойността е 0 (тъмна тема), в противен случай false
            } // Край на проверката за успешно четене на стойност
            RegCloseKey(hKey); // Затваря ключа в регистъра, дори ако четенето на стойността е неуспешно
        } // Край на проверката за успешно отваряне на ключ
#elif defined(__WXGTK__) // Ако се компилира за Linux с GTK
        // Linux dark mode detection // Откриване на тъмен режим в Linux
        wxString cmd = "gsettings get org.gnome.desktop.interface color-scheme"; // Команда за получаване на текущата цветова схема от настройките на GNOME
        wxArrayString output;                                                    // Масив от низове за съхранение на изхода от командата
        wxExecute(cmd, output, wxEXEC_SYNC);                                     // Изпълнява командата синхронно и съхранява изхода в 'output'

        if (!output.IsEmpty())
        {                                        // Ако има изход от командата
            wxString result = output[0].Lower(); // Взема първия ред от изхода и го преобразува в малки букви
            return result.Contains("dark");      // Връща true, ако изходът съдържа "dark", в противен случай false
        } // Край на проверката за непразен изход
#endif                   // Край на условната компилация за платформа
        return false;    // По подразбиране връща false (светла тема), ако откриването е неуспешно или не е имплементирано за текущата платформа
    } // Край на метода IsDarkMode

    static void RegisterForThemeChanges(wxWindow *window)
    {                                                                               // Статичен публичен метод за регистриране на прозорец за уведомления при промяна на системната тема
#ifdef __WXMSW__                                                                    // Ако се компилира за Windows
        window->Bind(wxEVT_SYS_COLOUR_CHANGED, &SystemTheme::OnSystemThemeChanged); // Свързва събитието за промяна на системните цветове (wxEVT_SYS_COLOUR_CHANGED) с метода OnSystemThemeChanged
#endif                                                                              // Край на условната компилация за Windows
    } // Край на метода RegisterForThemeChanges

private:         // Секция с частни членове на класа
#ifdef __WXMSW__ // Ако се компилира за Windows
    static void OnSystemThemeChanged(wxSysColourChangedEvent &event)
    {                                           // Статичен частен метод, който се извиква при промяна на системните цветове (само за Windows)
        ThemeConfig::Get().UpdateSystemTheme(); // Извиква метода UpdateSystemTheme на сингълтъна ThemeConfig, за да актуализира темата на приложението
        event.Skip();                           // Позволява на други обработчици на събития да обработят това събитие
    } // Край на метода OnSystemThemeChanged
#endif // Край на условната компилация за Windows
}; // Край на декларацията на клас SystemTheme

#endif // SYSTEMTHEME_H // Край на условната компилация, започната с #ifndef