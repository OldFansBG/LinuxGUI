#ifndef THEMECONFIG_H // Ако THEMECONFIG_H не е дефиниран (предпазва от многократно включване)
#define THEMECONFIG_H // Дефинира THEMECONFIG_H, за да се избегне повторно включване на този хедър файл

#include <wx/wx.h>         // Включва основния заглавен файл на wxWidgets
#include <yaml-cpp/yaml.h> // Включва заглавния файл за библиотеката yaml-cpp за работа с YAML файлове
#include <string>          // Включва заглавния файл за стандартния клас std::string
#include <map>             // Включва заглавния файл за стандартния контейнер std::map (асоциативен масив)
#include <vector>          // Включва заглавния файл за стандартния контейнер std::vector (динамичен масив)
#include "WindowIDs.h"     // Включва заглавния файл WindowIDs.h (Уверете се, че ID-тата са включени, ако са необходими)

// Forward declaration for MyButton if defined elsewhere // Предварителна декларация за MyButton, ако е дефиниран другаде
class MyButton; // Предварително декларира клас MyButton, за да може да се използва указател/референция към него преди пълната му дефиниция

struct ThemeColors           // Декларация на структура ThemeColors, която съдържа цветови дефиниции за темата
{                            // Начало на структурата ThemeColors
    struct ButtonColors      // Вложена структура за цветове на бутони
    {                        // Начало на структурата ButtonColors
        wxColour background; // Цвят на фона на бутона
        wxColour hover;      // Цвят на бутона при посочване с мишката (hover)
        wxColour pressed;    // Цвят на бутона при натискане
        wxColour text;       // Цвят на текста на бутона
    }; // Край на структурата ButtonColors

    struct InputColors        // Вложена структура за цветове на полета за въвеждане
    {                         // Начало на структурата InputColors
        wxColour background;  // Цвят на фона на полето за въвеждане
        wxColour border;      // Цвят на рамката на полето за въвеждане
        wxColour text;        // Цвят на текста в полето за въвеждане
        wxColour placeholder; // Цвят на текста-заместител (placeholder) в полето за въвеждане
    }; // Край на структурата InputColors

    struct GaugeColors       // Вложена структура за цветове на индикатори за прогрес (gauge)
    {                        // Начало на структурата GaugeColors
        wxColour background; // Цвят на фона на индикатора
        wxColour fill;       // Цвят на запълването (прогреса) на индикатора
    }; // Край на структурата GaugeColors

    struct PanelColors       // Вложена структура за цветове на панели
    {                        // Начало на структурата PanelColors
        wxColour background; // Цвят на фона на панела
        wxColour border;     // Цвят на рамката на панела
    }; // Край на структурата PanelColors

    wxColour titleBar;          // Цвят на заглавната лента
    wxColour statusBar;         // Цвят на статус лентата
    wxColour background;        // Основен цвят на фона
    wxColour text;              // Основен цвят на текста
    wxColour secondaryText;     // Вторичен цвят на текста
    wxColour border;            // Цвят на рамките
    ButtonColors button;        // Цветове за стандартни бутони
    ButtonColors extractButton; // Цветове за бутона 'Next'/'Extract' (Приема се, че това е за бутона 'Напред'/'Извлечи')
    InputColors input;          // Цветове за полета за въвеждане
    GaugeColors gauge;          // Цветове за индикатори
    PanelColors panel;          // Цветове за панели
}; // Край на структурата ThemeColors

class ThemeConfig              // Декларация на клас ThemeConfig за управление на теми
{                              // Начало на класа ThemeConfig
public:                        // Секция с публични членове на класа
    static ThemeConfig &Get(); // Статичен метод за достъп до единствения екземпляр на класа (Singleton)

    void LoadThemes(const wxString &filename = "themes.json");              // Метод за зареждане на теми от файл (по подразбиране "themes.json")
    void ApplyTheme(wxWindow *window, const wxString &themeName);           // Метод за прилагане на тема към даден прозорец
    void ApplyThemeToControl(wxWindow *control, const ThemeColors &colors); // Метод за прилагане на цветове от тема към специфичен контрол
    const ThemeColors &GetThemeColors(const wxString &themeName) const;     // Метод за получаване на цветовите дефиниции за дадена тема

    bool HasTheme(const wxString &themeName) const;             // Метод, който проверява дали дадена тема съществува
    wxArrayString GetAvailableThemes() const;                   // Метод, който връща списък с наличните теми
    wxString GetCurrentTheme() const { return m_currentTheme; } // Метод, който връща името на текущо приложената тема
    void SetCurrentTheme(const wxString &themeName);            // Метод за задаване на текущата тема

    // System theme related functions // Функции, свързани със системната тема
    void UpdateSystemTheme();                // Метод за актуализиране на темата спрямо системните настройки (напр. тъмен/светъл режим)
    void RegisterWindow(wxWindow *window);   // Метод за регистриране на прозорец, който да бъде уведомяван при смяна на темата
    void UnregisterWindow(wxWindow *window); // Метод за отписване на прозорец от уведомления за смяна на темата
    void InitializeWithSystemTheme();        // Метод за инициализация с текущата системна тема

private:                                                  // Секция с частни членове на класа
    ThemeConfig() = default;                              // Частен конструктор по подразбиране (за Singleton)
    ThemeConfig(const ThemeConfig &) = delete;            // Забранява копи-конструктора
    ThemeConfig &operator=(const ThemeConfig &) = delete; // Забранява оператора за присвояване

    // Helper methods for parsing and applying // Помощни методи за парсване и прилагане
    wxColour ParseColor(const YAML::Node &node) const;                                       // Помощен метод за парсване на цвят от YAML възел
    void ParseThemeColors(const YAML::Node &themeNode, ThemeColors &colors);                 // Помощен метод за парсване на цветовете на тема от YAML възел
    void ApplyColorsRecursively(wxWindow *window, const ThemeColors &colors);                // Помощен метод за рекурсивно прилагане на цветове към прозорец и неговите дъщерни елементи
    void ParseButtonColors(const YAML::Node &buttonNode, ThemeColors::ButtonColors &colors); // Помощен метод за парсване на цветове на бутони от YAML възел
    bool IsSystemDarkMode() const;                                                           // Помощен метод, който проверява дали системната тема е тъмна

    // Member variables // Член-променливи
    YAML::Node m_themes;                           // YAML възел, съхраняващ заредените данни за темите
    std::map<wxString, ThemeColors> m_themeColors; // Карта (map), която съхранява цветовите дефиниции за всяка тема, индексирани по името на темата
    wxString m_currentTheme = "light";             // Текущо избраната тема (по подразбиране "light" - светла)
    std::vector<wxWindow *> m_registeredWindows;   // Вектор от указатели към прозорци, които трябва да бъдат уведомени при промяна на темата
}; // Край на класа ThemeConfig

#endif // THEMECONFIG_H // Край на условната компилация, започната с #ifndef