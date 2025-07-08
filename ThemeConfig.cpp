#include "ThemeConfig.h" // Включва заглавния файл за ThemeConfig
#include <fstream> // Включва заглавния файл за файлови потоци (за четене от файл)
#include <wx/button.h> // Включва заглавния файл за wxButton
#include <wx/textctrl.h> // Включва заглавния файл за wxTextCtrl (текстово поле)
#include <wx/stattext.h> // Включва заглавния файл за wxStaticText (статичен текст/етикет)
#include <wx/gauge.h> // Включва заглавния файл за wxGauge (индикатор за прогрес)
#include <wx/statbox.h> // Включва заглавния файл за wxStaticBox (групираща рамка)
#include "CustomTitleBar.h" // Включва дефиницията на MyButton (предполага се, че CustomTitleBar включва MyButton)
#include "CustomStatusBar.h" // Включва заглавния файл за CustomStatusBar
#include "SystemTheme.h" // Включва заглавния файл за SystemTheme (за проверка на системната тема)
#include "SettingsDialog.h" // Включва ToggleSwitch, ако е необходимо тук (напр. ако SettingsDialog го дефинира или използва)

#ifdef __WXMSW__ // Ако се компилира за Windows (MSW - Microsoft Windows)
#include <windows.h> // Включва заглавния файл на Windows API
#endif // Край на условната компилация

// ... (Методите Get, LoadThemes, Parse* остават същите като в предишната добра версия) ...
ThemeConfig &ThemeConfig::Get() // Реализация на статичния метод Get за достъп до единствения екземпляр (Singleton)
{ // Начало на метода Get
    static ThemeConfig instance; // Създава статичен локален екземпляр (ще бъде създаден само веднъж)
    return instance; // Връща референция към екземпляра
} // Край на метода Get

void ThemeConfig::LoadThemes(const wxString &filename) // Реализация на метода LoadThemes за зареждане на теми от YAML файл
{ // Начало на метода LoadThemes
    std::ifstream fin(filename.ToStdString()); // Отваря файл за четене, преобразувайки wxString към std::string
    if (!fin.is_open()) // Проверява дали файлът е отворен успешно
    { // Начало на блока if (файлът не е отворен)
        wxLogWarning("Failed to open theme file: %s", filename); // Записва предупреждение в лога, че файлът с теми не е отворен
        m_themeColors.clear(); // Изчиства съществуващите заредени цветове на теми
        // Consider adding default themes programmatically here as fallback // Помислете за програмно добавяне на теми по подразбиране тук като резервен вариант
        return; // Прекратява изпълнението на метода
    } // Край на блока if

    try // Начало на try блок за обработка на изключения при парсване на YAML
    { // Начало на try блока
        m_themes = YAML::Load(fin); // Зарежда YAML съдържанието от файла в член-променливата m_themes
        m_themeColors.clear(); // Изчиства съществуващите заредени цветове на теми преди ново зареждане

        wxLogDebug("Loading themes from file: %s", filename); // Записва дебъг съобщение в лога
        for (const auto &theme : m_themes) // Итерира през всички теми в заредения YAML обект
        { // Начало на цикъла for
            wxString themeName = wxString::FromUTF8(theme.first.as<std::string>()); // Взема името на темата (ключа в YAML) и го преобразува в wxString
            ThemeColors colors; // Създава обект ThemeColors за съхранение на цветовете на текущата тема
            ParseThemeColors(theme.second, colors); // Парсва цветовете за текущата тема от YAML възела
            m_themeColors[themeName.Lower()] = colors; // Добавя парснатите цветове към картата m_themeColors, като използва името на темата с малки букви като ключ
            wxLogDebug("Loaded theme: %s", themeName); // Записва дебъг съобщение за заредената тема
        } // Край на цикъла for
        // Ensure default themes exist // Уверете се, че темите по подразбиране съществуват
        if (m_themeColors.find("light") == m_themeColors.end()) // Проверява дали темата "light" съществува
        { // Начало на блока if (тема "light" липсва)
            wxLogWarning("Default 'light' theme missing in %s", filename); // Записва предупреждение, ако темата "light" липсва
        } // Край на блока if
        if (m_themeColors.find("dark") == m_themeColors.end()) // Проверява дали темата "dark" съществува
        { // Начало на блока if (тема "dark" липсва)
            wxLogWarning("Default 'dark' theme missing in %s", filename); // Записва предупреждение, ако темата "dark" липсва
        } // Край на блока if
    } // Край на try блока
    catch (const YAML::Exception &e) // Улавя изключения от тип YAML::Exception
    { // Начало на catch блока
        wxLogError("Error parsing theme file '%s': %s", filename, e.what()); // Записва грешка в лога при проблем с парсването на YAML файла
    } // Край на catch блока
} // Край на метода LoadThemes

