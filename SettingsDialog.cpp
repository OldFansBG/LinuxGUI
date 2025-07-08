#include "SettingsDialog.h" // Включва заглавния файл SettingsDialog.h
#include "CustomTitleBar.h" // For parent cast // Включва CustomTitleBar.h (за преобразуване на типа на родителя)
#include <wx/dcbuffer.h>    // Включва библиотека за буфериран контекст на устройство (DC)
#include <wx/graphics.h>    // Включва библиотека за графичен контекст
#include <wx/stattext.h>    // Ensure included // Уверява се, че е включена библиотеката за статичен текст

wxDEFINE_EVENT(EVT_TOGGLE_SWITCH, wxCommandEvent); // Дефинира нов тип събитие EVT_TOGGLE_SWITCH

// --- ToggleSwitch Implementation (Same as before, OnPaint updated) --- // --- Имплементация на ToggleSwitch (същата като преди, OnPaint е актуализиран) ---
wxBEGIN_EVENT_TABLE(ToggleSwitch, wxControl) // Начало на дефиницията на таблицата на събитията за ToggleSwitch
    EVT_PAINT(ToggleSwitch::OnPaint)         // Свързва събитието за прерисуване (EVT_PAINT) с метода OnPaint
    EVT_LEFT_DOWN(ToggleSwitch::OnMouse)     // Свързва събитието за натискане на левия бутон на мишката (EVT_LEFT_DOWN) с метода OnMouse
    wxEND_EVENT_TABLE()                      // Край на дефиницията на таблицата на събитията

    ToggleSwitch::ToggleSwitch(wxWindow *parent, wxWindowID id, bool initialState, const wxPoint &pos, const wxSize &size) // Конструктор на ToggleSwitch
    : wxControl(parent, id, pos, size, wxBORDER_NONE), m_value(initialState),                                              // Инициализация на базовия клас wxControl (без рамка), инициализация на m_value
      m_thumbColor(*wxWHITE)                                                                                               // Thumb is usually white // Инициализация на цвета на "палеца" (обикновено е бял)
{                                                                                                                          // Начало на тялото на конструктора
    SetBackgroundStyle(wxBG_STYLE_PAINT);                                                                                  // Задава стил на фона, който изисква ръчно прерисуване
    SetMinSize(size);                                                                                                      // Задава минимален размер на контролата
    // Colors will be set dynamically in OnPaint based on theme // Цветовете ще се задават динамично в OnPaint въз основа на темата
} // Край на тялото на конструктора

void ToggleSwitch::SetValue(bool value)                   // Метод за задаване на стойността на ToggleSwitch
{                                                         // Начало на тялото на SetValue
    if (m_value != value)                                 // Ако новата стойност е различна от текущата
    {                                                     // Начало на блока if
        m_value = value;                                  // Задава новата стойност
        Refresh();                                        // Прерисува контролата
        wxCommandEvent event(EVT_TOGGLE_SWITCH, GetId()); // Създава ново събитие от тип EVT_TOGGLE_SWITCH
        event.SetInt(m_value ? 1 : 0);                    // Задава цяло число към събитието (1 за true, 0 за false)
        event.SetEventObject(this);                       // Задава обекта, изпратил събитието (текущия ToggleSwitch)
        GetEventHandler()->ProcessEvent(event);           // Изпраща събитието за обработка
    } // Край на блока if
} // Край на тялото на SetValue

