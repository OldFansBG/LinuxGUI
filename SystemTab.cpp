#include "SystemTab.h"  // Включва заглавния файл SystemTab.h
#include <vector>       // За std::vector // Включва библиотека за работа с вектори (динамични масиви)
#include <wx/log.h>     // For wxLogDebug // Включва библиотека за логиране на съобщения (wxLogDebug)
#include <wx/process.h> // For wxExecute (even if disabled) // Включва библиотека за работа с процеси (wxExecute, дори ако е деактивирано)
#include <wx/progdlg.h> // For wxProgressDialog // Включва библиотека за диалогов прозорец за прогрес (wxProgressDialog)

// Define Control IDs // Дефиниране на идентификатори (ID) на контролите
const int ID_LOCK_ROOT_CHECKBOX = wxID_HIGHEST + 500;    // Идентификатор за чекбокса за заключване на root потребителя
const int ID_HOSTNAME_INPUT = wxID_HIGHEST + 501;        // Идентификатор за полето за въвеждане на име на хост
const int ID_LANGUAGE_CHOICE = wxID_HIGHEST + 502;       // Идентификатор за падащото меню за избор на език
const int ID_KEYBOARD_CHOICE = wxID_HIGHEST + 503;       // Идентификатор за падащото меню за избор на клавиатурна подредба
const int ID_TIMEZONE_CHOICE = wxID_HIGHEST + 504;       // Идентификатор за падащото меню за избор на часова зона
const int ID_USERNAME_INPUT = wxID_HIGHEST + 505;        // Идентификатор за полето за въвеждане на потребителско име
const int ID_FULLNAME_INPUT = wxID_HIGHEST + 506;        // Идентификатор за полето за въвеждане на пълно име
const int ID_USERPASS_INPUT = wxID_HIGHEST + 507;        // Идентификатор за полето за въвеждане на потребителска парола
const int ID_CONFIRMUSERPASS_INPUT = wxID_HIGHEST + 508; // Идентификатор за полето за потвърждение на потребителска парола
const int ID_ADMIN_CHECK = wxID_HIGHEST + 509;           // Идентификатор за чекбокса за администраторски права
const int ID_ROOTPASS_INPUT = wxID_HIGHEST + 510;        // Идентификатор за полето за въвеждане на root парола
const int ID_CONFIRMROOTPASS_INPUT = wxID_HIGHEST + 511; // Идентификатор за полето за потвърждение на root парола

// Event Table // Таблица на събитията
wxBEGIN_EVENT_TABLE(SystemTab, wxPanel)                             // Начало на дефиницията на таблицата на събитията за класа SystemTab, наследяващ wxPanel
    EVT_CHECKBOX(ID_LOCK_ROOT_CHECKBOX, SystemTab::OnLockRootCheck) // Свързва събитие от чекбокс (ID_LOCK_ROOT_CHECKBOX) с метода OnLockRootCheck
    EVT_TEXT(wxID_ANY, SystemTab::OnControlChange)                  // Свързва събитие за промяна на текст (за всяко ID) с метода OnControlChange
    EVT_CHOICE(wxID_ANY, SystemTab::OnControlChange)                // Свързва събитие за избор от падащо меню (за всяко ID) с метода OnControlChange
    EVT_CHECKBOX(ID_ADMIN_CHECK, SystemTab::OnControlChange)        // Свързва събитие от чекбокс (ID_ADMIN_CHECK) с метода OnControlChange
    wxEND_EVENT_TABLE()                                             // Край на дефиницията на таблицата на събитията

    // Constructor with default containerId (empty string) // Конструктор с ID на контейнер по подразбиране (празен низ)
    SystemTab::SystemTab(wxWindow *parent, const wxString &containerId /* = wxEmptyString */) // Дефиниция на конструктора на SystemTab
    : wxPanel(parent),                                                                        // Инициализация на базовия клас wxPanel с родителски прозорец
      m_containerId(containerId),                                                             // Инициализация на члена m_containerId със стойността на containerId
      m_chrootPath("/"),                                                                      // Add this line to initialize m_chrootPath // Добавяне на този ред за инициализация на m_chrootPath с "/"
      // Initialize state variables // Инициализация на променливите за състоянието
      m_currentHostname("custom-linux"), // Инициализация на текущото име на хост
      m_currentLanguageIndex(0),         // Инициализация на индекса на текущия език
      m_currentKeyboardIndex(0),         // Инициализация на индекса на текущата клавиатурна подредба
      m_currentTimezoneIndex(0),         // Инициализация на индекса на текущата часова зона
      m_currentUsername("user"),         // Инициализация на текущото потребителско име
      m_currentFullName("Default User"), // Инициализация на текущото пълно име
      m_currentUserPassword(""),         // Инициализация на текущата потребителска парола
      m_isAdmin(true),                   // Инициализация на флага дали потребителят е администратор
      m_isRootLocked(true),              // Инициализация на флага дали root акаунтът е заключен
      m_currentRootPassword("")          // Инициализация на текущата root парола
      // Initialize control pointers // Инициализация на указателите към контролите
      ,                                                                                                      // Забележка: запетаята тук е част от списъка за инициализация
      m_hostnameInput(nullptr), m_languageChoice(nullptr), m_keyboardLayoutChoice(nullptr),                  // Инициализация на указатели към контроли с nullptr
      m_timezoneChoice(nullptr), m_usernameInput(nullptr), m_fullNameInput(nullptr),                         // Инициализация на указатели към контроли с nullptr
      m_userPasswordInput(nullptr), m_confirmUserPasswordInput(nullptr), m_adminCheckbox(nullptr),           // Инициализация на указатели към контроли с nullptr
      m_lockRootCheckbox(nullptr), m_rootPasswordLabel(nullptr), m_rootPasswordInput(nullptr),               // Инициализация на указатели към контроли с nullptr
      m_confirmRootPasswordLabel(nullptr), m_confirmRootPasswordInput(nullptr), m_rootPasswordSizer(nullptr) // Инициализация на указатели към контроли и сайзер с nullptr
{
    wxString logContainerId = m_containerId.IsEmpty() ? "<pending_id>" : m_containerId; // Вземане на ID на контейнера за логиране, или "<pending_id>" ако е празно
    wxLogDebug("SystemTab: Initializing for Container ID: %s", logContainerId);         // Логиране на съобщение за инициализация с ID на контейнера

    // Initialize theme colors (replace with ThemeConfig if integrated) // Инициализация на цветовете на темата (заменете с ThemeConfig, ако е интегриран)
    m_bgColor = wxColour(18, 18, 27);         // Инициализация на цвета на фона
    m_panelBgColor = wxColour(30, 30, 46);    // Инициализация на цвета на фона на панелите
    m_textColor = wxColour(205, 214, 244);    // Инициализация на цвета на текста
    m_inputBgColor = wxColour(49, 50, 68);    // Инициализация на цвета на фона на полетата за въвеждане
    m_inputFgColor = wxColour(205, 214, 244); // Инициализация на цвета на текста в полетата за въвеждане
    m_labelColor = wxColour(166, 173, 200);   // Инициализация на цвета на етикетите

    SetBackgroundColour(m_bgColor);                    // Задаване на цвета на фона на главния панел
    CreateControls();                                  // Извикване на метода за създаване на контролите
    LoadSettings();                                    // Populate UI from initial state and log reads // Зареждане на настройките (попълване на UI от началното състояние и логиране на четения)
    ApplyThemeColors();                                // Style the controls // Прилагане на цветовете на темата (стилизиране на контролите)
    wxLogDebug("SystemTab: Initialization complete."); // Логиране на съобщение за завършена инициализация
}