void ThemeConfig::ParseButtonColors(const YAML::Node &buttonNode, ThemeColors::ButtonColors &colors) // Реализация на метода ParseButtonColors за парсване на цветове на бутони
{ // Начало на метода ParseButtonColors
    if (buttonNode) // Проверява дали YAML възелът за бутона е валиден
    { // Начало на блока if (валиден възел)
        colors.background = ParseColor(buttonNode["background"]); // Парсва цвета на фона на бутона
        colors.hover = ParseColor(buttonNode["hover"]); // Парсва цвета на бутона при посочване
        colors.pressed = ParseColor(buttonNode["pressed"]); // Парсва цвета на бутона при натискане
        colors.text = ParseColor(buttonNode["text"]); // Парсва цвета на текста на бутона
    } // Край на блока if
    else // Ако YAML възелът за бутона не е валиден
    { // Начало на блока else
        colors.background = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE); // Задава цвят на фона по подразбиране от системните настройки
        colors.text = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT); // Задава цвят на текста по подразбиране от системните настройки
        colors.hover = colors.background; // Задава цвета при посочване да е същият като фона по подразбиране
        colors.pressed = colors.background; // Задава цвета при натискане да е същият като фона по подразбиране
    } // Край на блока else
} // Край на метода ParseButtonColors

wxColour ThemeConfig::ParseColor(const YAML::Node &node) const // Реализация на метода ParseColor за парсване на цвят от YAML възел
{ // Начало на метода ParseColor
    wxColour defaultColour = *wxBLACK; // Задава цвят по подразбиране (черен), ако парсването е неуспешно
    if (!node || !node.IsScalar()) // Проверява дали възелът е валиден и е скаларна стойност
        return defaultColour; // Връща цвета по подразбиране, ако проверката е неуспешна
    std::string hexColor = node.as<std::string>(); // Преобразува стойността на възела в std::string
    if (hexColor.empty() || hexColor[0] != '#' || (hexColor.length() != 7 && hexColor.length() != 9)) // Проверява дали форматът на шестнадесетичния цвят е валиден (#RRGGBB или #RRGGBBAA)
    { // Начало на блока if (невалиден формат)
        wxLogWarning("Invalid color format: '%s'. Expected #RRGGBB.", hexColor); // Записва предупреждение за невалиден формат на цвета
        return defaultColour; // Връща цвета по подразбиране
    } // Край на блока if
    unsigned int r, g, b; // Декларира променливи за червения, зеления и синия компонент на цвета
#ifdef _WIN32 // Ако се компилира за Windows
    int result = sscanf_s(hexColor.c_str() + 1, "%02x%02x%02x", &r, &g, &b); // Използва sscanf_s за парсване на цвета (по-сигурна версия за Windows)
#else // За други операционни системи
    int result = sscanf(hexColor.c_str() + 1, "%02x%02x%02x", &r, &g, &b); // Използва sscanf за парсване на цвета
#endif // Край на условната компилация
    if (result == 3) // Проверява дали са парснати успешно три компонента (R, G, B)
    { // Начало на блока if (успешно парсване)
        return wxColour(r, g, b); // Връща wxColour обект, създаден от парснатите компоненти
    } // Край на блока if
    else // Ако парсването е неуспешно
    { // Начало на блока else
        wxLogWarning("Failed to parse color: '%s'", hexColor); // Записва предупреждение за неуспешно парсване на цвета
        return defaultColour; // Връща цвета по подразбиране
    } // Край на блока else
} // Край на метода ParseColor