void ToggleSwitch::OnPaint(wxPaintEvent &event) // Метод за обработка на събитието за прерисуване
{                                               // Начало на тялото на OnPaint
    wxAutoBufferedPaintDC dc(this);             // Use buffered DC // Използва автоматично буфериран контекст на устройство за рисуване
    dc.SetBackground(*wxTRANSPARENT_BRUSH);     // Задава прозрачна четка за фона
    dc.Clear();                                 // Изчиства областта за рисуване

    wxSize size = GetSize();       // Взема размера на контролата
    int width = size.GetWidth();   // Взема ширината
    int height = size.GetHeight(); // Взема височината
    int radius = height / 2;       // Изчислява радиуса за заобляне (половината от височината)

    // --- Get current theme colors INSIDE OnPaint --- // --- Вземане на текущите цветове на темата ВЪТРЕ в OnPaint ---
    const ThemeColors &colors = ThemeConfig::Get().GetThemeColors(ThemeConfig::Get().GetCurrentTheme()); // Взема цветовете на текущата глобална тема
    m_trackOnColor = colors.button.background;                                                           // Use button background for ON state // Използва цвета на фона на бутон за състояние ВКЛЮЧЕНО
    m_trackOffColor = colors.input.background;                                                           // Use input background for OFF state // Използва цвета на фона на поле за въвеждане за състояние ИЗКЛЮЧЕНО

    wxGraphicsContext *gc = wxGraphicsContext::Create(dc); // Създава графичен контекст от DC за по-сложно рисуване
    if (gc)                                                // Ако графичният контекст е създаден успешно
    {                                                      // Начало на блока if (gc)
        gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);         // Enable anti-aliasing // Включва анти-алиасинг (изглаждане на ръбовете)

        // Draw track // Рисуване на "пистата"
        gc->SetBrush(wxBrush(m_value ? m_trackOnColor : m_trackOffColor)); // Задава четка с цвят според състоянието (включено/изключено)
        gc->SetPen(*wxTRANSPARENT_PEN);                                    // Задава прозрачна писалка (без контур)
        gc->DrawRoundedRectangle(0, 0, width, height, radius);             // Рисува заоблен правоъгълник за пистата

        // Draw thumb // Рисуване на "палеца" (плъзгача)
        gc->SetBrush(wxBrush(m_thumbColor)); // Задава четка с цвета на палеца
        // Add a subtle border to the thumb for definition // Добавяне на фина рамка към палеца за по-добра дефиниция
        gc->SetPen(wxPen(colors.secondaryText, 1));                    // Use secondary text color for border // Използва вторичния цвят на текста за рамката
        int thumbDiameter = height - 4;                                // Изчислява диаметъра на палеца (малко по-малък от височината)
        int thumbX = m_value ? width - thumbDiameter - 2 : 2;          // Изчислява X координатата на палеца според състоянието
        int thumbY = 2;                                                // Y координата на палеца (с малък отстъп)
        gc->DrawEllipse(thumbX, thumbY, thumbDiameter, thumbDiameter); // Рисува елипса (кръг) за палеца

        delete gc; // Освобождава графичния контекст
    } // Край на блока if (gc)
} // Край на тялото на OnPaint

void ToggleSwitch::OnMouse(wxMouseEvent &event) // Метод за обработка на събития от мишката
{                                               // Начало на тялото на OnMouse
    SetValue(!m_value);                         // Променя стойността на превключвателя на противоположната
} // Край на тялото на OnMouse

//Имплементация на SettingsDialog ---
wxBEGIN_EVENT_TABLE(SettingsDialog, wxDialog)                                 // Начало на дефиницията на таблицата на събитията за SettingsDialog
    EVT_BUTTON(wxID_OK, SettingsDialog::OnOK)                                 // Свързва събитието от бутон с ID wxID_OK с метода OnOK
    EVT_BUTTON(wxID_CANCEL, SettingsDialog::OnCancel)                         // Свързва събитието от бутон с ID wxID_CANCEL с метода OnCancel
    EVT_BUTTON(wxID_ANY, SettingsDialog::OnThemeButtonClick)                  // Catches both theme buttons // Свързва събитието от бутон с всяко ID (wxID_ANY) с метода OnThemeButtonClick (улавя и двата бутона за тема)
    EVT_PAINT(SettingsDialog::OnPaint)                                        // Свързва събитието за прерисуване с метода OnPaint
#ifdef __WXMSW__                                                              // Ако се компилира за Windows
    EVT_COMMAND(wxID_ANY, EVT_TOGGLE_SWITCH, SettingsDialog::OnToggleChanged) // Свързва персонализираното събитие EVT_TOGGLE_SWITCH (от всяка контрола) с метода OnToggleChanged
#endif                                                                        // Край на условната компилация
    wxEND_EVENT_TABLE()                                                       // Край на дефиницията на таблицата на събитията

    SettingsDialog::SettingsDialog(wxWindow *parent)                      // Конструктор на SettingsDialog
    : wxDialog(parent, wxID_ANY, "Settings",                              // Инициализация на базовия клас wxDialog с родител, ID, заглавие "Settings"
               wxDefaultPosition, wxSize(500, 450),                       // Позиция по подразбиране, размер 500x450
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxFRAME_SHAPED) // Стилове на диалога: по подразбиране, с възможност за преоразмеряване, с форма
{                                                                         // Начало на тялото на конструктора
    SetBackgroundStyle(wxBG_STYLE_PAINT);                                 // Задава стил на фона, който изисква ръчно прерисуване

    // Initialize the dialog's theme state from the global config // Инициализиране на състоянието на темата на диалога от глобалната конфигурация
    m_selectedThemeInDialog = ThemeConfig::Get().GetCurrentTheme(); // Задава избраната тема в диалога да бъде текущата глобална тема

    CreateContent(); // Извиква метод за създаване на съдържанието на диалога

    // Apply the initial theme to the dialog itself // Прилагане на началната тема към самия диалог
    ApplyTheme(m_selectedThemeInDialog); // Извиква метод за прилагане на темата

    Centre(); // Центрира диалога на екрана
} // Край на тялото на конструктора