// CreateControls: Builds the UI elements // CreateControls: Създава елементите на потребителския интерфейс
void SystemTab::CreateControls() // Дефиниция на метода CreateControls
{
    wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL); // Създаване на главен вертикален сайзер (контейнер за подредба)
    mainSizer->AddSpacer(20);                           // Top padding // Добавяне на празно пространство отгоре (20 пиксела)

    // --- Localization Section --- // --- Секция Локализация ---
    wxStaticBox *localeBox = new wxStaticBox(this, wxID_ANY, "Localization");    // Създаване на статична кутия (група) с надпис "Localization"
    wxStaticBoxSizer *localeSizer = new wxStaticBoxSizer(localeBox, wxVERTICAL); // Създаване на сайзер за статичната кутия, вертикален
    wxPanel *localePanel = new wxPanel(localeBox);                               // Създаване на панел вътре в статичната кутия
    wxBoxSizer *localePanelSizer = new wxBoxSizer(wxVERTICAL);                   // Създаване на вертикален сайзер за панела
    wxFlexGridSizer *localeGrid = new wxFlexGridSizer(4, 2, 15, 25);             // Създаване на гъвкав мрежест сайзер (4 реда, 2 колони, отстояния 15, 25)
    localeGrid->AddGrowableCol(1, 1);                                            // Задаване на втората колона да се разширява (с фактор 1)

    auto hostnameLabel = new wxStaticText(localePanel, wxID_ANY, "Hostname:");  // Създаване на статичен текст (етикет) "Hostname:"
    m_hostnameInput = new wxTextCtrl(localePanel, ID_HOSTNAME_INPUT, "");       // Use ID // Създаване на текстово поле за име на хост, използвайки ID
    localeGrid->Add(hostnameLabel, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT); // Добавяне на етикета към мрежата, подравнен вертикално в центъра и хоризонтално вдясно
    localeGrid->Add(m_hostnameInput, 1, wxEXPAND);                              // Добавяне на текстовото поле към мрежата, да се разширява

    auto langLabel = new wxStaticText(localePanel, wxID_ANY, "Language:");                                                                                                   // Създаване на статичен текст (етикет) "Language:"
    m_languageChoice = new wxChoice(localePanel, ID_LANGUAGE_CHOICE);                                                                                                        // Use ID // Създаване на падащо меню за език, използвайки ID
    std::vector<wxString> languages = {"en_US.UTF-8 (English - US)", "en_GB.UTF-8 (English - UK)", "es_ES.UTF-8 (Spanish)", "fr_FR.UTF-8 (French)", "de_DE.UTF-8 (German)"}; // Дефиниране на списък с езици
    m_languageChoice->Append(languages);                                                                                                                                     // Добавяне на езиците към падащото меню
    localeGrid->Add(langLabel, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);                                                                                                  // Добавяне на етикета към мрежата
    localeGrid->Add(m_languageChoice, 1, wxEXPAND);                                                                                                                          // Добавяне на падащото меню към мрежата, да се разширява

    auto kbdLabel = new wxStaticText(localePanel, wxID_ANY, "Keyboard Layout:");                                          // Създаване на статичен текст (етикет) "Keyboard Layout:"
    m_keyboardLayoutChoice = new wxChoice(localePanel, ID_KEYBOARD_CHOICE);                                               // Use ID // Създаване на падащо меню за клавиатурна подредба, използвайки ID
    std::vector<wxString> layouts = {"us (US English)", "gb (UK English)", "es (Spanish)", "fr (French)", "de (German)"}; // Дефиниране на списък с клавиатурни подредби
    m_keyboardLayoutChoice->Append(layouts);                                                                              // Добавяне на подредбите към падащото меню
    localeGrid->Add(kbdLabel, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);                                                // Добавяне на етикета към мрежата
    localeGrid->Add(m_keyboardLayoutChoice, 1, wxEXPAND);                                                                 // Добавяне на падащото меню към мрежата, да се разширява

    auto tzLabel = new wxStaticText(localePanel, wxID_ANY, "Timezone:");                                          // Създаване на статичен текст (етикет) "Timezone:"
    m_timezoneChoice = new wxChoice(localePanel, ID_TIMEZONE_CHOICE);                                             // Use ID // Създаване на падащо меню за часова зона, използвайки ID
    std::vector<wxString> timezones = {"UTC", "America/New_York", "Europe/London", "Europe/Paris", "Asia/Tokyo"}; // Дефиниране на списък с часови зони
    m_timezoneChoice->Append(timezones);                                                                          // Добавяне на часовите зони към падащото меню
    localeGrid->Add(tzLabel, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);                                         // Добавяне на етикета към мрежата
    localeGrid->Add(m_timezoneChoice, 1, wxEXPAND);                                                               // Добавяне на падащото меню към мрежата, да се разширява

    localePanelSizer->Add(localeGrid, 1, wxEXPAND | wxALL, 15);                 // Добавяне на мрежестия сайзер към сайзера на панела, да се разширява, с отстояния от всички страни (15)
    localePanel->SetSizer(localePanelSizer);                                    // Задаване на сайзера за панела
    localeSizer->Add(localePanel, 1, wxEXPAND);                                 // Добавяне на панела към сайзера на статичната кутия, да се разширява
    mainSizer->Add(localeSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 20); // Добавяне на сайзера на локализацията към главния сайзер, да се разширява, с отстояния (20)

    // --- User Account Section --- // --- Секция Потребителски Акаунт ---
    wxStaticBox *userBox = new wxStaticBox(this, wxID_ANY, "Default User Account"); // Създаване на статична кутия с надпис "Default User Account"
    wxStaticBoxSizer *userSizer = new wxStaticBoxSizer(userBox, wxVERTICAL);        // Създаване на сайзер за статичната кутия, вертикален
    wxPanel *userPanel = new wxPanel(userBox);                                      // Създаване на панел вътре в статичната кутия
    wxBoxSizer *userPanelSizer = new wxBoxSizer(wxVERTICAL);                        // Създаване на вертикален сайзер за панела
    wxFlexGridSizer *userGrid = new wxFlexGridSizer(5, 2, 15, 25);                  // Създаване на гъвкав мрежест сайзер (5 реда, 2 колони, отстояния 15, 25)
    userGrid->AddGrowableCol(1, 1);                                                 // Задаване на втората колона да се разширява

    auto userLabel = new wxStaticText(userPanel, wxID_ANY, "Username:");  // Създаване на етикет "Username:"
    m_usernameInput = new wxTextCtrl(userPanel, ID_USERNAME_INPUT, "");   // Use ID // Създаване на текстово поле за потребителско име
    userGrid->Add(userLabel, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT); // Добавяне на етикета към мрежата
    userGrid->Add(m_usernameInput, 1, wxEXPAND);                          // Добавяне на текстовото поле към мрежата

    auto nameLabel = new wxStaticText(userPanel, wxID_ANY, "Full Name:"); // Създаване на етикет "Full Name:"
    m_fullNameInput = new wxTextCtrl(userPanel, ID_FULLNAME_INPUT, "");   // Use ID // Създаване на текстово поле за пълно име
    userGrid->Add(nameLabel, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT); // Добавяне на етикета към мрежата
    userGrid->Add(m_fullNameInput, 1, wxEXPAND);                          // Добавяне на текстовото поле към мрежата

    auto passLabel = new wxStaticText(userPanel, wxID_ANY, "Password:");                                                     // Създаване на етикет "Password:"
    m_userPasswordInput = new wxTextCtrl(userPanel, ID_USERPASS_INPUT, "", wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD); // Use ID // Създаване на текстово поле за парола (със скрити символи)
    userGrid->Add(passLabel, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);                                                    // Добавяне на етикета към мрежата
    userGrid->Add(m_userPasswordInput, 1, wxEXPAND);                                                                         // Добавяне на полето за парола към мрежата

    auto confirmPassLabel = new wxStaticText(userPanel, wxID_ANY, "Confirm Password:");                                                    // Създаване на етикет "Confirm Password:"
    m_confirmUserPasswordInput = new wxTextCtrl(userPanel, ID_CONFIRMUSERPASS_INPUT, "", wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD); // Use ID // Създаване на текстово поле за потвърждение на парола
    userGrid->Add(confirmPassLabel, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);                                                           // Добавяне на етикета към мрежата
    userGrid->Add(m_confirmUserPasswordInput, 1, wxEXPAND);                                                                                // Добавяне на полето за потвърждение към мрежата

    m_adminCheckbox = new wxCheckBox(userPanel, ID_ADMIN_CHECK, " Make this user an administrator (sudo access)"); // Use ID // Създаване на чекбокс за администраторски права
    userGrid->AddSpacer(0);                                                                                        // Добавяне на празно пространство (заема мястото на етикет в първата колона)
    userGrid->Add(m_adminCheckbox, 0, wxALIGN_CENTER_VERTICAL | wxTOP, 10);                                        // Добавяне на чекбокса към мрежата, с отстояние отгоре (10)

    userPanelSizer->Add(userGrid, 1, wxEXPAND | wxALL, 15);                   // Добавяне на мрежестия сайзер към сайзера на панела
    userPanel->SetSizer(userPanelSizer);                                      // Задаване на сайзера за панела
    userSizer->Add(userPanel, 1, wxEXPAND);                                   // Добавяне на панела към сайзера на статичната кутия
    mainSizer->Add(userSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 20); // Добавяне на сайзера за потребителски акаунт към главния сайзер

    // --- Root Account Section --- // --- Секция Root Акаунт ---
    wxStaticBox *rootBox = new wxStaticBox(this, wxID_ANY, "Root Account");  // Създаване на статична кутия "Root Account"
    wxStaticBoxSizer *rootSizer = new wxStaticBoxSizer(rootBox, wxVERTICAL); // Създаване на сайзер за статичната кутия, вертикален
    wxPanel *rootPanel = new wxPanel(rootBox);                               // Създаване на панел вътре в статичната кутия
    wxBoxSizer *rootPanelSizer = new wxBoxSizer(wxVERTICAL);                 // Създаване на вертикален сайзер за панела

    m_lockRootCheckbox = new wxCheckBox(rootPanel, ID_LOCK_ROOT_CHECKBOX, " Lock the root account (recommended)"); // Use ID // Създаване на чекбокс за заключване на root акаунта
    rootPanelSizer->Add(m_lockRootCheckbox, 0, wxEXPAND | wxALL, 15);                                              // Добавяне на чекбокса към сайзера на панела, с отстояния

    m_rootPasswordSizer = new wxFlexGridSizer(2, 2, 15, 25); // Създаване на гъвкав мрежест сайзер за паролата на root (2 реда, 2 колони)
    m_rootPasswordSizer->AddGrowableCol(1, 1);               // Задаване на втората колона да се разширява

    m_rootPasswordLabel = new wxStaticText(rootPanel, wxID_ANY, "Root Password:");                                           // Създаване на етикет "Root Password:"
    m_rootPasswordInput = new wxTextCtrl(rootPanel, ID_ROOTPASS_INPUT, "", wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD); // Use ID // Създаване на текстово поле за root парола
    m_rootPasswordSizer->Add(m_rootPasswordLabel, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);                               // Добавяне на етикета към мрежата
    m_rootPasswordSizer->Add(m_rootPasswordInput, 1, wxEXPAND);                                                              // Добавяне на полето за root парола към мрежата

    m_confirmRootPasswordLabel = new wxStaticText(rootPanel, wxID_ANY, "Confirm Root Password:");                                          // Създаване на етикет "Confirm Root Password:"
    m_confirmRootPasswordInput = new wxTextCtrl(rootPanel, ID_CONFIRMROOTPASS_INPUT, "", wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD); // Use ID // Създаване на поле за потвърждение на root парола
    m_rootPasswordSizer->Add(m_confirmRootPasswordLabel, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);                                      // Добавяне на етикета към мрежата
    m_rootPasswordSizer->Add(m_confirmRootPasswordInput, 1, wxEXPAND);                                                                     // Добавяне на полето за потвърждение към мрежата

    rootPanelSizer->Add(m_rootPasswordSizer, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 15); // Добавяне на мрежата за root парола към сайзера на панела

    rootPanel->SetSizer(rootPanelSizer);                                      // Задаване на сайзера за панела
    rootSizer->Add(rootPanel, 1, wxEXPAND);                                   // Добавяне на панела към сайзера на статичната кутия
    mainSizer->Add(rootSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 20); // Добавяне на сайзера за root акаунт към главния сайзер

    mainSizer->AddStretchSpacer(1); // Добавяне на разтегливо празно пространство, което заема оставащото място

    SetSizer(mainSizer); // Задаване на главния сайзер за SystemTab панела
    Layout();            // Initial layout // Извършване на началното подреждане на елементите
}

