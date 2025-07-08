#ifndef WINDOW_IDS_H // Ако WINDOW_IDS_H не е дефиниран (предпазва от многократно включване)
#define WINDOW_IDS_H // Дефинира WINDOW_IDS_H, за да се избегне повторно включване на този хедър файл

// It's generally good practice to start custom IDs above wxID_HIGHEST // Обикновено е добра практика да се започва с персонализирани ID-та над wxID_HIGHEST
// or use wxNewId() / wxID_ANY, but for simplicity, we'll keep the existing enum style. // или да се използва wxNewId() / wxID_ANY, но за простота ще запазим съществуващия стил на изброяване.
// Just be mindful of potential clashes with wxWidgets' internal IDs if you add many more. // Просто внимавайте за потенциални конфликти с вътрешните ID-та на wxWidgets, ако добавите много повече.

enum // Декларация на изброяване (enum) за дефиниране на константни идентификатори за прозорци и контроли
{    // Начало на дефинициите на идентификаторите
  // --- MainFrame IDs --- // --- ID-та за MainFrame (Главния прозорец) ---
    ID_BROWSE_ISO = 1, // ID за бутон за избор на ISO файл
    ID_BROWSE_WORKDIR, // ID за бутон за избор на работна директория
    ID_DETECT,         // ID за бутон за откриване на ОС/Дистрибуция
    ID_CANCEL,         // ID за бутон за отмяна/изход от приложението (може да е общ)
    ID_SETTINGS,       // ID за бутон за отваряне на диалоговия прозорец за настройки
    ID_NEXT,           // ID за бутон за преминаване от MainFrame към SecondWindow

    // --- SecondWindow Tab/View IDs --- // --- ID-та за табове/изгледи в SecondWindow ---
    ID_TERMINAL_TAB = 1001, // ID за бутон за превключване към изглед "Терминал"
    ID_SQL_TAB,             // ID за бутон за превключване към изглед "Конфигурация" (предишно SQL)

    // --- SecondWindow Button IDs --- // --- ID-та за бутони в SecondWindow ---
    ID_NEXT_BUTTON,    // ID за бутон в SecondWindow за задействане на "Създай ISO"
    ID_MONGODB_BUTTON, // ID за бутон в горната лента на SecondWindow за показване на MongoDB панела

    // --- MongoDBPanel Communication ID --- // --- ID за комуникация с MongoDBPanel ---
    // ID used for the event sent *from* MongoDBPanel *to* SecondWindow when its internal close button is clicked. // ID, използвано за събитието, изпратено *от* MongoDBPanel *към* SecondWindow, когато вътрешният му бутон за затваряне е натиснат.
    ID_MONGODB_PANEL_CLOSE, // ID за затваряне на MongoDB панела

    // --- ConfigTab Sub-Tab IDs (Ensure these don't clash) --- // --- ID-та за под-табове в ConfigTab (Уверете се, че не се застъпват) ---
    // These were previously defined in ConfigTab.h, centralizing them here // Те бяха предварително дефинирани в ConfigTab.h, централизират се тук
    ID_CONFIG_DESKTOP = 2001, // ID за под-таб "Работен плот" в конфигурацията
    ID_CONFIG_APPS,           // ID за под-таб "Приложения" в конфигурацията
    ID_CONFIG_SYSTEM,         // ID за под-таб "Система" в конфигурацията
    ID_CONFIG_CUSTOMIZE,      // ID за под-таб "Персонализиране" в конфигурацията

    // --- CustomTitleBar Button IDs (Ensure these don't clash) --- // --- ID-та за бутони в CustomTitleBar (Уверете се, че не се застъпват) ---
    // These were previously defined in CustomTitleBar.h // Те бяха предварително дефинирани в CustomTitleBar.h
    ID_MINIMIZE = 3001, // ID за бутон за минимизиране на прозореца
    ID_CLOSE_WINDOW,    // ID за бутон за затваряне на прозореца

    // Add other specific IDs for widgets as needed... // Добавете други специфични ID-та за уиджети при нужда...
    // Example: ID_HOSTNAME_INPUT, ID_LANGUAGE_CHOICE etc. if needed for event handling // Пример: ID_HOSTNAME_INPUT, ID_LANGUAGE_CHOICE и т.н., ако са необходими за обработка на събития

    // --- FlatpakStore Timer IDs (from FlatpakStore.h) --- // --- ID-та за таймери във FlatpakStore (от FlatpakStore.h) ---
    ID_BATCH_TIMER = 4001,   // ID за таймер за пакетна обработка
    ID_LAYOUT_TIMER,         // ID за таймер за оформление
    ID_SHOW_INSTALLED_BUTTON // ID за бутон във FlatpakStore за показване на инсталираните

    // Add other IDs... // Добавете други ID-та...
}; // Край на изброяването

#endif // WINDOW_IDS_H // Край на условната компилация, започната с #ifndef