// Applies theme colors to the controls WITHIN the settings dialog // Прилага цветовете на темата към контролите ВЪТРЕ в диалога за настройки
void SettingsDialog::ApplyTheme(const wxString &theme)              // Метод за прилагане на тема
{                                                                   // Начало на тялото на ApplyTheme
    ThemeConfig &config = ThemeConfig::Get();                       // Взема референция към обекта ThemeConfig
    if (!config.HasTheme(theme))                                    // Ако конфигурацията няма тема с даденото име
    {                                                               // Начало на блока if
        wxLogError("SettingsDialog: Theme '%s' not found.", theme); // Логира грешка, че темата не е намерена
        return;                                                     // Прекратява изпълнението на метода
    } // Край на блока if
    m_selectedThemeInDialog = theme.Lower(); // Update internal state // Актуализира вътрешното състояние на избраната тема (с малки букви)

    const ThemeColors &colors = config.GetThemeColors(m_selectedThemeInDialog); // Взема цветовете на избраната тема

    // --- Update Dialog's Cached Colors --- // --- Актуализиране на кешираните цветове на диалога ---
    m_bgColor = colors.background;              // Задава цвета на фона
    m_textColor = colors.text;                  // Задава цвета на текста
    m_labelColor = colors.secondaryText;        // Задава цвета на етикетите
    m_primaryColor = colors.button.background;  // Задава основния цвят (за фон на бутони)
    m_secondaryColor = colors.input.background; // Задава вторичния цвят (за фон на полета за въвеждане)
    m_inputBgColor = colors.input.background;   // Задава цвета на фона на полета за въвеждане
    m_inputFgColor = colors.input.text;         // Задава цвета на текста в полета за въвеждане

    // --- Apply to Dialog and Main Panel --- // --- Прилагане към диалога и главния панел ---
    SetBackgroundColour(m_bgColor);                  // Задава цвета на фона на диалога
    if (m_mainPanel)                                 // Ако главният панел съществува
        m_mainPanel->SetBackgroundColour(m_bgColor); // Задава цвета на фона на главния панел

    // --- Apply to Specific Controls --- // --- Прилагане към специфични контроли ---
    if (m_titleText)                                   // Ако заглавният текст съществува
        m_titleText->SetForegroundColour(m_textColor); // Задава цвета на текста на заглавието

    // Theme Section Panel and Label // Панел и етикет на секцията за тема
    wxWindow *themePanel = m_lightButton ? m_lightButton->GetParent() : nullptr; // Взема родителския панел на бутона за светла тема (ако съществува)
    if (themePanel)                                                              // Ако панелът за тема съществува
    {                                                                            // Начало на блока if (themePanel)
        themePanel->SetBackgroundColour(m_bgColor);                              // Match dialog background // Задава цвета на фона на панела да съответства на фона на диалога
        wxWindowList children = themePanel->GetChildren();                       // Взема списък с дъщерните контроли на панела
        for (wxWindow *child : children)                                         // Итерира през дъщерните контроли
        {                                                                        // Начало на цикъла for
            if (auto *label = dynamic_cast<wxStaticText *>(child))               // Ако контролата е статичен текст (етикет)
            {                                                                    // Начало на блока if (label)
                label->SetForegroundColour(m_labelColor);                        // Задава цвета на текста на етикета
                label->SetBackgroundColour(m_bgColor);                           // Match panel background // Задава цвета на фона на етикета да съответства на фона на панела
            } // Край на блока if (label)
            // Apply to button sizer panel if nested // Прилагане към панела на сайзера за бутони, ако е вложен
            else if (auto *panel = dynamic_cast<wxPanel *>(child)) // Ако контролата е панел
            {                                                      // Начало на блока else if (panel)
                panel->SetBackgroundColour(m_bgColor);             // Задава цвета на фона на панела
            } // Край на блока else if (panel)
            else if (auto *sizer = dynamic_cast<wxBoxSizer *>(child->GetSizer())) // Ако контролата има сайзер от тип wxBoxSizer
            {                                                                     // Начало на блока else if (sizer)
                                                                                  // Could iterate sizer items if needed, but direct children usually sufficient // Може да се итерират елементите на сайзера, ако е необходимо, но директните дъщерни елементи обикновено са достатъчни
            } // Край на блока else if (sizer)
        } // Край на цикъла for
    } // Край на блока if (themePanel)

    // Container Section Box and Inner Panel // Секция за контейнери (кутия и вътрешен панел)
    if (m_containerSizer)                                    // Ако сайзерът за контейнери съществува
    {                                                        // Начало на блока if (m_containerSizer)
        wxStaticBox *box = m_containerSizer->GetStaticBox(); // Взема статичната кутия от сайзера
        if (box)                                             // Ако кутията съществува
        {                                                    // Начало на блока if (box)
            box->SetForegroundColour(m_labelColor);          // Задава цвета на текста на кутията
            // Find the inner panel to set its background // Намиране на вътрешния панел, за да се зададе неговият фон
            wxWindowList boxChildren = box->GetChildren();                                     // Взема дъщерните контроли на кутията
            if (!boxChildren.IsEmpty())                                                        // Ако има дъщерни контроли
            {                                                                                  // Начало на блока if (!boxChildren.IsEmpty())
                wxPanel *innerPanel = dynamic_cast<wxPanel *>(boxChildren.Item(0)->GetData()); // Взема първата дъщерна контрола като панел
                if (innerPanel)                                                                // Ако вътрешният панел съществува
                {                                                                              // Начало на блока if (innerPanel)
                    innerPanel->SetBackgroundColour(m_bgColor);                                // Match dialog BG // Задава цвета на фона на вътрешния панел да съответства на фона на диалога
                                                                                               // Style controls inside the inner panel // Стилизиране на контролите вътре във вътрешния панел
                    wxSizer *innerSizer = innerPanel->GetSizer();                              // Взема сайзера на вътрешния панел
                    if (innerSizer)                                                            // Ако вътрешният сайзер съществува
                    {                                                                          // Начало на блока if (innerSizer)
                        wxSizerItemList items = innerSizer->GetChildren();                     // Взема елементите на вътрешния сайзер
                        for (wxSizerItem *item : items)                                        // Итерира през елементите
                        {                                                                      // Начало на цикъла for (items)
                            wxPanel *rowPanel = dynamic_cast<wxPanel *>(item->GetWindow());    // Panels for Docker/WSL rows // Панели за редовете Docker/WSL
                            if (rowPanel)                                                      // Ако елементът е панел за ред
                            {                                                                  // Начало на блока if (rowPanel)
                                rowPanel->SetBackgroundColour(m_bgColor);                      // Задава цвета на фона на панела за ред
                                wxSizer *rowSizer = rowPanel->GetSizer();                      // Взема сайзера на панела за ред
                                if (rowSizer)                                                  // Ако сайзерът на реда съществува
                                {                                                              // Начало на блока if (rowSizer)
                                    wxSizerItemList rowItems = rowSizer->GetChildren();        // Взема елементите на сайзера на реда
                                    for (wxSizerItem *rowItem : rowItems)                      // Итерира през елементите на реда
                                    {                                                          // Начало на цикъла for (rowItems)
                                        wxWindow *widget = rowItem->GetWindow();               // Взема контролата (уиджет)
                                        if (auto *text = dynamic_cast<wxStaticText *>(widget)) // Ако е статичен текст
                                        {                                                      // Начало на блока if (text)
                                            text->SetForegroundColour(m_textColor);            // Задава цвета на текста
                                            text->SetBackgroundColour(m_bgColor);              // Задава цвета на фона
                                        } // Край на блока if (text)
                                        else if (auto *toggle = dynamic_cast<ToggleSwitch *>(widget)) // Ако е ToggleSwitch
                                        {                                                             // Начало на блока else if (toggle)
                                            toggle->Refresh();                                        // Toggle redraws using current theme // Превключвателят се прерисува, използвайки текущата тема
                                        } // Край на блока else if (toggle)
                                    } // Край на цикъла for (rowItems)
                                } // Край на блока if (rowSizer)
                            } // Край на блока if (rowPanel)
                        } // Край на цикъла for (items)
                    } // Край на блока if (innerSizer)
                } // Край на блока if (innerPanel)
            } // Край на блока if (!boxChildren.IsEmpty())
        } // Край на блока if (box)
    } // Край на блока if (m_containerSizer)

    // OK/Cancel Buttons // Бутони OK/Cancel
    if (m_okButton)                                      // Ако бутонът OK съществува
    {                                                    // Начало на блока if (m_okButton)
        m_okButton->SetBackgroundColour(m_primaryColor); // Задава основния цвят за фон
        m_okButton->SetForegroundColour(*wxWHITE);       // Задава бял цвят за текст
    } // Край на блока if (m_okButton)
    if (m_cancelButton)                                        // Ако бутонът Cancel съществува
    {                                                          // Начало на блока if (m_cancelButton)
        m_cancelButton->SetBackgroundColour(m_secondaryColor); // Задава вторичния цвят за фон
        m_cancelButton->SetForegroundColour(m_textColor);      // Задава цвета на текста
    } // Край на блока if (m_cancelButton)

    // --- Update Theme Button Appearance --- // --- Актуализиране на външния вид на бутоните за тема ---
    if (m_lightButton && m_darkButton)                          // Ако и двата бутона за тема съществуват
    {                                                           // Начало на блока if (m_lightButton && m_darkButton)
        if (m_selectedThemeInDialog == "light")                 // Ако избраната тема е "light"
        {                                                       // Начало на блока if (light theme)
            m_lightButton->SetBackgroundColour(m_primaryColor); // Active button uses primary // Активният бутон използва основния цвят
            m_lightButton->SetForegroundColour(*wxWHITE);       // Бял текст
            m_darkButton->SetBackgroundColour(m_bgColor);       // Inactive button matches dialog BG // Неактивният бутон съответства на фона на диалога
            m_darkButton->SetForegroundColour(m_textColor);     // Цвят на текста
        } // Край на блока if (light theme)
        else                                                   // В противен случай (тъмна или други теми)
        {                                                      // Dark or other themes // Тъмна или други теми
            m_darkButton->SetBackgroundColour(m_primaryColor); // Active button uses primary // Активният бутон използва основния цвят
            m_darkButton->SetForegroundColour(*wxWHITE);       // Бял текст
            m_lightButton->SetBackgroundColour(m_bgColor);     // Inactive button matches dialog BG // Неактивният бутон съответства на фона на диалога
            m_lightButton->SetForegroundColour(m_textColor);   // Цвят на текста
        } // Край на блока else
        m_lightButton->Refresh(); // Прерисува бутона за светла тема
        m_darkButton->Refresh();  // Прерисува бутона за тъмна тема
    } // Край на блока if (m_lightButton && m_darkButton)

    // --- Refresh Layout and Drawing --- // --- Опресняване на подредбата и рисуването ---
    if (m_mainPanel)           // Ако главният панел съществува
        m_mainPanel->Layout(); // Преизчислява подредбата на главния панел
    Layout();                  // Преизчислява подредбата на диалога
    Refresh();                 // Прерисува диалога
    Update();                  // Ensures immediate redraw // Осигурява незабавно прерисуване
} // Край на тялото на ApplyTheme