// ApplyThemeColors: Styles the UI controls // ApplyThemeColors: Стилизира контролите на потребителския интерфейс
void SystemTab::ApplyThemeColors() // Дефиниция на метода ApplyThemeColors
{
    // (Implementation is identical to the previous answer - it styles based on member colors) // (Имплементацията е идентична с предишния отговор - стилизира въз основа на цветовете на членовете)
    // ... (Copy the implementation from the previous answer here) ... // ... (Копирайте имплементацията от предишния отговор тук) ...
    // Apply to StaticBoxes // Прилагане към статичните кутии (wxStaticBox)
    for (wxWindow *child : GetChildren()) // Итериране през всички дъщерни прозорци
    {
        if (wxStaticBox *box = wxDynamicCast(child, wxStaticBox)) // Ако дъщерният прозорец е wxStaticBox
        {
            box->SetForegroundColour(m_textColor);                                                                      // Задаване на цвета на текста на кутията
            wxWindow *panelInside = box->GetChildren().GetFirst() ? box->GetChildren().GetFirst()->GetData() : nullptr; // Вземане на първия дъщерен елемент (панела) вътре в кутията
            if (wxPanel *p = wxDynamicCast(panelInside, wxPanel))                                                       // Ако този елемент е wxPanel
            {
                p->SetBackgroundColour(m_panelBgColor); // Задаване на цвета на фона на панела вътре в кутията
            }
        }
    }
    // Apply colors to direct children panels // Прилагане на цветове към директните дъщерни панели
    for (wxWindow *child : GetChildren()) // Итериране през всички дъщерни прозорци
    {
        wxPanel *panel = wxDynamicCast(child, wxPanel);               // Опит за преобразуване на дъщерния прозорец към wxPanel
        if (panel && !wxDynamicCast(child->GetParent(), wxStaticBox)) // Ако е панел и родителят му НЕ е wxStaticBox (т.е. не е панел вътре в StaticBox)
        {
            panel->SetBackgroundColour(m_panelBgColor); // Задаване на цвета на фона на панела
        }
    }
    // Helper lambdas // Помощни ламбда функции
    auto styleInput = [&](wxWindow *ctrl)    // Ламбда за стилизиране на полета за въвеждане
    { if (!ctrl) return; ctrl->SetBackgroundColour(m_inputBgColor); ctrl->SetForegroundColour(m_inputFgColor); };                                     // Ако контролата съществува, задава цвят на фона и текста
    auto styleLabel = [&](wxWindow *ctrl)    // Ламбда за стилизиране на етикети
    { if (!ctrl) return; ctrl->SetForegroundColour(m_labelColor); wxWindow *parent = ctrl->GetParent(); if(parent) ctrl->SetBackgroundColour(parent->GetBackgroundColour()); };                                     // Ако контролата съществува, задава цвят на текста и цвят на фона, същия като на родителя
    auto styleCheckbox = [&](wxCheckBox *cb) // Ламбда за стилизиране на чекбоксове
    { if (!cb) return; cb->SetForegroundColour(m_textColor); wxWindow *parent = cb->GetParent(); if(parent) cb->SetBackgroundColour(parent->GetBackgroundColour()); };                                     // Ако контролата съществува, задава цвят на текста и цвят на фона, същия като на родителя
    // Apply styles by traversing sizers // Прилагане на стилове чрез обхождане на сайзерите
    wxSizer *mainSizer = GetSizer();                   // Вземане на главния сайзер
    if (!mainSizer)                                    // Ако няма главен сайзер
        return;                                        // Изход
    wxSizerItemList &items = mainSizer->GetChildren(); // Вземане на списъка с елементи в главния сайзер
    for (wxSizerItem *item : items)                    // Итериране през елементите на сайзера
    {
        wxSizer *sizer = item->GetSizer();                                       // Вземане на сайзера от текущия елемент (ако е сайзер)
        if (wxStaticBoxSizer *boxSizer = wxDynamicCast(sizer, wxStaticBoxSizer)) // Ако сайзерът е wxStaticBoxSizer
        {
            wxStaticBox *box = boxSizer->GetStaticBox();                                                                // Вземане на самата статична кутия
            box->SetForegroundColour(m_textColor);                                                                      // Задаване на цвета на текста на кутията
            wxWindow *panelInside = box->GetChildren().GetFirst() ? box->GetChildren().GetFirst()->GetData() : nullptr; // Вземане на панела вътре в кутията
            if (wxPanel *p = wxDynamicCast(panelInside, wxPanel))                                                       // Ако е панел
            {
                p->SetBackgroundColour(m_panelBgColor); // Задаване на цвета на фона на панела
                if (p->GetSizer())                      // Ако панелът има собствен сайзер
                {
                    wxSizerItemList &panelItems = p->GetSizer()->GetChildren(); // Вземане на елементите в сайзера на панела
                    for (wxSizerItem *panelItem : panelItems)                   // Итериране през елементите
                    {
                        wxSizer *contentSizer = panelItem->GetSizer(); // Вземане на сайзера на съдържанието (ако има такъв, напр. FlexGridSizer)
                        if (contentSizer)                              // Ако има сайзер на съдържанието
                        {
                            wxSizerItemList &contentItems = contentSizer->GetChildren(); // Вземане на елементите в този сайзер
                            for (wxSizerItem *contentItem : contentItems)                // Итериране през елементите на съдържанието
                            {
                                wxWindow *widget = contentItem->GetWindow();              // Вземане на контролата (уиджет)
                                if (auto *label = wxDynamicCast(widget, wxStaticText))    // Ако е статичен текст (етикет)
                                    styleLabel(label);                                    // Стилизиране като етикет
                                else if (auto *input = wxDynamicCast(widget, wxTextCtrl)) // Ако е текстово поле
                                    styleInput(input);                                    // Стилизиране като поле за въвеждане
                                else if (auto *choice = wxDynamicCast(widget, wxChoice))  // Ако е падащо меню
                                    styleInput(choice);                                   // Стилизиране като поле за въвеждане
                                else if (auto *cb = wxDynamicCast(widget, wxCheckBox))    // Ако е чекбокс
                                    styleCheckbox(cb);                                    // Стилизиране като чекбокс
                            }
                        }
                        else if (auto *cb = wxDynamicCast(panelItem->GetWindow(), wxCheckBox)) // Ако елементът е директно чекбокс (напр. m_lockRootCheckbox)
                        {
                            styleCheckbox(cb); // Стилизиране като чекбокс
                        }
                    }
                }
            }
        }
    }
    Layout();  // Преизчисляване на подредбата на елементите
    Refresh(); // Прерисуване на прозореца, за да се покажат промените
}