void ThemeConfig::ParseThemeColors(const YAML::Node &themeNode, ThemeColors &colors) // Реализация на метода ParseThemeColors за парсване на всички цветове на тема
{ // Начало на метода ParseThemeColors
    colors.titleBar = ParseColor(themeNode["titleBar"]); // Парсва цвета на заглавната лента
    colors.statusBar = ParseColor(themeNode["statusBar"]); // Парсва цвета на статус лентата
    colors.background = ParseColor(themeNode["background"]); // Парсва основния цвят на фона
    colors.text = ParseColor(themeNode["text"]); // Парсва основния цвят на текста
    colors.secondaryText = ParseColor(themeNode["secondaryText"]); // Парсва вторичния цвят на текста
    colors.border = ParseColor(themeNode["border"]); // Парсва цвета на рамките
    ParseButtonColors(themeNode["button"], colors.button); // Парсва цветовете за стандартни бутони
    ParseButtonColors(themeNode["extractButton"], colors.extractButton); // Парсва цветовете за бутона 'extract'
    const auto &inputNode = themeNode["input"]; // Взема YAML възела за цветовете на полетата за въвеждане
    if (inputNode) // Проверява дали възелът за полета за въвеждане е валиден
    { // Начало на блока if (валиден възел за input)
        colors.input.background = ParseColor(inputNode["background"]); // Парсва цвета на фона на полето за въвеждане
        colors.input.border = ParseColor(inputNode["border"]); // Парсва цвета на рамката на полето за въвеждане
        colors.input.text = ParseColor(inputNode["text"]); // Парсва цвета на текста в полето за въвеждане
        colors.input.placeholder = ParseColor(inputNode["placeholder"]); // Парсва цвета на текста-заместител
    } // Край на блока if
    else // Ако възелът за полета за въвеждане не е валиден (резервни стойности)
    { // Начало на блока else (резервни стойности за input)
        colors.input.background = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW); // Задава системен цвят на фона на прозореца
        colors.input.text = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT); // Задава системен цвят на текста в прозореца
        colors.input.border = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW); // Задава системен цвят на сянката на бутона за рамка
        colors.input.placeholder = wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT); // Задава системен цвят на сивия текст за заместител
    } // Край на блока else
    const auto &gaugeNode = themeNode["gauge"]; // Взема YAML възела за цветовете на индикаторите
    if (gaugeNode) // Проверява дали възелът за индикаторите е валиден
    { // Начало на блока if (валиден възел за gauge)
        colors.gauge.background = ParseColor(gaugeNode["background"]); // Парсва цвета на фона на индикатора
        colors.gauge.fill = ParseColor(gaugeNode["fill"]); // Парсва цвета на запълването на индикатора
    } // Край на блока if
    else // Ако възелът за индикаторите не е валиден (резервни стойности)
    { // Начало на блока else (резервни стойности за gauge)
        colors.gauge.background = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE); // Задава системен цвят на лицето на бутона за фон
        colors.gauge.fill = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT); // Задава системен цвят на маркирането за запълване
    } // Край на блока else
    const auto &panelNode = themeNode["panel"]; // Взема YAML възела за цветовете на панелите
    if (panelNode) // Проверява дали възелът за панелите е валиден
    { // Начало на блока if (валиден възел за panel)
        colors.panel.background = ParseColor(panelNode["background"]); // Парсва цвета на фона на панела
        colors.panel.border = ParseColor(panelNode["border"]); // Парсва цвета на рамката на панела
    } // Край на блока if
    else // Ако възелът за панелите не е валиден (резервни стойности)
    { // Начало на блока else (резервни стойности за panel)
        colors.panel.background = colors.background; // Задава основния фон на темата като фон на панела
        colors.panel.border = colors.border; // Задава основния цвят на рамката на темата като рамка на панела
    } // Край на блока else
} // Край на метода ParseThemeColors

void ThemeConfig::ApplyTheme(wxWindow *window, const wxString &themeName) // Реализация на метода ApplyTheme за прилагане на тема към прозорец
{ // Начало на метода ApplyTheme
    wxString lowerThemeName = themeName.Lower(); // Преобразува името на темата в малки букви за последователно сравнение
    auto it = m_themeColors.find(lowerThemeName); // Търси темата в картата със заредени теми
    if (it == m_themeColors.end()) // Ако темата не е намерена
    { // Начало на блока if (темата не е намерена)
        wxLogError("Theme not found: %s. Applying default 'light' theme.", themeName); // Записва грешка, че темата не е намерена и ще се приложи "light"
        lowerThemeName = "light"; // Задава "light" като резервна тема
        it = m_themeColors.find(lowerThemeName); // Отново търси темата (сега "light")
        if (it == m_themeColors.end()) // Ако и темата "light" не е намерена
        { // Начало на блока if (и "light" темата не е намерена)
            wxLogError("Default 'light' theme also not found! Cannot apply theme."); // Записва грешка, че и "light" темата липсва
            return; // Прекратява изпълнението, тъй като не може да приложи тема
        } // Край на блока if
    } // Край на блока if

    const ThemeColors &colors = it->second; // Взема референция към цветовете на намерената тема
    m_currentTheme = lowerThemeName; // Актуализира глобалното състояние на текущата тема

    if (window) // Проверява дали указателят към прозореца е валиден
    { // Начало на блока if (валиден прозорец)
        ApplyColorsRecursively(window, colors); // Рекурсивно прилага цветовете към прозореца и неговите дъщерни елементи
        window->Refresh(); // Принуждава прерисуване на прозореца
        window->Update(); // Незабавно актуализира външния вид на прозореца
    } // Край на блока if
} // Край на метода ApplyTheme