// CreateContent remains largely the same, but remove direct color settings // CreateContent остава почти същият, но премахва директните настройки на цветове
void SettingsDialog::CreateContent() // Метод за създаване на съдържанието на диалога
{                                    // Начало на тялото на CreateContent
    // Get initial colors just for reference if needed, but don't apply here // Вземане на началните цветове само за справка, ако е необходимо, но не ги прилагайте тук
    const ThemeColors &initialColors = ThemeConfig::Get().GetThemeColors(m_selectedThemeInDialog); // Взема началните цветове
    // Example: wxColour initialBg = initialColors.background; // Пример: wxColour initialBg = initialColors.background;

    m_mainPanel = new wxPanel(this);                    // Създава главен панел
    wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL); // Създава главен вертикален сайзер

    m_titleText = new wxStaticText(m_mainPanel, wxID_ANY, "Settings"); // Създава заглавен текст
    wxFont titleFont = m_titleText->GetFont();                         // Взема шрифта на заглавния текст
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);                            // Задава удебелен шрифт
    titleFont.SetPointSize(titleFont.GetPointSize() + 3);              // Увеличава размера на шрифта
    m_titleText->SetFont(titleFont);                                   // Задава новия шрифт
    mainSizer->Add(m_titleText, 0, wxALL | wxEXPAND, 20);              // Добавя заглавния текст към главния сайзер с отстояния и разширяване

    wxPanel *themePanel = new wxPanel(m_mainPanel);                                                                 // Създава панел за секцията с теми
    wxBoxSizer *themeSizer = new wxBoxSizer(wxVERTICAL);                                                            // Създава вертикален сайзер за темите
    wxStaticText *themeLabel = new wxStaticText(themePanel, wxID_ANY, "Theme");                                     // Създава етикет "Theme"
    themeSizer->Add(themeLabel, 0, wxLEFT | wxTOP | wxRIGHT, 10);                                                   // Добавя етикета към сайзера с отстояния
    wxBoxSizer *themeButtonSizer = new wxBoxSizer(wxHORIZONTAL);                                                    // Създава хоризонтален сайзер за бутоните за тема
    m_lightButton = new wxButton(themePanel, wxID_ANY, "Light", wxDefaultPosition, wxSize(160, 45), wxBORDER_NONE); // Създава бутон "Light"
    m_darkButton = new wxButton(themePanel, wxID_ANY, "Dark", wxDefaultPosition, wxSize(160, 45), wxBORDER_NONE);   // Създава бутон "Dark"
    wxFont buttonFont = m_lightButton->GetFont();                                                                   // Взема шрифта на бутона
    buttonFont.SetWeight(wxFONTWEIGHT_BOLD);                                                                        // Задава удебелен шрифт
    m_lightButton->SetFont(buttonFont);                                                                             // Задава шрифта на бутона "Light"
    m_darkButton->SetFont(buttonFont);                                                                              // Задава шрифта на бутона "Dark"
    themeButtonSizer->Add(m_lightButton, 1, wxEXPAND | wxRIGHT, 5);                                                 // Добавя бутона "Light" към сайзера с разширяване и отстояние
    themeButtonSizer->Add(m_darkButton, 1, wxEXPAND | wxLEFT, 5);                                                   // Добавя бутона "Dark" към сайзера с разширяване и отстояние
    themeSizer->Add(themeButtonSizer, 0, wxEXPAND | wxALL, 10);                                                     // Добавя сайзера за бутони към сайзера за теми
    themePanel->SetSizer(themeSizer);                                                                               // Задава сайзера за панела с теми
    mainSizer->Add(themePanel, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);                                                 // Добавя панела с теми към главния сайзер

    mainSizer->AddSpacer(15); // Добавя празно пространство

    m_containerSizer = new wxStaticBoxSizer(wxVERTICAL, m_mainPanel, "Container Options");          // Създава сайзер със статична кутия "Container Options"
    wxStaticBox *containerBox = m_containerSizer->GetStaticBox();                                   // Взема статичната кутия
    wxPanel *containerInnerPanel = new wxPanel(containerBox);                                       // Създава вътрешен панел в кутията
    wxBoxSizer *containerInnerSizer = new wxBoxSizer(wxVERTICAL);                                   // Създава вертикален сайзер за вътрешния панел
    wxPanel *dockerPanel = new wxPanel(containerInnerPanel);                                        // Създава панел за Docker
    wxBoxSizer *dockerSizer = new wxBoxSizer(wxHORIZONTAL);                                         // Създава хоризонтален сайзер за Docker
    wxStaticText *dockerText = new wxStaticText(dockerPanel, wxID_ANY, "Use Docker (Recommended)"); // Създава текст "Use Docker (Recommended)"
    m_dockerToggle = new ToggleSwitch(dockerPanel, wxID_ANY, true);                                 // Създава ToggleSwitch за Docker (първоначално включен)
    dockerSizer->Add(dockerText, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);                         // Добавя текста към сайзера
    dockerSizer->Add(m_dockerToggle, 0, wxALIGN_CENTER_VERTICAL);                                   // Добавя превключвателя към сайзера
    dockerPanel->SetSizer(dockerSizer);                                                             // Задава сайзера за панела Docker
    containerInnerSizer->Add(dockerPanel, 0, wxEXPAND | wxBOTTOM, 5);                               // Добавя панела Docker към вътрешния сайзер