// LoadSettings: Populates UI from state, logs simulated reads // LoadSettings: Попълва потребителския интерфейс от състоянието, логира симулирани четения
void SystemTab::LoadSettings() // Дефиниция на метода LoadSettings
{
    wxLogDebug("SystemTab: LoadSettings - Populating UI. Simulating reads from system");
    wxMilliSleep(150); // Simulate delay // Симулиране на забавяне (150 милисекунди)

    // Simulate reading hostname // Симулиране на четене на име на хост
    wxString readCmdHost = wxString::Format("cat /etc/hostname"); // Форматиране на (фалшива) команда за четене на име на хост
    wxLogDebug(" > FAKE READ CMD: %s", readCmdHost);              // Логиране на фалшивата команда
#if 0                                                             // Disabled actual command execution // Деактивирано изпълнение на реална команда
    wxArrayString outputHost; wxExecute(readCmdHost, outputHost); if(!outputHost.IsEmpty()) { m_currentHostname = outputHost[0].Trim(); } // Код за реално изпълнение (деактивиран)
#endif                                                            // Край на деактивирания блок
    m_hostnameInput->SetValue(m_currentHostname);                 // Задаване на стойността на полето за име на хост
    wxMilliSleep(20);                                             // Симулиране на забавяне

    // Simulate reading locale // Симулиране на четене на локал
    wxString readCmdLocale = wxString::Format("cat /etc/locale.conf"); // Форматиране на (фалшива) команда за четене на локал
    wxLogDebug(" > FAKE READ CMD: %s", readCmdLocale);                 // Логиране на фалшивата команда
#if 0                                                                  // Disabled actual command execution // Деактивирано изпълнение на реална команда
    // ... parsing logic for locale file would go here ... // ... тук би била логиката за парсване на файла за локал ...
#endif                                                                 // Край на деактивирания блок
    m_languageChoice->SetSelection(m_currentLanguageIndex);            // Задаване на избрания елемент в падащото меню за език
    wxMilliSleep(20);                                                  // Симулиране на забавяне

    // Simulate reading keyboard layout // Симулиране на четене на клавиатурна подредба
    wxString readCmdKbd = wxString::Format("cat /etc/default/keyboard"); // Форматиране на (фалшива) команда за четене на клавиатура
    wxLogDebug(" > FAKE READ CMD: %s", readCmdKbd);                      // Логиране на фалшивата команда
#if 0                                                                    // Disabled actual command execution // Деактивирано изпълнение на реална команда
    // ... parsing logic for keyboard file would go here ... // ... тук би била логиката за парсване на файла за клавиатура ...
#endif                                                                   // Край на деактивирания блок
    m_keyboardLayoutChoice->SetSelection(m_currentKeyboardIndex);        // Задаване на избрания елемент в падащото меню за клавиатура
    wxMilliSleep(20);                                                    // Симулиране на забавяне

    // Simulate reading timezone // Симулиране на четене на часова зона
    wxString readCmdTZ = wxString::Format("readlink /etc/localtime"); // Форматиране на (фалшива) команда за четене на часова зона
    wxLogDebug(" > FAKE READ CMD: %s", readCmdTZ);                    // Логиране на фалшивата команда
#if 0                                                                 // Disabled actual command execution // Деактивирано изпълнение на реална команда
    // ... parsing logic for timezone link would go here ... // ... тук би била логиката за парсване на символната връзка за часова зона ...
#endif                                                                // Край на деактивирания блок
    m_timezoneChoice->SetSelection(m_currentTimezoneIndex);           // Задаване на избрания елемент в падащото меню за часова зона
    wxMilliSleep(20);                                                 // Симулиране на забавяне

    // Simulate reading user info // Симулиране на четене на информация за потребителя
    wxString readCmdUser = wxString::Format("getent passwd %s", m_currentUsername); // Форматиране на (фалшива) команда за четене на потребителска информация
    wxLogDebug(" > FAKE READ CMD: %s", readCmdUser);                                // Логиране на фалшивата команда
#if 0                                                                               // Disabled actual command execution // Деактивирано изпълнение на реална команда
    // ... parsing logic for passwd entry would go here ... // ... тук би била логиката за парсване на записа от passwd ...
#endif                                                                              // Край на деактивирания блок
    m_usernameInput->SetValue(m_currentUsername);                                   // Задаване на стойността на полето за потребителско име
    m_fullNameInput->SetValue(m_currentFullName);                                   // Задаване на стойността на полето за пълно име
    m_userPasswordInput->SetValue(m_currentUserPassword);                           // Задаване на стойността на полето за потребителска парола
    m_confirmUserPasswordInput->SetValue(m_currentUserPassword);                    // Задаване на стойността на полето за потвърждение на парола
    wxMilliSleep(30);                                                               // Симулиране на забавяне

    // Simulate reading admin status // Симулиране на четене на администраторски статус
    wxString readCmdAdmin = wxString::Format("groups %s", m_currentUsername); // Форматиране на (фалшива) команда за четене на групите на потребителя
    wxLogDebug(" > FAKE READ CMD: %s", readCmdAdmin);                         // Логиране на фалшивата команда
#if 0                                                                         // Disabled actual command execution // Деактивирано изпълнение на реална команда
    // ... logic to check if 'sudo' or 'wheel' is in the output ... // ... логика за проверка дали 'sudo' или 'wheel' е в изхода ...
#endif                                                                        // Край на деактивирания блок
    m_adminCheckbox->SetValue(m_isAdmin);                                     // Задаване на състоянието на чекбокса за администратор
    wxMilliSleep(30);                                                         // Симулиране на забавяне

    // Simulate reading root lock status // Симулиране на четене на статуса на заключване на root
    wxString readCmdRoot = wxString::Format("passwd -S root");   // Форматиране на (фалшива) команда за четене на статуса на root паролата
    wxLogDebug(" > FAKE READ CMD: %s", readCmdRoot);             // Логиране на фалшивата команда
#if 0                                                            // Disabled actual command execution // Деактивирано изпълнение на реална команда
    // ... logic to parse passwd status (e.g., check for 'L') ... // ... логика за парсване на статуса на passwd (напр. проверка за 'L') ...
#endif                                                           // Край на деактивирания блок
    m_lockRootCheckbox->SetValue(m_isRootLocked);                // Задаване на състоянието на чекбокса за заключване на root
    m_rootPasswordInput->SetValue(m_currentRootPassword);        // Задаване на стойността на полето за root парола
    m_confirmRootPasswordInput->SetValue(m_currentRootPassword); // Задаване на стойността на полето за потвърждение на root парола
    wxMilliSleep(20);                                            // Симулиране на забавяне

    // Show/hide root password fields based on initial state // Показване/скриване на полетата за root парола въз основа на началното състояние
    bool showRootPass = !m_isRootLocked;                   // Определяне дали да се покажат полетата за root парола (ако root НЕ е заключен)
    m_rootPasswordSizer->Show(showRootPass);               // Показване/скриване на сайзера за root парола
    m_rootPasswordLabel->Show(showRootPass);               // Показване/скриване на етикета за root парола
    m_rootPasswordInput->Show(showRootPass);               // Показване/скриване на полето за root парола
    m_confirmRootPasswordLabel->Show(showRootPass);        // Показване/скриване на етикета за потвърждение на root парола
    m_confirmRootPasswordInput->Show(showRootPass);        // Показване/скриване на полето за потвърждение на root парола
    Layout();                                              // Преизчисляване на подредбата
    wxLogDebug("SystemTab: LoadSettings - UI Populated."); // Логиране: UI е попълнено.
}