void ThemeConfig::ApplyColorsRecursively(wxWindow *window, const ThemeColors &colors) // Реализация на метода ApplyColorsRecursively за рекурсивно прилагане на цветове
{ // Начало на метода ApplyColorsRecursively
    if (!window) // Ако указателят към прозореца е невалиден
        return; // Прекратява изпълнението

    ApplyThemeToControl(window, colors); // Прилага темата към текущия контрол/прозорец

    wxWindowList &children = window->GetChildren(); // Взема списък с дъщерните прозорци/контроли
    for (wxWindow *child : children) // Итерира през всички дъщерни елементи
    { // Начало на цикъла for
        // Apply recursively // Приложи рекурсивно
        ApplyColorsRecursively(child, colors); // Рекурсивно извикване за всеки дъщерен елемент
    } // Край на цикъла for
} // Край на метода ApplyColorsRecursively

void ThemeConfig::ApplyThemeToControl(wxWindow *control, const ThemeColors &colors) // Реализация на метода ApplyThemeToControl за прилагане на тема към конкретен контрол
{ // Начало на метода ApplyThemeToControl
    if (!control) // Ако указателят към контрола е невалиден
        return; // Прекратява изпълнението

    control->SetBackgroundColour(colors.panel.background); // Задава цвета на фона на панела по подразбиране
    control->SetForegroundColour(colors.text); // Задава основния цвят на текста по подразбиране

    // --- Specific Control Styling --- // --- Стилизиране на специфични контроли ---

    if (auto *frame = dynamic_cast<wxFrame *>(control)) // Ако контролът е wxFrame (основен прозорец)
    { // Начало на блока if (wxFrame)
        frame->SetBackgroundColour(colors.background); // Задава основния фон на темата за главния прозорец
    } // Край на блока if
    else if (auto *dialog = dynamic_cast<wxDialog *>(control)) // Ако контролът е wxDialog (диалогов прозорец)
    { // Начало на блока else if (wxDialog)
        dialog->SetBackgroundColour(colors.background); // Задава основния фон на темата за диалоговия прозорец
    } // Край на блока else if
    else if (auto *titleBar = dynamic_cast<CustomTitleBar *>(control)) // Ако контролът е CustomTitleBar
    { // Начало на блока else if (CustomTitleBar)
        titleBar->SetBackgroundColour(colors.titleBar); // Задава цвета на фона на заглавната лента
        // Theme the text inside the title bar // Тематизиране на текста вътре в заглавната лента
        if (wxStaticText *titleText = titleBar->GetTitleTextWidget()) // Ако има уиджет за текст на заглавието
        { // Начало на блока if (има titleText)
            titleText->SetForegroundColour(colors.text);     // Използва основния цвят на текста за заглавието
            titleText->SetBackgroundColour(colors.titleBar); // Съответства на фона на заглавната лента
        } // Край на блока if
        // Refresh the internal buttons (they will pick up theme in their OnPaint) // Опреснява вътрешните бутони (те ще вземат темата в своя OnPaint)
        titleBar->RefreshControls(); // Извиква метод за опресняване на контролите в заглавната лента
        titleBar->Refresh(); // Опреснява самата заглавна лента
    } // Край на блока else if
    else if (auto *statusBar = dynamic_cast<CustomStatusBar *>(control)) // Ако контролът е CustomStatusBar
    { // Начало на блока else if (CustomStatusBar)
        statusBar->SetBackgroundColour(colors.statusBar); // Задава цвета на фона на статус лентата
        // Theme text inside status bar (assuming CustomStatusBar has a method or direct access) // Тематизиране на текста в статус лентата (предполага се, че CustomStatusBar има метод или директен достъп)
        // statusBar->GetStatusTextWidget()->SetForegroundColour(colors.text); // Пример
        // statusBar->GetStatusTextWidget()->SetBackgroundColour(colors.statusBar);
        statusBar->Refresh(); // Опреснява статус лентата
    } // Край на блока else if
    else if (auto *button = dynamic_cast<wxButton *>(control)) // Ако контролът е wxButton (стандартен бутон)
    { // Начало на блока else if (wxButton)
        // Exclude buttons inside CustomTitleBar if MyButton handles itself // Изключва бутони вътре в CustomTitleBar, ако MyButton се обработва сам
        if (!dynamic_cast<CustomTitleBar *>(control->GetParent())) // Проверява дали родителят не е CustomTitleBar
        { // Начало на блока if (не е бутон в CustomTitleBar)
            if (control->GetId() == ID_NEXT) // Ако ID-то на бутона е ID_NEXT (специфичен бутон)
            { // Начало на блока if (ID_NEXT)
                button->SetBackgroundColour(colors.extractButton.background); // Задава цветовете за бутона "extract"
                button->SetForegroundColour(colors.extractButton.text);
            } // Край на блока if
            else // За общи бутони
            { // Начало на блока else (общ бутон)
                button->SetBackgroundColour(colors.button.background); // Задава стандартните цветове за бутон
                button->SetForegroundColour(colors.button.text);
            } // Край на блока else
        } // Край на блока if
    } // Край на блока else if
    else if (auto *myButton = dynamic_cast<MyButton *>(control)) // Ако контролът е MyButton (персонализиран бутон)
    { // Начало на блока else if (MyButton)
        // MyButton now gets theme colors in its OnPaint, just needs refresh // MyButton сега получава цветовете на темата в своя OnPaint, просто се нуждае от опресняване
        myButton->Refresh(); // Опреснява персонализирания бутон
    } // Край на блока else if
    else if (auto *textCtrl = dynamic_cast<wxTextCtrl *>(control)) // Ако контролът е wxTextCtrl (текстово поле)
    { // Начало на блока else if (wxTextCtrl)
        textCtrl->SetBackgroundColour(colors.input.background); // Задава цвета на фона на полето за въвеждане
        textCtrl->SetForegroundColour(colors.input.text); // Задава цвета на текста в полето за въвеждане
    } // Край на блока else if
    else if (auto *staticText = dynamic_cast<wxStaticText *>(control)) // Ако контролът е wxStaticText (статичен текст)
    { // Начало на блока else if (wxStaticText)
        // Make labels use secondary text color, unless it's the title bar text // Накарайте етикетите да използват вторичния цвят на текста, освен ако не е текстът на заглавната лента
        if (!dynamic_cast<CustomTitleBar *>(control->GetParent())) // Ако родителят не е CustomTitleBar
        { // Начало на блока if (не е текст в CustomTitleBar)
            staticText->SetForegroundColour(colors.secondaryText); // Задава вторичния цвят на текста
            // Match background of its parent panel // Съответства на фона на родителския си панел
            wxWindow *parent = control->GetParent(); // Взема родителския прозорец
            if (parent) // Ако има родител
            { // Начало на блока if (има родител)
                control->SetBackgroundColour(parent->GetBackgroundColour()); // Задава цвета на фона на контрола да съответства на този на родителя
            } // Край на блока if
        } // Край на блока if
        else // Ако е текст в CustomTitleBar
        { // Начало на блока else (текст в CustomTitleBar)
            // Title bar text is handled when styling CustomTitleBar // Текстът на заглавната лента се обработва при стилизиране на CustomTitleBar
            control->SetForegroundColour(colors.text); // Задава основния цвят на текста
            control->SetBackgroundColour(colors.titleBar); // Задава цвета на фона на заглавната лента
        } // Край на блока else
    } // Край на блока else if
    else if (auto *gauge = dynamic_cast<wxGauge *>(control)) // Ако контролът е wxGauge (индикатор за прогрес)
    { // Начало на блока else if (wxGauge)
        // Note: Gauge appearance is heavily OS-dependent // Забележка: Външният вид на Gauge силно зависи от операционната система
        // Try setting both FG and BG, but it might not work as expected visually // Опитайте да зададете и FG, и BG, но може да не работи визуално както се очаква
        gauge->SetBackgroundColour(colors.gauge.background); // Задава цвета на фона на индикатора
        gauge->SetForegroundColour(colors.gauge.fill); // Задава цвета на запълването на индикатора
    } // Край на блока else if
    else if (auto *staticBox = dynamic_cast<wxStaticBox *>(control)) // Ако контролът е wxStaticBox (групираща рамка)
    { // Начало на блока else if (wxStaticBox)
        staticBox->SetForegroundColour(colors.secondaryText); // Задава цвета на етикета на рамката
        // Background is usually system-drawn, but set it anyway // Фонът обикновено се рисува от системата, но го задайте все пак
        staticBox->SetBackgroundColour(colors.panel.background); // Задава цвета на фона на панела
    } // Край на блока else if
    else if (auto *choice = dynamic_cast<wxChoice *>(control)) // Ако контролът е wxChoice (падащ списък)
    { // Начало на блока else if (wxChoice)
        choice->SetBackgroundColour(colors.input.background); // Задава цвета на фона на полето за въвеждане
        choice->SetForegroundColour(colors.input.text); // Задава цвета на текста в полето за въвеждане
    } // Край на блока else if
    else if (auto *checkbox = dynamic_cast<wxCheckBox *>(control)) // Ако контролът е wxCheckBox (отметка)
    { // Начало на блока else if (wxCheckBox)
        checkbox->SetForegroundColour(colors.text); // Задава цвета на текста на етикета
                                                    // Checkbox BG usually matches parent // Фонът на Checkbox обикновено съответства на родителя
        wxWindow *parent = control->GetParent(); // Взема родителския прозорец
        if (parent) // Ако има родител
        { // Начало на блока if (има родител)
            checkbox->SetBackgroundColour(parent->GetBackgroundColour()); // Задава цвета на фона на отметката да съответства на този на родителя
        } // Край на блока if
    } // Край на блока else if
    else if (auto *toggle = dynamic_cast<ToggleSwitch *>(control)) // Ако контролът е ToggleSwitch (персонализиран превключвател)
    { // Начало на блока else if (ToggleSwitch)
        // Toggle switch redraws itself using theme colors in OnPaint // Превключвателят се прерисува сам, използвайки цветовете на темата в OnPaint
        toggle->Refresh(); // Опреснява превключвателя
    } // Край на блока else if
    // Handle generic panels that aren't special types // Обработка на общи панели, които не са специални типове
    else if (auto *panel = dynamic_cast<wxPanel *>(control)) // Ако контролът е wxPanel (обикновен панел)
    { // Начало на блока else if (wxPanel)
        // Only apply if it's not one of the special panels already handled // Прилагайте само ако не е един от специалните панели, които вече са обработени
        if (!dynamic_cast<CustomTitleBar *>(control) && !dynamic_cast<CustomStatusBar *>(control)) // Ако не е CustomTitleBar или CustomStatusBar
        { // Начало на блока if (не е специален панел)
            panel->SetBackgroundColour(colors.panel.background); // Задава цвета на фона на панела
        } // Край на блока if
    } // Край на блока else if

    // Don't refresh every single control, refresh the top-level window once // Не опреснявайте всеки отделен контрол, опреснете прозореца от най-високо ниво веднъж
    // control->Refresh();
} // Край на метода ApplyThemeToControl