#ifdef __WXMSW__                                                                                           // Ако се компилира за Windows
    wxPanel *wslPanel = new wxPanel(containerInnerPanel);                                                  // Създава панел за WSL
    wxBoxSizer *wslSizer = new wxBoxSizer(wxHORIZONTAL);                                                   // Създава хоризонтален сайзер за WSL
    wxStaticText *wslText = new wxStaticText(wslPanel, wxID_ANY, "Use Windows Subsystem for Linux (WSL)"); // Създава текст за WSL
    m_wslToggle = new ToggleSwitch(wslPanel, wxID_ANY, false);                                             // Създава ToggleSwitch за WSL (първоначално изключен)
    wslSizer->Add(wslText, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);                                      // Добавя текста към сайзера
    wslSizer->Add(m_wslToggle, 0, wxALIGN_CENTER_VERTICAL);                                                // Добавя превключвателя към сайзера
    wslPanel->SetSizer(wslSizer);                                                                          // Задава сайзера за панела WSL
    containerInnerSizer->Add(wslPanel, 0, wxEXPAND);                                                       // Добавя панела WSL към вътрешния сайзер
    m_dockerToggle->Bind(EVT_TOGGLE_SWITCH, &SettingsDialog::OnToggleChanged, this);                       // Свързва събитието от Docker превключвателя с OnToggleChanged
    m_wslToggle->Bind(EVT_TOGGLE_SWITCH, &SettingsDialog::OnToggleChanged, this);                          // Свързва събитието от WSL превключвателя с OnToggleChanged