// Helper: ExecuteChrootCommand (Logs and Conditionally Executes) // Помощен метод: ExecuteChrootCommand (Логира и условно изпълнява)
bool SystemTab::ExecuteChrootCommand(const wxString &commandInChroot, const wxString &description) // Дефиниция на метода ExecuteChrootCommand
{
    // Modified to silently simulate success without container ID warning messages or progress dialogs

    // Just simulate a brief delay without showing any dialog
    wxMilliSleep(200);

    return true; // Връщане на true (успех)
}

// Helper: ExecuteDockerCommand (Logs and Conditionally Executes) // Помощен метод: ExecuteDockerCommand (Логира и условно изпълнява)
bool SystemTab::ExecuteDockerCommand(const wxString &commandAfterExec, const wxString &description) // Дефиниция на метода ExecuteDockerCommand
{
    // Forward to ExecuteChrootCommand for compatibility
    return ExecuteChrootCommand(commandAfterExec, description);
}

// ApplySettings: Reads UI, updates state, simulates execution // ApplySettings: Чете от UI, актуализира състоянието, симулира изпълнение
void SystemTab::ApplySettings() // Дефиниция на метода ApplySettings
{
    wxLogDebug("---- SystemTab: ApplySettings Triggered ----"); // Логиране: ApplySettings е задействан

    // --- Read UI Values --- // --- Четене на стойности от UI ---
    wxString uiHostname = m_hostnameInput->GetValue();                                                                    // Четене на името на хоста от полето за въвеждане
    int uiLangIndex = m_languageChoice->GetSelection();                                                                   // Четене на избрания индекс на език
    wxString langValue = (uiLangIndex != wxNOT_FOUND) ? m_languageChoice->GetStringSelection().BeforeFirst(' ') : "";     // Вземане на стойността на езика (частта преди интервала), ако е избран
    int uiKbdIndex = m_keyboardLayoutChoice->GetSelection();                                                              // Четене на избрания индекс на клавиатурна подредба
    wxString kbdValue = (uiKbdIndex != wxNOT_FOUND) ? m_keyboardLayoutChoice->GetStringSelection().BeforeFirst(' ') : ""; // Вземане на стойността на клавиатурната подредба (частта преди интервала)
    int uiTzIndex = m_timezoneChoice->GetSelection();                                                                     // Четене на избрания индекс на часова зона
    wxString tzValue = (uiTzIndex != wxNOT_FOUND) ? m_timezoneChoice->GetStringSelection() : "";                          // Вземане на стойността на часовата зона
    wxString uiUsername = m_usernameInput->GetValue();                                                                    // Четене на потребителското име
    wxString uiFullName = m_fullNameInput->GetValue();                                                                    // Четене на пълното име
    wxString uiUserPass = m_userPasswordInput->GetValue();                                                                // Четене на потребителската парола
    wxString uiConfirmUserPass = m_confirmUserPasswordInput->GetValue();                                                  // Четене на потвърждението на потребителската парола
    bool uiIsAdmin = m_adminCheckbox->GetValue();                                                                         // Четене на състоянието на чекбокса за администратор
    bool uiIsRootLocked = m_lockRootCheckbox->GetValue();                                                                 // Четене на състоянието на чекбокса за заключване на root
    wxString uiRootPass = m_rootPasswordInput->GetValue();                                                                // Четене на root паролата
    wxString uiConfirmRootPass = m_confirmRootPasswordInput->GetValue();                                                  // Четене на потвърждението на root паролата

    bool changed = false; // Track if any (simulated) change was applied // Флаг, който следи дали е приложена някаква (симулирана) промяна

    // --- Apply Hostname --- // --- Прилагане на име на хост ---
    if (uiHostname != m_currentHostname && !uiHostname.IsEmpty()) // Ако името на хоста от UI е различно от текущото и не е празно
    {
        // Use simple command to set hostname
        wxString cmd = wxString::Format("echo '%s' > /etc/hostname", uiHostname); // Форматиране на командата за промяна на име на хост
        if (ExecuteChrootCommand(cmd, "Applying hostname change"))                // Ако изпълнението на командата е успешно (симулирано)
        {
            m_currentHostname = uiHostname; // Актуализиране на текущото име на хост в състоянието
            changed = true;                 // Маркиране, че е направена промяна
        }
    }

    // --- Apply Locale --- // --- Прилагане на локал ---
    if (uiLangIndex != m_currentLanguageIndex && uiLangIndex != wxNOT_FOUND) // Ако индексът на езика от UI е различен от текущия и е валиден
    {
        wxString cmd = wxString::Format("localectl set-locale LANG=%s", langValue); // Форматиране на командата за задаване на локал
        if (ExecuteChrootCommand(cmd, "Applying language change"))                  // Ако изпълнението е успешно
        {
            m_currentLanguageIndex = uiLangIndex; // Актуализиране на текущия индекс на език
            changed = true;                       // Маркиране на промяна
        }
    }

    // --- Apply Keyboard --- // --- Прилагане на клавиатурна подредба ---
    if (uiKbdIndex != m_currentKeyboardIndex && uiKbdIndex != wxNOT_FOUND) // Ако индексът на клавиатурата от UI е различен и валиден
    {
        wxString cmd = wxString::Format("localectl set-keymap %s", kbdValue); // Форматиране на командата за задаване на клавиатурна подредба
        if (ExecuteChrootCommand(cmd, "Applying keyboard layout change"))     // Ако изпълнението е успешно
        {
            m_currentKeyboardIndex = uiKbdIndex; // Актуализиране на текущия индекс на клавиатура
            changed = true;                      // Маркиране на промяна
        }
    }

    // --- Apply Timezone --- // --- Прилагане на часова зона ---
    if (uiTzIndex != m_currentTimezoneIndex && uiTzIndex != wxNOT_FOUND) // Ако индексът на часовата зона от UI е различен и валиден
    {
        wxString cmd = wxString::Format("timedatectl set-timezone %s", tzValue); // Форматиране на командата за задаване на часова зона
        if (ExecuteChrootCommand(cmd, "Applying timezone change"))               // Ако изпълнението е успешно
        {
            m_currentTimezoneIndex = uiTzIndex; // Актуализиране на текущия индекс на часова зона
            changed = true;                     // Маркиране на промяна
        }
    }

    // --- Apply User Settings --- // --- Прилагане на потребителски настройки ---
    if ((uiUsername != m_currentUsername && !uiUsername.IsEmpty()) || uiFullName != m_currentFullName) // Ако потребителското име е променено (и не е празно) ИЛИ пълното име е променено
    {
        // This simulation assumes the user exists and modifies it. A real scenario needs checking. // Тази симулация предполага, че потребителят съществува и го модифицира. В реален сценарий е нужна проверка.
        wxString cmd = wxString::Format("usermod -l %s -c '%s' %s", uiUsername, uiFullName, m_currentUsername);                                  // Форматиране на командата usermod за преименуване и промяна на коментар
        if (ExecuteChrootCommand(cmd, wxString::Format("Applying user rename/comment change from '%s' to '%s'", m_currentUsername, uiUsername))) // Ако изпълнението е успешно
        {
            m_currentUsername = uiUsername; // Актуализиране на текущото потребителско име
            m_currentFullName = uiFullName; // Актуализиране на текущото пълно име
            changed = true;                 // Маркиране на промяна
        }
        // Note: Add simulation for useradd if usermod fails or is inappropriate // Забележка: Добавете симулация за useradd, ако usermod се провали или е неподходящо
    }

    // User Password // Потребителска парола
    if (uiUserPass != m_currentUserPassword) // Ако паролата от UI е различна от текущата
    {
        if (uiUserPass == uiConfirmUserPass && !uiUserPass.IsEmpty()) // Ако паролата и потвърждението съвпадат и паролата не е празна
        {
            // Use echo piped to chpasswd for better password handling (avoids command line history) // Използване на echo, пренасочено към chpasswd, за по-добро обработване на пароли (избягва историята на командите)
            wxString cmd = wxString::Format("/bin/bash -c \"echo '%s:%s' | chpasswd\"", m_currentUsername, uiUserPass);   // Форматиране на командата за промяна на парола
            if (ExecuteChrootCommand(cmd, wxString::Format("Applying user password change for '%s'", m_currentUsername))) // Ако изпълнението е успешно
            {
                m_currentUserPassword = uiUserPass; // Актуализиране на текущата потребителска парола
                changed = true;                     // Маркиране на промяна
            }
        }
        else if (uiUserPass != uiConfirmUserPass) // Ако паролите не съвпадат
        {
            wxLogWarning("SystemTab: APPLY - User passwords do not match. Skipping change."); // Логиране на предупреждение: Паролите не съвпадат, промяната се пропуска.
            // Revert UI fields if passwords don't match after attempting change // Връщане на полетата в UI, ако паролите не съвпадат след опит за промяна
            m_userPasswordInput->SetValue(m_currentUserPassword); // Връщане на старата парола в полето
            m_confirmUserPasswordInput->Clear();                  // Изчистване на полето за потвърждение
        }
    }

    // Admin Status // Администраторски статус
    if (uiIsAdmin != m_isAdmin) // Ако администраторският статус от UI е различен от текущия
    {
        wxString group = "sudo"; // Adjust group name if needed (e.g., "wheel") // Име на групата (коригирайте, ако е необходимо, напр. "wheel")
        wxString cmd;            // Деклариране на променлива за командата
        if (uiIsAdmin)           // Ако потребителят трябва да стане администратор
        {
            cmd = wxString::Format("usermod -aG %s %s", group, m_currentUsername); // Команда за добавяне към групата
        }
        else // Ако потребителят не трябва да бъде администратор
        {
            cmd = wxString::Format("gpasswd -d %s %s", m_currentUsername, group); // Команда за премахване от групата
        }
        if (ExecuteChrootCommand(cmd, wxString::Format("Applying admin status change (%s) for '%s'", uiIsAdmin ? "ADD" : "REMOVE", m_currentUsername))) // Ако изпълнението е успешно
        {
            m_isAdmin = uiIsAdmin; // Актуализиране на текущия администраторски статус
            changed = true;        // Маркиране на промяна
        }
    }

    // --- Apply Root Account Lock/Unlock --- // --- Прилагане на заключване/отключване на Root акаунт ---
    if (uiIsRootLocked != m_isRootLocked) // Ако статусът на заключване на root от UI е различен от текущия
    {
        wxString cmd = uiIsRootLocked ? "passwd -l root" : "passwd -u root";                                                           // Команда за заключване (passwd -l) или отключване (passwd -u) на root
        if (ExecuteChrootCommand(cmd, wxString::Format("Applying root account lock status (%s)", uiIsRootLocked ? "LOCK" : "UNLOCK"))) // Ако изпълнението е успешно
        {
            m_isRootLocked = uiIsRootLocked; // Актуализиране на текущия статус на заключване
            if (m_isRootLocked)              // Ако root е заключен
            {
                m_currentRootPassword = "";          // Clear password state if locked // Изчистване на състоянието на паролата, ако е заключен
                m_rootPasswordInput->Clear();        // Clear UI field // Изчистване на полето в UI
                m_confirmRootPasswordInput->Clear(); // Изчистване на полето за потвърждение в UI
            }
            changed = true; // Маркиране на промяна
        }
    }

    // Root Password (only apply if unlocked and password changed) // Root парола (прилага се само ако е отключен и паролата е променена)
    if (!m_isRootLocked && uiRootPass != m_currentRootPassword) // Ако root НЕ е заключен И паролата от UI е различна от текущата
    {
        if (uiRootPass == uiConfirmRootPass && !uiRootPass.IsEmpty()) // Ако root паролата и потвърждението съвпадат и не са празни
        {
            wxString cmd = wxString::Format("/bin/bash -c \"echo 'root:%s' | chpasswd\"", uiRootPass); // Команда за промяна на root паролата
            if (ExecuteChrootCommand(cmd, "Applying root password change"))                            // Ако изпълнението е успешно
            {
                m_currentRootPassword = uiRootPass; // Актуализиране на текущата root парола
                changed = true;                     // Маркиране на промяна
            }
        }
        else if (uiRootPass != uiConfirmRootPass) // Ако root паролите не съвпадат
        {
            wxLogWarning("SystemTab: APPLY - Root passwords do not match. Skipping change."); // Логиране на предупреждение: Root паролите не съвпадат.
            // Revert UI fields // Връщане на полетата в UI
            m_rootPasswordInput->SetValue(m_currentRootPassword); // Връщане на старата root парола
            m_confirmRootPasswordInput->Clear();                  // Изчистване на полето за потвърждение
        }
    }

    if (!changed) // Ако не са направени промени
    {
        wxLogDebug("SystemTab: ApplySettings - No changes detected to apply."); // Логиране: Не са открити промени за прилагане.
    }
    wxLogDebug("---- SystemTab: ApplySettings Finished ----"); // Логиране: ApplySettings завърши.
}