// ... (Методите HasTheme, GetAvailableThemes, ParseThemeColors остават същите) ...
bool ThemeConfig::HasTheme(const wxString &themeName) const // Реализация на метода HasTheme за проверка на съществуването на тема
{ // Начало на метода HasTheme
    return m_themeColors.count(themeName.Lower()) > 0; // Проверява дали темата (с малки букви) съществува в картата m_themeColors
} // Край на метода HasTheme

wxArrayString ThemeConfig::GetAvailableThemes() const // Реализация на метода GetAvailableThemes за получаване на списък с наличните теми
{ // Начало на метода GetAvailableThemes
    wxArrayString themes; // Създава wxArrayString за съхранение на имената на темите
    for (const auto &[name, _] : m_themeColors) // Итерира през картата с теми (name е ключът, _ е стойността, която не се използва тук)
    { // Начало на цикъла for
        themes.Add(name); // Добавя името на темата към списъка
    } // Край на цикъла for
    if (std::find(themes.begin(), themes.end(), "light") == themes.end() && HasTheme("light")) // Ако "light" не е в списъка, но съществува като тема
        themes.Add("light"); // Добавя "light" към списъка
    if (std::find(themes.begin(), themes.end(), "dark") == themes.end() && HasTheme("dark")) // Ако "dark" не е в списъка, но съществува като тема
        themes.Add("dark"); // Добавя "dark" към списъка
    return themes; // Връща списъка с наличните теми
} // Край на метода GetAvailableThemes