#endif                                                                                                     // Край на условната компилация

    containerInnerPanel->SetSizer(containerInnerSizer);                   // Задава сайзера за вътрешния панел на контейнерите
    m_containerSizer->Add(containerInnerPanel, 1, wxEXPAND | wxALL, 10);  // Добавя вътрешния панел към сайзера на контейнерите
    mainSizer->Add(m_containerSizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 20); // Добавя сайзера на контейнерите към главния сайзер

    mainSizer->AddStretchSpacer(1); // Добавя разтегливо празно пространство, за да избута бутоните надолу

    wxBoxSizer *buttonSizer = new wxBoxSizer(wxHORIZONTAL);                                                               // Създава хоризонтален сайзер за бутоните OK/Cancel
    m_cancelButton = new wxButton(m_mainPanel, wxID_CANCEL, "Cancel", wxDefaultPosition, wxSize(100, 40), wxBORDER_NONE); // Създава бутон "Cancel"
    m_okButton = new wxButton(m_mainPanel, wxID_OK, "OK", wxDefaultPosition, wxSize(100, 40), wxBORDER_NONE);             // Създава бутон "OK"
    m_okButton->SetFont(buttonFont);                                                                                      // Задава шрифта на бутона "OK"
    m_cancelButton->SetFont(buttonFont);                                                                                  // Задава шрифта на бутона "Cancel"
    buttonSizer->AddStretchSpacer(1);                                                                                     // Добавя разтегливо празно пространство вляво от бутоните
    buttonSizer->Add(m_cancelButton, 0, wxRIGHT, 10);                                                                     // Добавя бутона "Cancel" с отстояние вдясно
    buttonSizer->Add(m_okButton, 0);                                                                                      // Добавя бутона "OK"
    mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 20);                                                                 // Добавя сайзера за бутони към главния сайзер

    m_mainPanel->SetSizer(mainSizer);                     // Задава главния сайзер за главния панел
    wxBoxSizer *dialogSizer = new wxBoxSizer(wxVERTICAL); // Създава вертикален сайзер за целия диалог
    dialogSizer->Add(m_mainPanel, 1, wxEXPAND);           // Добавя главния панел към сайзера на диалога, за да заема цялото пространство
    SetSizer(dialogSizer);                                // Задава сайзера за диалога
    Layout();                                             // Преизчислява подредбата на елементите
} // Край на тялото на CreateContent