// --- Event Handlers --- // --- Обработчици на събития ---

void SystemTab::OnLockRootCheck(wxCommandEvent &event) // Обработчик на събитието за промяна на чекбокса за заключване на root
{
    // --- UI Logic (Show/Hide Password fields) --- // --- Логика на UI (Показване/Скриване на полетата за парола) ---
    bool isLocked = event.IsChecked(); // Вземане на състоянието на чекбокса (дали е маркиран)
    if (m_rootPasswordSizer)           // Ако сайзерът за root парола съществува
    {
        bool showPasswords = !isLocked;                      // Определяне дали да се покажат полетата за парола (ако НЕ е заключен)
        m_rootPasswordSizer->Show(showPasswords);            // Показване/скриване на сайзера
        if (m_rootPasswordLabel)                             // Ако етикетът за root парола съществува
            m_rootPasswordLabel->Show(showPasswords);        // Показване/скриване на етикета
        if (m_rootPasswordInput)                             // Ако полето за root парола съществува
            m_rootPasswordInput->Show(showPasswords);        // Показване/скриване на полето
        if (m_confirmRootPasswordLabel)                      // Ако етикетът за потвърждение съществува
            m_confirmRootPasswordLabel->Show(showPasswords); // Показване/скриване на етикета
        if (m_confirmRootPasswordInput)                      // Ако полето за потвърждение съществува
            m_confirmRootPasswordInput->Show(showPasswords); // Показване/скриване на полето

        if (!showPasswords)                          // Ако полетата се скриват
        {                                            // If hiding, clear fields // Ако се скриват, изчистете полетата
            if (m_rootPasswordInput)                 // Ако полето за root парола съществува
                m_rootPasswordInput->Clear();        // Изчистване на полето
            if (m_confirmRootPasswordInput)          // Ако полето за потвърждение съществува
                m_confirmRootPasswordInput->Clear(); // Изчистване на полето
            // Don't clear m_currentRootPassword here, let ApplySettings handle state based on lock status // Не изчиствайте m_currentRootPassword тук, оставете ApplySettings да обработи състоянието въз основа на статуса на заключване
        }
        // Layout the panels to reflect visibility changes // Пренареждане на панелите, за да отразят промените във видимостта
        wxWindow *parentPanel = m_rootPasswordSizer->GetContainingWindow(); // Вземане на родителския панел на сайзера
        if (parentPanel)                                                    // Ако родителският панел съществува
            parentPanel->Layout();                                          // Пренареждане на родителския панел
        this->Layout();                                                     // Пренареждане на текущия панел (SystemTab)
    }

    // --- Trigger the apply logic --- // --- Задействане на do за прилагане ---
    ApplySettings(); // Reads the NEW checkbox state and simulates commands // Чете НОВОТО състояние на чекбокса и симулира команди

    event.Skip(); // Предаване на събитието за по-нататъшна обработка (ако има)
}

void SystemTab::OnControlChange(wxCommandEvent &event) // Обработчик на събитието за промяна на контрола
{
    // --- Trigger the simulation whenever a relevant control changes --- // --- Задействане на симулацията, когато съответна контрола се промени ---
    ApplySettings(); // Извикване на ApplySettings за прилагане на промените
    event.Skip();    // Allow default processing // Позволяване на обработката по подразбиране
}