const ThemeColors &ThemeConfig::GetThemeColors(const wxString &themeName) const // Реализация на метода GetThemeColors за получаване на цветовете на дадена тема
{ // Начало на метода GetThemeColors
    static const ThemeColors defaultColors = [] // Статична ламбда функция за инициализация на цветове по подразбиране (ще се изпълни веднъж)
    { // Начало на ламбда функцията
        ThemeColors dc; // Създава обект ThemeColors за цветовете по подразбиране
        dc.background = wxSystemSettings::GetColour(wxSYS_COLOUR_APPWORKSPACE); // Задава системен цвят за фон на работното пространство на приложението
        dc.text = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT); // Задава системен цвят за текст в прозорец
        dc.secondaryText = wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT); // Задава системен цвят за сив текст
        dc.border = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW); // Задава системен цвят за сянка на бутон (използва се за рамка)
        dc.titleBar = wxSystemSettings::GetColour(wxSYS_COLOUR_ACTIVECAPTION); // Задава системен цвят за активна заглавна лента
        dc.statusBar = wxSystemSettings::GetColour(wxSYS_COLOUR_MENUBAR); // Задава системен цвят за лента с менюта (използва се за статус лента)
        dc.button.background = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE); // Задава системен цвят за лице на бутон
        dc.button.text = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT); // Задава системен цвят за текст на бутон
        dc.extractButton = dc.button; // По подразбиране цветовете на бутона "extract" са същите като на нормален бутон
        dc.input.background = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW); // Задава системен цвят за фон на прозорец (за полета за въвеждане)
        dc.input.text = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT); // Задава системен цвят за текст в прозорец (за полета за въвеждане)
        dc.gauge.background = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE); // Задава системен цвят за лице на бутон (за фон на индикатор)
        dc.gauge.fill = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT); // Задава системен цвят за маркиране (за запълване на индикатор)
        dc.panel.background = dc.background; // Задава фона на панела да е същият като основния фон
        return dc; // Връща структурата с цветове по подразбиране
    }(); // Незабавно извикване на ламбда функцията

    wxString lowerThemeName = themeName.Lower(); // Преобразува името на темата в малки букви
    auto it = m_themeColors.find(lowerThemeName); // Търси темата в картата
    if (it != m_themeColors.end()) // Ако темата е намерена
    { // Начало на блока if (темата е намерена)
        return it->second; // Връща цветовете на намерената тема
    } // Край на блока if
    else // Ако темата не е намерена
    { // Начало на блока else (темата не е намерена)
        wxLogWarning("GetThemeColors: Theme '%s' not found, returning default.", themeName); // Записва предупреждение, че темата не е намерена
        auto lightIt = m_themeColors.find("light"); // Опитва се да намери темата "light"
        if (lightIt != m_themeColors.end()) // Ако темата "light" е намерена
        { // Начало на блока if ("light" е намерена)
            return lightIt->second; // Връща цветовете на темата "light" като предпочитан резервен вариант
        } // Край на блока if
        return defaultColors; // Абсолютен резервен вариант: връща цветовете по подразбиране
    } // Край на блока else
} // Край на метода GetThemeColors