void SettingsDialog::OnThemeButtonClick(wxCommandEvent &event) // Метод за обработка на натискане на бутон за тема
{                                                              // Начало на тялото на OnThemeButtonClick
    wxObject *obj = event.GetEventObject();                    // Взема обекта, изпратил събитието
    wxString newTheme;                                         // Променлива за новата тема
    if (obj == m_lightButton)                                  // Ако обектът е бутонът за светла тема
    {                                                          // Начало на блока if (lightButton)
        newTheme = "light";                                    // Задава новата тема на "light"
    } // Край на блока if (lightButton)
    else if (obj == m_darkButton) // Ако обектът е бутонът за тъмна тема
    {                             // Начало на блока else if (darkButton)
        newTheme = "dark";        // Задава новата тема на "dark"
    } // Край на блока else if (darkButton)
    else        // В противен случай
    {           // Начало на блока else
        return; // Not a theme button // Не е бутон за тема, излиза
    } // Край на блока else

    // Only apply if the theme actually changed within the dialog's context // Прилагане само ако темата действително се е променила в контекста на диалога
    if (newTheme != m_selectedThemeInDialog) // Ако новата тема е различна от текущо избраната в диалога
    {                                        // Начало на блока if (theme changed)
        ApplyTheme(newTheme);                // Прилага новата тема
    } // Край на блока if (theme changed)
} // Край на тялото на OnThemeButtonClick

void SettingsDialog::OnToggleChanged(wxCommandEvent &event) // Метод за обработка на промяна на ToggleSwitch
{                                                           // Начало на тялото на OnToggleChanged
#ifdef __WXMSW__                                            // Ако се компилира за Windows
    wxObject *obj = event.GetEventObject();                 // Взема обекта, изпратил събитието
    bool value = event.GetInt() != 0;                       // Взема стойността от събитието (true/false)

    if (!m_dockerToggle || !m_wslToggle) // Ако някой от превключвателите не е инициализиран
        return;                          // Safety // Излиза (защита)

    // Mutual Exclusivity Logic: If one is turned ON, turn the other OFF. // Логика за взаимно изключване: Ако единият се включи, другият се изключва.
    if (obj == m_dockerToggle && value) // Ако Docker превключвателят е променен и е включен
    {                                   // Начало на блока if (docker on)
        m_wslToggle->SetValue(false);   // Изключва WSL превключвателя
    } // Край на блока if (docker on)
    else if (obj == m_wslToggle && value) // Ако WSL превключвателят е променен и е включен
    {                                     // Начало на блока else if (wsl on)
        m_dockerToggle->SetValue(false);  // Изключва Docker превключвателя
    } // Край на блока else if (wsl on)
    // Prevent both being turned OFF. // Предотвратяване на изключването и на двата.
    // If the user action *would* result in both being off, revert the action // Ако действието на потребителя *би* довело до изключване и на двата, върнете действието
    // by setting the one they just interacted with back to true. // като зададете този, с който току-що са взаимодействали, обратно на true.
    else if (!value && !m_dockerToggle->GetValue() && !m_wslToggle->GetValue()) // Ако текущото действие изключва превключвател И двата превключвателя са изключени
    {                                                                           // Начало на блока else if (both off)
        // This state should ideally not be reached if the above logic works, // Това състояние идеално не трябва да се достига, ако горната логика работи,
        // but as a fallback, force one back on. // но като резервен вариант, принудително включете единия обратно.
        if (obj == m_dockerToggle)          // Ако промененият превключвател е Docker
            m_dockerToggle->SetValue(true); // Revert docker toggle // Върнете Docker превключвателя на включено
        else if (obj == m_wslToggle)        // Ако промененият превключвател е WSL
            m_wslToggle->SetValue(true);    // Revert wsl toggle // Върнете WSL превключвателя на включено
    } // Край на блока else if (both off)

#endif // Край на условната компилация
} // Край на тялото на OnToggleChanged

// Returns the theme currently selected *in the dialog*, not necessarily the global one yet // Връща темата, текущо избрана *в диалога*, не непременно глобалната все още
wxString SettingsDialog::GetSelectedTheme() const // Метод за получаване на избраната тема в диалога
{                                                 // Начало на тялото на GetSelectedTheme
    return m_selectedThemeInDialog;               // Връща стойността на m_selectedThemeInDialog
} // Край на тялото на GetSelectedTheme

bool SettingsDialog::UseDocker() const                         // Метод за проверка дали се използва Docker
{                                                              // Начало на тялото на UseDocker
    return m_dockerToggle ? m_dockerToggle->GetValue() : true; // Връща стойността на Docker превключвателя (ако съществува), иначе true по подразбиране
} // Край на тялото на UseDocker

bool SettingsDialog::UseWSL() const                       // Метод за проверка дали се използва WSL
{                                                         // Начало на тялото на UseWSL
#ifdef __WXMSW__                                          // Ако се компилира за Windows
    return m_wslToggle ? m_wslToggle->GetValue() : false; // Връща стойността на WSL превключвателя (ако съществува), иначе false по подразбиране
#else                                                     // В противен случай (не е Windows)
    return false; // Връща false
#endif                                                    // Край на условната компилация
} // Край на тялото на UseWSL

void SettingsDialog::OnPaint(wxPaintEvent &event) // Метод за обработка на събитието за прерисуване на диалога
{                                                 // Начало на тялото на OnPaint (Dialog)
    wxAutoBufferedPaintDC dc(this);               // Използва автоматично буфериран DC
    // Use colors based on the dialog's *currently selected* theme state // Използване на цветове въз основа на *текущо избраното* състояние на темата на диалога
    const ThemeColors &colors = ThemeConfig::Get().GetThemeColors(m_selectedThemeInDialog); // Взема цветовете на текущо избраната тема в диалога
    dc.SetBackground(wxBrush(colors.background));                                           // Задава фона с цвета от темата
    dc.Clear();                                                                             // Изчиства областта за рисуване

    // Optional rounded corners // Опционални заоблени ъгли
    wxGraphicsContext *gc = wxGraphicsContext::Create(dc); // Създава графичен контекст
    if (gc)                                                // Ако е създаден успешно
    {                                                      // Начало на блока if (gc dialog)
        gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);         // Включва анти-алиасинг
        wxRect rect = GetClientRect();                     // Взема клиентската област на диалога
        gc->SetBrush(wxBrush(colors.background));          // Задава четка с цвета на фона
        gc->SetPen(*wxTRANSPARENT_PEN);                    // Задава прозрачна писалка
        // gc->DrawRoundedRectangle(rect.x, rect.y, rect.width, rect.height, 10); // Uncomment if desired // Откоментирайте, ако желаете заоблени ъгли
        delete gc; // Освобождава графичния контекст
    } // Край на блока if (gc dialog)
} // Край на тялото на OnPaint (Dialog)

void SettingsDialog::OnOK(wxCommandEvent &event) // Метод за обработка на натискане на бутон "OK"
{                                                // Начало на тялото на OnOK
    // 1. Get the theme selected *in the dialog* when OK was clicked // 1. Вземане на темата, избрана *в диалога*, когато е натиснат OK
    wxString finalSelectedTheme = GetSelectedTheme(); // Взема избраната тема от диалога

    // 2. Update the global ThemeConfig state // 2. Актуализиране на глобалното състояние на ThemeConfig
    ThemeConfig::Get().SetCurrentTheme(finalSelectedTheme); // Задава текущата глобална тема

    // 3. Apply the final theme globally (to parent and potentially other registered windows) // 3. Прилагане на финалната тема глобално (към родителя и потенциално други регистрирани прозорци)
    wxWindow *parent = GetParent(); // Взема родителския прозорец
    if (parent)                     // Ако родителят съществува
    {                               // Начало на блока if (parent)
        // This will trigger ApplyTheme on MainFrame and its children (including TitleBar) // Това ще задейства ApplyTheme на MainFrame и неговите дъщерни елементи (включително TitleBar)
        ThemeConfig::Get().ApplyTheme(parent, finalSelectedTheme); // Прилага темата към родителя
    } // Край на блока if (parent)
    // Optionally trigger an update for all other registered windows if needed // Опционално задействане на актуализация за всички други регистрирани прозорци, ако е необходимо
    // ThemeConfig::Get().UpdateSystemTheme(); // Or a similar method if not tied to system event // Или подобен метод, ако не е свързан със системно събитие

    // 4. Close the dialog // 4. Затваряне на диалога
    EndModal(wxID_OK); // Затваря модалния диалог с резултат wxID_OK
} // Край на тялото на OnOK

void SettingsDialog::OnCancel(wxCommandEvent &event) // Метод за обработка на натискане на бутон "Cancel"
{                                                    // Начало на тялото на OnCancel
    // Discard changes made within the dialog // Отхвърляне на промените, направени в диалога
    EndModal(wxID_CANCEL); // Затваря модалния диалог с резултат wxID_CANCEL
} // Край на тялото на OnCancel