bool ThemeConfig::IsSystemDarkMode() const // Реализация на метода IsSystemDarkMode за проверка дали системната тема е тъмна
{ // Начало на метода IsSystemDarkMode
    return SystemTheme::IsDarkMode(); // Извиква статичния метод IsDarkMode от класа SystemTheme
} // Край на метода IsSystemDarkMode

void ThemeConfig::UpdateSystemTheme() // Реализация на метода UpdateSystemTheme за актуализиране на темата спрямо системната
{ // Начало на метода UpdateSystemTheme
    wxString systemThemeName = IsSystemDarkMode() ? "dark" : "light"; // Определя името на системната тема ("dark" или "light")
    wxLogInfo("System theme detected: %s", systemThemeName); // Записва информация за откритата системна тема

    if (!HasTheme(systemThemeName)) // Ако дефинираната системна тема не съществува в заредените теми
    { // Начало на блока if (системната тема не е дефинирана)
        wxLogWarning("System theme '%s' is not defined. Using '%s'.", systemThemeName, m_currentTheme); // Записва предупреждение
        systemThemeName = "light"; // Задава "light" като резервна системна тема
        if (!HasTheme(systemThemeName)) // Ако и "light" темата не е дефинирана
        { // Начало на блока if (и "light" не е дефинирана)
            wxLogError("Fallback 'light' theme not found!"); // Записва грешка
            return; // Прекратява изпълнението
        } // Край на блока if
    } // Край на блока if

    if (m_currentTheme != systemThemeName) // Ако текущата тема е различна от системната
    { // Начало на блока if (темите са различни)
        wxLogInfo("Applying system theme: %s", systemThemeName); // Записва информация за прилагане на системната тема
        m_currentTheme = systemThemeName; // Задава текущата тема да бъде системната

        for (wxWindow *window : m_registeredWindows) // Итерира през всички регистрирани прозорци
        { // Начало на цикъла for
            if (window) // Ако указателят към прозореца е валиден
            { // Начало на блока if (валиден прозорец)
                window->CallAfter([this, window, systemThemeName]() // Извиква ламбда функция асинхронно след обработка на текущите събития
                                  { // Начало на ламбда функцията
                    if (window) { // Проверява отново вътре в CallAfter, тъй като прозорецът може да е бил унищожен междувременно
                       ApplyTheme(window, systemThemeName); // Прилага новата системна тема към прозореца
                    } }); // Край на ламбда функцията
            } // Край на блока if
        } // Край на цикъла for
    } // Край на блока if
    else // Ако текущата тема вече е същата като системната
    { // Начало на блока else (темите са еднакви)
        wxLogInfo("System theme (%s) already applied.", systemThemeName); // Записва информация, че системната тема вече е приложена
    } // Край на блока else
} // Край на метода UpdateSystemTheme

void ThemeConfig::RegisterWindow(wxWindow *window) // Реализация на метода RegisterWindow за регистриране на прозорец за обновяване на темата
{ // Начало на метода RegisterWindow
    if (window && std::find(m_registeredWindows.begin(), m_registeredWindows.end(), window) == m_registeredWindows.end()) // Ако прозорецът е валиден и все още не е регистриран
    { // Начало на блока if (валиден и нерегистриран прозорец)
        m_registeredWindows.push_back(window); // Добавя прозореца към списъка с регистрирани прозорци
        wxLogDebug("Registered window %p for theme updates.", window); // Записва дебъг съобщение за регистрацията
    } // Край на блока if
} // Край на метода RegisterWindow

void ThemeConfig::UnregisterWindow(wxWindow *window) // Реализация на метода UnregisterWindow за отписване на прозорец
{ // Начало на метода UnregisterWindow
    auto it = std::find(m_registeredWindows.begin(), m_registeredWindows.end(), window); // Търси прозореца в списъка с регистрирани прозорци
    if (it != m_registeredWindows.end()) // Ако прозорецът е намерен
    { // Начало на блока if (прозорецът е намерен)
        m_registeredWindows.erase(it); // Премахва прозореца от списъка
        wxLogDebug("Unregistered window %p from theme updates.", window); // Записва дебъг съобщение за отписването
    } // Край на блока if
} // Край на метода UnregisterWindow

void ThemeConfig::InitializeWithSystemTheme() // Реализация на метода InitializeWithSystemTheme за инициализация с текущата системна тема
{ // Начало на метода InitializeWithSystemTheme
    LoadThemes(); // Зарежда темите от файла (използвайки името по подразбиране)
    UpdateSystemTheme(); // Актуализира темата спрямо системната
} // Край на метода InitializeWithSystemTheme

void ThemeConfig::SetCurrentTheme(const wxString &themeName) // Реализация на метода SetCurrentTheme за задаване на текущата тема
{ // Начало на метода SetCurrentTheme
    wxString lowerName = themeName.Lower(); // Преобразува името на темата в малки букви
    if (HasTheme(lowerName)) // Ако темата съществува
    { // Начало на блока if (темата съществува)
        m_currentTheme = lowerName; // Задава новата текуща тема
        wxLogInfo("Current theme set to: %s", m_currentTheme); // Записва информация за промяната
    } // Край на блока if
    else // Ако темата не съществува
    { // Начало на блока else (темата не съществува)
        wxLogWarning("Attempted to set unknown theme: %s", themeName); // Записва предупреждение за опит за задаване на неизвестна тема
    } // Край на блока else
} // Край на метода SetCurrentTheme