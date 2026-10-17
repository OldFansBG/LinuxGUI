#include "MainFrame.h"                 // Включва дефиницията на класа MainFrame.
#include "ISOReader.h"                 // Включва класа ISOReader за четене на ISO файлове.
#include "SettingsDialog.h"            // Включва диалога за настройки.
#include <wx/filedlg.h>                // Включва класа wxFileDialog за диалози за избор на файл.
#include <wx/dirdlg.h>                 // Включва класа wxDirDialog за диалози за избор на директория.
#include <wx/msgdlg.h>                 // Включва класа wxMessageBox за съобщения към потребителя.
#include <wx/tokenzr.h>                // Включва класа wxStringTokenizer за разбиване на низове.
#include <algorithm>                   // Включва стандартния алгоритъмен заглавен файл за общи алгоритми.
#include "FilesystemSelectionDialog.h" // Включва диалога за избор на файлова система.
#include <filesystem>                  // Включва стандартния заглавен файл за работа с файлова система.
#include <wx/artprov.h>                // Включва класа wxArtProvider за икони и изображения.
#include <wx/bmpbuttn.h>               // Включва класа wxBitmapButton за бутони с изображения.
#include <wx/settings.h>               // Включва класа wxSystemSettings за системни настройки.
#include <wx/display.h>                // Включва класа wxDisplay за информация за дисплея.
#include <wx/filename.h>               // Включва класа wxFileName за манипулиране на имена на файлове.
#include "SystemTheme.h"               // Включва класа SystemTheme за работа със системната тема.
#include "DesktopTab.h"                // Включва класа DesktopTab, вероятно за управление на потребителски интерфейс, свързан с работния плот.
#include <wx/textfile.h>               // Включва класа wxTextFile за работа с текстови файлове.

BEGIN_EVENT_TABLE(MainFrame, wxFrame)                     // Започва дефиницията на таблицата със събития за класа MainFrame, наследяващ wxFrame.
EVT_BUTTON(ID_BROWSE_ISO, MainFrame::OnBrowseISO)         // Свързва събитието за натискане на бутон с ID ID_BROWSE_ISO с метода OnBrowseISO.
EVT_BUTTON(ID_BROWSE_WORKDIR, MainFrame::OnBrowseWorkDir) // Свързва събитието за натискане на бутон с ID ID_BROWSE_WORKDIR с метода OnBrowseWorkDir.
EVT_BUTTON(ID_DETECT, MainFrame::OnDetect)                // Свързва събитието за натискане на бутон с ID ID_DETECT с метода OnDetect.
EVT_BUTTON(ID_NEXT, MainFrame::OnNextButton)              // Свързва събитието за натискане на бутон с ID ID_NEXT с метода OnNextButton (преди беше ID_EXTRACT).
EVT_BUTTON(ID_CANCEL, MainFrame::OnCancel)                // Свързва събитието за натискане на бутон с ID ID_CANCEL с метода OnCancel.
EVT_BUTTON(ID_SETTINGS, MainFrame::OnSettings)            // Свързва събитието за натискане на бутон с ID ID_SETTINGS с метода OnSettings.
END_EVENT_TABLE()                                         // Завършва дефиницията на таблицата със събития.

MainFrame::MainFrame(const wxString &title)                                                           // Конструктор на класа MainFrame, приемащ заглавие като wxString.
    : wxFrame()                                                                                       // Инициализира базовия клас wxFrame по подразбиране.
{                                                                                                     // Начало на тялото на конструктора.
    Create(NULL, wxID_ANY, title, wxDefaultPosition, wxDefaultSize,                                   // Създава рамката на прозореца.
           wxCAPTION | wxCLOSE_BOX | wxMINIMIZE_BOX | wxSYSTEM_MENU | wxNO_BORDER | wxCLIP_CHILDREN); // Задава стилове на прозореца: заглавна лента, бутон за затваряне, бутон за минимизиране, системно меню, без рамка, изрязване на дъщерни елементи.

    // Load and set the application icon
    // Зареждане и задаване на иконата на приложението
    wxIcon icon;                                                     // Създава обект за икона.
    icon.LoadFile(wxT("IDI_MAIN_ICON"), wxBITMAP_TYPE_ICO_RESOURCE); // Опитва да зареди иконата от ресурс с име "IDI_MAIN_ICON".
    if (icon.IsOk())                                                 // Проверява дали иконата е заредена успешно.
    {                                                                // Начало на if блока.
        SetIcon(icon);                                               // Задава заредената икона за прозореца.
    } // Край на if блока.
    else                                                                                                                   // Ако зареждането на иконата е неуспешно.
    {                                                                                                                      // Начало на else блока.
        wxLogWarning("Failed to load application icon 'linuxisopro.ico'. Check if the file is in the correct directory."); // Извежда предупреждение в log-а.
    } // Край на else блока.

    SetBackgroundStyle(wxBG_STYLE_PAINT); // Задава стил за рисуване на фона (позволява персонализирано рисуване).

    // Load the themes first
    // Първо зареждане на темите
    ThemeConfig &themeConfig = ThemeConfig::Get(); // Взема референция към сингълтън инстанцията на ThemeConfig.
    themeConfig.LoadThemes("themes.json");         // Зарежда дефинициите на темите от файла "themes.json".

    // Register for system theme changes
    // Регистриране за промени в системната тема
    SystemTheme::RegisterForThemeChanges(this); // Регистрира текущия прозорец (this) да получава известия за промени в системната тема.

    // Register this window with ThemeConfig
    // Регистриране на този прозорец в ThemeConfig
    themeConfig.RegisterWindow(this); // Регистрира текущия прозорец (this) в ThemeConfig, за да може темата да бъде приложена към него.

#ifdef __WXMSW__                                                                               // Условен компилационен блок: само ако компилираме за Microsoft Windows (MSW).
    HWND hwnd = GetHandle();                                                                   // Взема Win32 хендъл (HWND) на прозореца.
    if (hwnd)                                                                                  // Проверява дали хендълът е валиден.
    {                                                                                          // Начало на if блока.
        LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);                                    // Взема текущите стилове на прозореца.
        style &= ~(WS_CAPTION | WS_OVERLAPPED | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX); // Премахва стилове като заглавна лента, припокриване, системно меню, бутони за минимизиране/максимизиране.
        style &= ~WS_THICKFRAME;                                                               // Премахва стила за дебела рамка, която позволява преоразмеряване.
        SetWindowLongPtr(hwnd, GWL_STYLE, style);                                              // Задава модифицираните стилове на прозореца.

        LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);                                     // Взема текущите разширени стилове на прозореца.
        exStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE | WS_EX_WINDOWEDGE); // Премахва разширени стилове като рамка на модален диалог, ръбове и т.н.
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle);                                               // Задава модифицираните разширени стилове на прозореца.

        BOOL value = TRUE;                                                     // Дефинира булева променлива и я инициализира с TRUE.
        DwmSetWindowAttribute(hwnd, 19, &value, sizeof(value));                // Задава атрибут на прозореца за "тъмен режим" (атрибут 19 е DWMWA_USE_IMMERSIVE_DARK_MODE).
        DwmSetWindowAttribute(hwnd, DWMWA_MICA_EFFECT, &value, sizeof(value)); // Задава атрибут на прозореца за "Mica ефект" (полупрозрачност на фона).

        SetWindowPos(hwnd, NULL, 0, 0, 0, 0,                                     // Променя позицията и размера на прозореца.
                     SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER); // Флагове: уведомява системата за промяна на рамката, без промяна на позиция, размер или Z-ред.
    } // Край на if блока.
#endif // Край на условния компилационен блок за Windows.

    m_titleBar = new CustomTitleBar(this);   // Създава нова персонализирана заглавна лента и я асоциира с текущия прозорец.
    m_statusBar = new CustomStatusBar(this); // Създава нова персонализирана лента за състояние и я асоциира с текущия прозорец.

    if (!LoadConfig())                                                             // Извиква LoadConfig() за зареждане на конфигурация и проверява дали е неуспешно.
    {                                                                              // Начало на if блока.
        SetStatusText("Failed to load configuration file. Using default values."); // Задава текст в лентата за състояние, ако зареждането на конфигурацията е неуспешно.
    } // Край на if блока.

    CreateFrameControls(); // Създава контролите (UI елементите) в рамката на прозореца.

    // Initialize with system theme
    // Инициализиране със системна тема
    themeConfig.InitializeWithSystemTheme(); // Инициализира приложението със текущата системна тема.

    SetMinSize(wxSize(600, 500)); // Задава минимален размер на прозореца 600x500 пиксела.
    SetSize(wxSize(600, 500));    // Задава начален размер на прозореца 600x500 пиксела.

    m_progressGauge->SetValue(0); // Инициализира прогрес бара (gauge) на 0.
    SetStatusText("Ready");       // Задава "Ready" като текст в лентата за състояние.

    Centre(); // Центрира прозореца на екрана.
    Layout(); // Преизчислява и подрежда оформлението на елементите в прозореца.

    // Create DesktopTab instance
    // Създаване на инстанция на DesktopTab
    m_desktopTab = new DesktopTab(this); // Създава нов обект DesktopTab, асоцииран с текущия MainFrame.
    m_desktopTab->Hide();                // Не е част от оформлението - иначе се рисува върху горния ляв ъгъл на прозореца

    // Bind custom event to handler
    // Свързване на персонализирано събитие с обработчик
    Bind(FILE_COPY_COMPLETE_EVENT, &MainFrame::OnGUIDetected, this); // Свързва персонализирано събитие (FILE_COPY_COMPLETE_EVENT) с метода OnGUIDetected на текущия обект.
} // Край на тялото на конструктора.

MainFrame::~MainFrame() // Деструктор на класа MainFrame.
{                       // Начало на тялото на деструктора.
    // Unregister from ThemeConfig when window is destroyed
    // Отписване от ThemeConfig, когато прозорецът е унищожен
    ThemeConfig::Get().UnregisterWindow(this); // Отписва текущия прозорец от ThemeConfig, за да избегне проблеми след унищожаване на обекта.
} // Край на тялото на деструктора.

void MainFrame::CreateFrameControls()       // Метод за създаване на контролите (UI елементи) на рамката.
{                                           // Начало на тялото на метода.
    wxPanel *mainPanel = new wxPanel(this); // Създава основен панел, който ще съдържа всички други панели.

    wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);                      // Създава вертикален сайзър за основния панел.
    mainSizer->Add(CreateLogoPanel(mainPanel), 0, wxEXPAND | wxALL, 5);      // Добавя панела с логото, разтягащ се, с отстъп от 5 пиксела.
    mainSizer->Add(CreateDetectionPanel(mainPanel), 0, wxEXPAND | wxALL, 5); // Добавя панела за детекция, разтягащ се, с отстъп от 5 пиксела.
    mainSizer->Add(CreateProjectPanel(mainPanel), 0, wxEXPAND | wxALL, 5);   // Добавя панела за проект, разтягащ се, с отстъп от 5 пиксела.
    mainSizer->Add(CreateProgressPanel(mainPanel), 0, wxEXPAND | wxALL, 5);  // Добавя панела за прогрес, разтягащ се, с отстъп от 5 пиксела.

    mainPanel->SetSizer(mainSizer); // Задава основния сайзър на основния панел.

    wxBoxSizer *frameSizer = new wxBoxSizer(wxVERTICAL); // Създава вертикален сайзър за цялата рамка.
    frameSizer->Add(m_titleBar, 0, wxEXPAND);            // Добавя персонализираната заглавна лента, разтягаща се.
    frameSizer->Add(mainPanel, 1, wxEXPAND);             // Добавя основния панел, който се разтяга и заема останалото пространство.
    frameSizer->Add(m_statusBar, 0, wxEXPAND);           // Добавя персонализираната лента за състояние, разтягаща се.
    SetSizer(frameSizer);                                // Задава сайзъра на рамката.
} // Край на тялото на метода.

void MainFrame::SetStatusText(const wxString &text) // Метод за задаване на текст в лентата за състояние.
{                                                   // Начало на тялото на метода.
    if (m_statusBar)                                // Проверява дали лентата за състояние е инициализирана.
    {                                               // Начало на if блока.
        m_statusBar->SetStatusText(text);           // Задава текста в лентата за състояние.
    } // Край на if блока.
} // Край на тялото на метода.

wxPanel *MainFrame::CreateLogoPanel(wxWindow *parent)                      // Метод за създаване на панела с логото/заглавката.
{                                                                          // Начало на тялото на метода.
    wxPanel *panel = new wxPanel(parent);                                  // Създава нов панел, чийто родител е подаденият родител.
    wxStaticBoxSizer *sizer = new wxStaticBoxSizer(wxVERTICAL, panel, ""); // Създава вертикален сайзър със статична кутия (рамка) за панела. Рамката е празна.
    wxPanel *headerPanel = new wxPanel(panel);                             // Създава вторичен панел за съдържанието на заглавката.

    wxBoxSizer *headerSizer = new wxBoxSizer(wxHORIZONTAL); // Създава хоризонтален сайзър за съдържанието на заглавката.

    m_settingsButton = new MyButton(headerPanel, ID_SETTINGS,           // Създава персонализиран бутон за настройки.
                                    "gear.png",                         // Път до изображението на бутона.
                                    wxDefaultPosition, wxSize(32, 32)); // Позиция по подразбиране и размер 32x32 пиксела.
    m_settingsButton->SetAlwaysShowButton(true);                        // Задава бутонът винаги да бъде видим.

    headerSizer->AddStretchSpacer();                                            // Добавя разтягащ се спейсър, за да избута бутона надясно.
    headerSizer->Add(m_settingsButton, 0, wxALIGN_CENTER_VERTICAL | wxALL, 10); // Добавя бутона за настройки, центриран вертикално, с отстъп от 10 пиксела от всички страни.

    headerPanel->SetSizer(headerSizer);              // Задава сайзъра на панела за заглавката.
    sizer->Add(headerPanel, 0, wxEXPAND | wxALL, 5); // Добавя панела за заглавката към главния сайзър на този панел, разтягащ се, с отстъп от 5 пиксела.

    panel->SetSizer(sizer); // Задава главния сайзър на панела.
    return panel;           // Връща указател към създадения панел.
} // Край на тялото на метода.

wxPanel *MainFrame::CreateDetectionPanel(wxWindow *parent)                 // Метод за създаване на панела за детекция на дистрибуция.
{                                                                          // Начало на тялото на метода.
    wxPanel *panel = new wxPanel(parent);                                  // Създава нов панел, чийто родител е подаденият родител.
    wxStaticBoxSizer *sizer = new wxStaticBoxSizer(wxVERTICAL, panel, ""); // Създава вертикален сайзър със статична кутия за панела.
    wxPanel *contentPanel = new wxPanel(panel);                            // Създава вторичен панел за съдържанието.

    wxBoxSizer *isoSizer = new wxBoxSizer(wxHORIZONTAL);                   // Създава хоризонтален сайзър за полето за ISO файл.
    auto isoLabel = new wxStaticText(contentPanel, wxID_ANY, "ISO File:"); // Създава статичен текст "ISO File:".
    isoSizer->Add(isoLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);      // Добавя етикета, центриран вертикално, с десен отстъп от 5 пиксела.

    m_isoPathCtrl = new wxTextCtrl(contentPanel, wxID_ANY, "",                         // Създава текстово поле за пътя до ISO файла.
                                   wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE); // Позиция, размер по подразбиране, с проста рамка.
    isoSizer->Add(m_isoPathCtrl, 1, wxRIGHT, 5);                                       // Добавя текстовото поле, което се разтяга, с десен отстъп от 5 пиксела.

    wxButton *browseButton = new wxButton(contentPanel, ID_BROWSE_ISO, "Browse",             // Създава бутон "Browse" за избор на ISO файл.
                                          wxDefaultPosition, wxSize(-1, 25), wxBORDER_NONE); // Позиция, фиксирана височина 25, без рамка.
    isoSizer->Add(browseButton, 0);                                                          // Добавя бутона.

    wxBoxSizer *detectSizer = new wxBoxSizer(wxHORIZONTAL);                                  // Създава хоризонтален сайзър за бутона за детекция и полето за дистрибуция.
    wxButton *detectButton = new wxButton(contentPanel, ID_DETECT, "Detect",                 // Създава бутон "Detect".
                                          wxDefaultPosition, wxSize(-1, 25), wxBORDER_NONE); // Позиция, фиксирана височина 25, без рамка.
    detectSizer->Add(detectButton, 0, wxRIGHT, 5);                                           // Добавя бутона за детекция, с десен отстъп от 5 пиксела.

    auto distroLabel = new wxStaticText(contentPanel, wxID_ANY, "Detected Distribution:"); // Създава статичен текст "Detected Distribution:".
    detectSizer->Add(distroLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);                // Добавя етикета, центриран вертикално, с десен отстъп от 5 пиксела.

    m_distroCtrl = new wxTextCtrl(contentPanel, wxID_ANY, "",                                         // Създава текстово поле за показаната дистрибуция.
                                  wxDefaultPosition, wxDefaultSize, wxTE_READONLY | wxBORDER_SIMPLE); // Само за четене, с проста рамка.
    detectSizer->Add(m_distroCtrl, 1);                                                                // Добавя текстовото поле, което се разтяга.

    wxBoxSizer *contentSizer = new wxBoxSizer(wxVERTICAL);  // Създава вертикален сайзър за цялото съдържание на панела.
    contentSizer->Add(isoSizer, 0, wxEXPAND | wxALL, 5);    // Добавя сайзъра за ISO файл, разтягащ се, с отстъп от 5 пиксела.
    contentSizer->Add(detectSizer, 0, wxEXPAND | wxALL, 5); // Добавя сайзъра за детекция, разтягащ се, с отстъп от 5 пиксела.
    contentPanel->SetSizer(contentSizer);                   // Задава сайзъра на панела за съдържанието.

    sizer->Add(contentPanel, 1, wxEXPAND | wxALL, 2); // Добавя панела за съдържание към главния сайзър на този панел, разтягащ се, с отстъп от 2 пиксела.
    panel->SetSizer(sizer);                           // Задава главния сайзър на панела.
    return panel;                                     // Връща указател към създадения панел.
} // Край на тялото на метода.

wxPanel *MainFrame::CreateProjectPanel(wxWindow *parent)                   // Метод за създаване на панела за информация за проекта.
{                                                                          // Начало на тялото на метода.
    wxPanel *panel = new wxPanel(parent);                                  // Създава нов панел, чийто родител е подаденият родител.
    wxStaticBoxSizer *sizer = new wxStaticBoxSizer(wxVERTICAL, panel, ""); // Създава вертикален сайзър със статична кутия за панела.
    wxPanel *contentPanel = new wxPanel(panel);                            // Създава вторичен панел за съдържанието.

    wxFlexGridSizer *gridSizer = new wxFlexGridSizer(2, 2, 5, 5); // Създава гъвкав мрежов сайзър с 2 реда, 2 колони, с 5 пиксела вертикален и хоризонтален отстъп.
    gridSizer->AddGrowableCol(1, 1);                              // Задава втората колона (индекс 1) да бъде разтягаща се с коефициент 1.

    auto nameLabel = new wxStaticText(contentPanel, wxID_ANY, "Project Name:"); // Създава статичен текст "Project Name:".
    gridSizer->Add(nameLabel, 0, wxALIGN_CENTER_VERTICAL);                      // Добавя етикета, центриран вертикално.

    m_projectNameCtrl = new wxTextCtrl(contentPanel, wxID_ANY, "",                          // Създава текстово поле за име на проекта.
                                       wxDefaultPosition, wxSize(-1, 25), wxBORDER_SIMPLE); // Позиция, фиксирана височина 25, с проста рамка.
    gridSizer->Add(m_projectNameCtrl, 1, wxEXPAND);                                         // Добавя текстовото поле, което се разтяга хоризонтално.

    auto versionLabel = new wxStaticText(contentPanel, wxID_ANY, "Version:"); // Създава статичен текст "Version:".
    gridSizer->Add(versionLabel, 0, wxALIGN_CENTER_VERTICAL);                 // Добавя етикета, центриран вертикално.

    m_versionCtrl = new wxTextCtrl(contentPanel, wxID_ANY, "",                          // Създава текстово поле за версията.
                                   wxDefaultPosition, wxSize(-1, 25), wxBORDER_SIMPLE); // Позиция, фиксирана височина 25, с проста рамка.
    gridSizer->Add(m_versionCtrl, 1, wxEXPAND);                                         // Добавя текстовото поле, което се разтяга хоризонтално.

    wxBoxSizer *dirSizer = new wxBoxSizer(wxHORIZONTAL);                            // Създава хоризонтален сайзър за работен каталог.
    auto dirLabel = new wxStaticText(contentPanel, wxID_ANY, "Working Directory:"); // Създава статичен текст "Working Directory:".
    dirSizer->Add(dirLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);               // Добавя етикета, центриран вертикално, с десен отстъп от 5 пиксела.

    m_workDirCtrl = new wxTextCtrl(contentPanel, wxID_ANY, "",                          // Създава текстово поле за пътя до работния каталог.
                                   wxDefaultPosition, wxSize(-1, 25), wxBORDER_SIMPLE); // Позиция, фиксирана височина 25, с проста рамка.
    dirSizer->Add(m_workDirCtrl, 1, wxRIGHT, 5);                                        // Добавя текстовото поле, което се разтяга, с десен отстъп от 5 пиксела.

    wxButton *workDirButton = new wxButton(contentPanel, ID_BROWSE_WORKDIR, "Browse",         // Създава бутон "Browse" за избор на работен каталог.
                                           wxDefaultPosition, wxSize(-1, 25), wxBORDER_NONE); // Позиция, фиксирана височина 25, без рамка.
    dirSizer->Add(workDirButton, 0);                                                          // Добавя бутона.

    wxBoxSizer *contentSizer = new wxBoxSizer(wxVERTICAL); // Създава вертикален сайзър за цялото съдържание на панела.
    contentSizer->Add(gridSizer, 0, wxEXPAND | wxALL, 5);  // Добавя мрежовия сайзър, разтягащ се, с отстъп от 5 пиксела.
    contentSizer->Add(dirSizer, 0, wxEXPAND | wxALL, 5);   // Добавя сайзъра за директорията, разтягащ се, с отстъп от 5 пиксела.
    contentPanel->SetSizer(contentSizer);                  // Задава сайзъра на панела за съдържанието.

    sizer->Add(contentPanel, 1, wxEXPAND | wxALL, 2); // Добавя панела за съдържание към главния сайзър на този панел, разтягащ се, с отстъп от 2 пиксела.
    panel->SetSizer(sizer);                           // Задава главния сайзър на панела.
    return panel;                                     // Връща указател към създадения панел.
} // Край на тялото на метода.

wxPanel *MainFrame::CreateProgressPanel(wxWindow *parent)                  // Метод за създаване на панела за прогрес и бутони за действие.
{                                                                          // Начало на тялото на метода.
    wxPanel *panel = new wxPanel(parent);                                  // Създава нов панел, чийто родител е подаденият родител.
    wxStaticBoxSizer *sizer = new wxStaticBoxSizer(wxVERTICAL, panel, ""); // Създава вертикален сайзър със статична кутия за панела.
    wxPanel *contentPanel = new wxPanel(panel);                            // Създава вторичен панел за съдържанието.

    wxBoxSizer *contentSizer = new wxBoxSizer(wxVERTICAL); // Създава вертикален сайзър за съдържанието.

    m_progressGauge = new wxGauge(contentPanel, wxID_ANY, 100,        // Създава прогрес бар с максимална стойност 100.
                                  wxDefaultPosition, wxSize(-1, 20)); // Позиция по подразбиране, фиксирана височина 20.
    contentSizer->Add(m_progressGauge, 0, wxEXPAND | wxALL, 5);       // Добавя прогрес бара, разтягащ се, с отстъп от 5 пиксела.

    m_statusText = new wxStaticText(contentPanel, wxID_ANY, "");        // Създава статичен текстов контрол за показване на статус съобщения.
    contentSizer->Add(m_statusText, 0, wxEXPAND | wxLEFT | wxRIGHT, 5); // Добавя статус текста, разтягащ се, с ляв и десен отстъп от 5 пиксела.

    wxBoxSizer *buttonSizer = new wxBoxSizer(wxHORIZONTAL); // Създава хоризонтален сайзър за бутоните.
    buttonSizer->AddStretchSpacer();                        // Добавя разтягащ се спейсър, за да избута бутоните надясно.

    wxButton *nextButton = new wxButton(contentPanel, ID_NEXT, "Next",                       // Създава бутон "Next" (преди беше "Extract").
                                        wxDefaultPosition, wxSize(-1, 25), wxBORDER_NONE);   // Позиция, фиксирана височина 25, без рамка.
    wxButton *cancelButton = new wxButton(contentPanel, ID_CANCEL, "Cancel",                 // Създава бутон "Cancel".
                                          wxDefaultPosition, wxSize(-1, 25), wxBORDER_NONE); // Позиция, фиксирана височина 25, без рамка.

    buttonSizer->Add(nextButton, 0, wxRIGHT, 5); // Добавя бутона "Next", с десен отстъп от 5 пиксела.
    buttonSizer->Add(cancelButton, 0);           // Добавя бутона "Cancel".

    contentSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 5); // Добавя сайзъра с бутоните, разтягащ се, с отстъп от 5 пиксела.
    contentPanel->SetSizer(contentSizer);                   // Задава сайзъра на панела за съдържанието.
    sizer->Add(contentPanel, 1, wxEXPAND | wxALL, 2);       // Добавя панела за съдържание към главния сайзър на този панел, разтягащ се, с отстъп от 2 пиксела.
    panel->SetSizer(sizer);                                 // Задава главния сайзър на панела.
    return panel;                                           // Връща указател към създадения панел.
} // Край на тялото на метода.

void MainFrame::OpenSecondWindow()                                           // Метод за отваряне на втория прозорец.
{                                                                            // Начало на тялото на метода.
    SecondWindow *secondWindow = new SecondWindow(this, "Analyzer",          // Създава нова инстанция на SecondWindow.
                                                  m_isoPathCtrl->GetValue(), // Предава пътя до ISO файла.
                                                  m_workDirCtrl->GetValue(), // Предава работния каталог.
                                                  m_desktopTab);             // Предава указател към m_desktopTab като пети параметър.
    this->Hide();                                                            // Скрива текущия (първи) прозорец.
    secondWindow->Show(true);                                                // Показва втория прозорец.
} // Край на тялото на метода.

void MainFrame::OnBrowseISO(wxCommandEvent &event)                 // Обработва събитието при натискане на бутона "Browse ISO".
{                                                                  // Начало на тялото на метода.
    wxFileDialog openFileDialog(this, "Open ISO file", "", "",     // Създава диалог за отваряне на файл.
                                "ISO files (*.iso)|*.iso",         // Филтър за файлове: само .iso файлове.
                                wxFD_OPEN | wxFD_FILE_MUST_EXIST); // Стил на диалога: отваряне, файлът трябва да съществува.
    if (openFileDialog.ShowModal() == wxID_CANCEL)                 // Показва диалога и проверява дали потребителят е натиснал "Cancel".
        return;                                                    // Излиза от метода, ако потребителят е натиснал "Cancel".

    m_isoPathCtrl->SetValue(openFileDialog.GetPath());            // Задава избрания път до ISO файла в текстовото поле.
    m_distroCtrl->SetValue("");                                   // Изчиства полето за засечена дистрибуция.
    m_progressGauge->SetValue(0);                                 // Задава прогрес бара на 0.
    SetStatusText("ISO file selected. Click Detect to analyze."); // Задава статус текст.
} // Край на тялото на метода.

void MainFrame::OnBrowseWorkDir(wxCommandEvent &event)               // Обработва събитието при натискане на бутона "Browse WorkDir".
{                                                                    // Начало на тялото на метода.
    wxDirDialog dirDialog(this, "Choose working directory", "",      // Създава диалог за избор на директория.
                          wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST); // Стил на диалога: стандартен, директорията трябва да съществува.
    if (dirDialog.ShowModal() == wxID_CANCEL)                        // Показва диалога и проверява дали потребителят е натиснал "Cancel".
        return;                                                      // Излиза от метода, ако потребителят е натиснал "Cancel".
    m_workDirCtrl->SetValue(dirDialog.GetPath());                    // Задава избрания път до работната директория в текстовото поле.
} // Край на тялото на метода.

void MainFrame::OnNextButton(wxCommandEvent &event) // Обработва събитието при натискане на бутона "Next".
{                                                   // Начало на тялото на метода.
    // Validate the inputs before anything is written
    // Първо проверява въведените данни, преди да се запише нещо
    if (m_workDirCtrl->IsEmpty() || m_isoPathCtrl->IsEmpty())              // Проверява дали текстовите полета за работен каталог или ISO път са празни.
    {                                                                      // Начало на if блока.
        wxMessageBox("Please select both ISO file and working directory.", // Показва съобщение за грешка.
                     "Error", wxOK | wxICON_ERROR);                        // Заглавие, бутон OK, икона за грешка.
        return;                                                            // Излиза от метода.
    } // Край на if блока.

    if (m_projectNameCtrl->GetValue().Trim().IsEmpty())                             // Проверява дали полето за име на проекта е празно.
    {                                                                               // Начало на if блока.
        wxMessageBox("Please enter a project name.", "Error", wxOK | wxICON_ERROR); // Показва съобщение за грешка.
        return;                                                                     // Излиза от метода.
    } // Край на if блока.

    if (!wxDirExists(m_workDirCtrl->GetValue()))                                                        // Работната директория трябва да съществува
    {
        wxMessageBox("The working directory does not exist.", "Error", wxOK | wxICON_ERROR, this);
        return;
    }

    // Then save the settings
    // След това запазва настройките
    AppSettings settings;                         // Създава обект за настройките на приложението.
    settings.isoPath = m_isoPathCtrl->GetValue(); // Взема пътя до ISO файла от контролата и го запазва.
    settings.workDir = m_workDirCtrl->GetValue(); // Взема работния каталог от контролата и го запазва.

    // Remove spaces from the project name before saving
    // Премахва интервалите от името на проекта преди запазване
    wxString projectName = m_projectNameCtrl->GetValue(); // Взема името на проекта от контролата.
    projectName.Replace(" ", "");                         // Заменя всички интервали в името на проекта с празен низ.
    settings.projectName = projectName;                   // Записва модифицираното име на проекта.

    settings.version = m_versionCtrl->GetValue();       // Взема версията от контролата и я запазва.
    settings.detectedDistro = m_distroCtrl->GetValue(); // Взема засечената дистрибуция от контролата и я запазва.

    wxString configPath = m_workDirCtrl->GetValue() + wxFILE_SEP_PATH + "settings.json"; // Създава пътя до файла за настройки (в работната директория).

    if (!m_settingsManager.SaveSettings(settings, configPath))                  // Опитва да запази настройките и проверява дали е неуспешно.
    {                                                                           // Начало на if блока.
        wxMessageBox("Failed to save settings!", "Error", wxOK | wxICON_ERROR); // Показва съобщение за грешка.
        return;                                                                 // Спира изпълнението, ако запазването е неуспешно.
    } // Край на if блока.

    // Search for SquashFS files in the ISO
    // Търсене на SquashFS файлове в ISO-то
    wxArrayString squashfsFiles;                                                                                                  // Създава масив от низове за съхранение на намерените SquashFS файлове.
    if (!SearchSquashFS(m_isoPathCtrl->GetValue(), squashfsFiles))                                                                // Извиква SearchSquashFS и проверява дали е неуспешно (не са намерени файлове).
    {                                                                                                                             // Начало на if блока.
        wxMessageBox("No SquashFS/SFS files found in the ISO.\nThe ISO might not be a Linux distribution or might be corrupted.", // Показва съобщение за грешка.
                     "Error", wxOK | wxICON_ERROR);                                                                               // Заглавие, бутон OK, икона за грешка.
        return;                                                                                                                   // Излиза от метода.
    } // Край на if блока.

    // Too many squashfs files - present user with selection dialog
    // Твърде много squashfs файлове - показва на потребителя диалог за избор
    wxString selectedFsFile;            // Променлива за съхранение на избрания файл от файловата система.
    if (squashfsFiles.GetCount() > 1)   // Проверява дали са намерени повече от един SquashFS файл.
    {                                   // Начало на if блока.
        wxArrayString prioritizedFiles; // Създава масив за приоритизирани файлове с описания.
        wxArrayString descriptions;     // (Този ред всъщност не се използва, но е остатък от предишна логика или грешка)

        // Create prioritized list with descriptions
        // Създаване на приоритизиран списък с описания
        for (const wxString &file : squashfsFiles) // Обхожда всички намерени SquashFS файлове.
        {                                          // Начало на for цикъла.
            wxString lowerFile = file.Lower();     // Преобразува името на файла в малки букви за сравнение.
            wxString description;                  // Променлива за описанието.

            // Add descriptions to help user identify files
            // Добавя описания, за да помогне на потребителя да идентифицира файловете
            if (lowerFile.Contains("airootfs.sfs"))                                                                                 // Проверява за специфичен Arch Linux файл.
                description = " (Arch Linux main filesystem)";                                                                      // Добавя описание за Arch Linux.
            else if (lowerFile.Contains("minimal.standard") && !lowerFile.Contains("enhanced") && !lowerFile.Contains("languages")) // Проверява за Ubuntu стандартен файл.
                description = " (Ubuntu standard filesystem)";                                                                      // Добавя описание за Ubuntu.
            else if (lowerFile.Contains("rootfs"))                                                                                  // Проверява за общ rootfs файл.
                description = " (Root filesystem)";                                                                                 // Добавя общо описание.

            prioritizedFiles.Add(file + description); // Добавя файла и неговото описание към приоритизирания списък.
        } // Край на for цикъла.

        // Show custom selection dialog
        // Показва персонализиран диалог за избор
        FilesystemSelectionDialog dialog(this,                                                             // Създава диалог за избор на файлова система.
                                         "Multiple filesystem files found. Please select the one to use:", // Заглавие на диалога.
                                         prioritizedFiles);                                                // Списък с файлове за избор.

        if (dialog.ShowModal() == wxID_OK)                   // Показва диалога и проверява дали потребителят е избрал файл и е натиснал OK.
        {                                                    // Начало на if блока.
            selectedFsFile = dialog.GetSelectedFilesystem(); // Взема избрания файл (санирано име).
        } // Край на if блока.
        else // Ако потребителят е натиснал "Cancel".
        {    // Начало на else блока.
            // User canceled
            // Потребителят е отменил
            return; // Излиза от метода.
        } // Край на else блока.
    } // Край на if блока (повече от един файл).
    else if (squashfsFiles.GetCount() == 1) // Ако е намерен само един SquashFS файл.
    {                                       // Начало на else if блока.
        // Just one file, use it directly
        // Само един файл, използва се директно
        selectedFsFile = squashfsFiles[0]; // Използва директно единствения намерен файл.
    } // Край на else if блока.

    // Save the selected filesystem file path
    // Записва пътя на избрания файл от файловата система
    wxString fsFilePath = m_workDirCtrl->GetValue() + wxFILE_SEP_PATH + projectName + "_selected_fs.txt"; // Създава пътя до файл, който ще съдържа името на избраната файлова система.
    wxTextFile file(fsFilePath);                                                                          // Създава обект wxTextFile за работа с този файл.

    if (selectedFsFile.StartsWith("/"))         // Проверява дали избраният файл започва с наклонена черта (абсолютен път в ISO-то).
    {                                           // Начало на if блока.
        selectedFsFile = selectedFsFile.Mid(1); // Премахва водещата наклонена черта.
    } // Край на if блока.

    if (file.Exists()) // Проверява дали файлът съществува.
        file.Open();   // Ако съществува, го отваря.
    else               // Ако не съществува.
        file.Create(); // Създава го.

    file.Clear();                 // Изчиства съдържанието на файла.
    file.AddLine(selectedFsFile); // Добавя избрания файл като ред във файла.

    if (!file.Write())                                                                                   // Опитва да запише файла и проверява дали е неуспешно.
    {                                                                                                    // Начало на if блока.
        wxMessageBox("Failed to save selected filesystem file path!", "Warning", wxOK | wxICON_WARNING); // Показва предупреждение.
    } // Край на if блока.
    file.Close(); // Затваря файла.

    // Log what was found and selected
    // Записва какво е намерено и избрано
    wxString message = wxString::Format("Found %zu filesystem files. Selected: %s", // Форматира съобщение.
                                        squashfsFiles.GetCount(), selectedFsFile);  // Добавя броя на намерените файлове и избрания файл.
    SetStatusText(message);                                                         // Задава съобщението в лентата за състояние.

    // Store current ISO path and proceed
    // Записва текущия ISO път и продължава
    m_currentISOPath = m_isoPathCtrl->GetValue(); // Записва текущия ISO път в член-променлива.

    // Open second window
    // Отваря втория прозорец
    SecondWindow *secondWindow = new SecondWindow(this, "Analyzer",          // Създава нова инстанция на SecondWindow.
                                                  m_currentISOPath,          // Предава текущия ISO път.
                                                  m_workDirCtrl->GetValue(), // Предава работния каталог.
                                                  m_desktopTab);             // Предава указател към m_desktopTab.
    this->Hide();                                                            // Скрива текущия прозорец.
    secondWindow->Show(true);                                                // Показва втория прозорец.
} // Край на тялото на метода.

void MainFrame::OnCancel(wxCommandEvent &event)                                  // Обработва събитието при натискане на бутона "Cancel".
{                                                                                // Начало на тялото на метода.
    wxWindowList &windows = wxTopLevelWindows;                                   // Взема списък с всички прозорци от най-високо ниво.
    for (wxWindowList::iterator it = windows.begin(); it != windows.end(); ++it) // Обхожда всички прозорци в списъка.
    {                                                                            // Начало на for цикъла.
        (*it)->Close(true);                                                      // Затваря всеки прозорец принудително.
    } // Край на for цикъла.
} // Край на тялото на метода.

void MainFrame::OnSettings(wxCommandEvent &event)            // Обработва събитието при натискане на бутона "Settings".
{                                                            // Начало на тялото на метода.
    SettingsDialog *dialog = new SettingsDialog(this);       // Създава нов диалог за настройки, чийто родител е текущият прозорец.
    if (dialog->ShowModal() == wxID_OK)                      // Показва диалога модално и проверява дали потребителят е натиснал OK.
    {                                                        // Начало на if блока.
        wxString selectedTheme = dialog->GetSelectedTheme(); // Взема избраната тема от диалога.
        ThemeConfig::Get().LoadThemes();                     // Презарежда дефинициите на темите.
        ThemeConfig::Get().SetCurrentTheme(selectedTheme);   // Задава избраната тема като текуща.
        ThemeConfig::Get().ApplyTheme(this, selectedTheme);  // Прилага избраната тема към текущия прозорец.
        Layout();                                            // Преизчислява и подрежда оформлението на елементите.
        Refresh(true);                                       // Принуждава прерисуване на прозореца и неговите деца.
        Update();                                            // Обновява прозореца незабавно.
    } // Край на if блока.
    dialog->Destroy(); // Унищожава обекта на диалога (освобождава памет).
} // Край на тялото на метода.

bool MainFrame::LoadConfig()                                                             // Метод за зареждане на конфигурационния файл.
{                                                                                        // Начало на тялото на метода.
    try                                                                                  // Започва блок за обработка на изключения.
    {                                                                                    // Начало на try блока.
        std::filesystem::path exePath = std::filesystem::current_path() / "config.yaml"; // Дефинира път до "config.yaml" в текущата директория на изпълнимия файл.
        if (std::filesystem::exists(exePath))                                            // Проверява дали файлът съществува на този път.
        {                                                                                // Начало на if блока.
            m_config = YAML::LoadFile(exePath.string());                                 // Зарежда YAML файла.
            return true;                                                                 // Връща true, ако е успешно.
        } // Край на if блока.

        std::filesystem::path buildPath = std::filesystem::current_path() / "build" / "config.yaml"; // Дефинира път до "config.yaml" в поддиректория "build".
        if (std::filesystem::exists(buildPath))                                                      // Проверява дали файлът съществува на този път.
        {                                                                                            // Начало на if блока.
            m_config = YAML::LoadFile(buildPath.string());                                           // Зарежда YAML файла.
            return true;                                                                             // Връща true, ако е успешно.
        } // Край на if блока.
        return false; // Връща false, ако файлът не е намерен на нито един от пътищата.
    } // Край на try блока.
    catch (const YAML::Exception &e)                                       // Прихваща изключение от тип YAML::Exception.
    {                                                                      // Начало на catch блока.
        wxMessageBox("Error loading configuration: " + wxString(e.what()), // Показва съобщение за грешка със съдържанието на изключението.
                     "Error", wxOK | wxICON_ERROR);                        // Заглавие, бутон OK, икона за грешка.
        return false;                                                      // Връща false, тъй като зареждането е неуспешно.
    } // Край на catch блока.
} // Край на тялото на метода.

bool MainFrame::SearchSquashFS(const wxString &isoPath, wxArrayString &foundFiles)      // Метод за търсене на SquashFS файлове в ISO файл.
{                                                                                       // Начало на тялото на метода.
    try                                                                                 // Започва блок за обработка на изключения.
    {                                                                                   // Начало на try блока.
        ISOReader reader(isoPath.ToStdString());                                        // Създава обект ISOReader с пътя до ISO файла.
        if (!reader.open())                                                             // Опитва да отвори ISO файла и проверява дали е неуспешно.
        {                                                                               // Начало на if блока.
            wxMessageBox("Failed to open ISO file: " + wxString(reader.getLastError()), // Показва съобщение за грешка.
                         "Error", wxOK | wxICON_ERROR);                                 // Заглавие, бутон OK, икона за грешка.
            return false;                                                               // Връща false.
        } // Край на if блока.

        auto files = reader.listFiles(); // Взема списък с всички файлове в ISO-то.
        bool found = false;              // Флаг, указващ дали са намерени файлове.

        for (const auto &file : files)                                // Обхожда всеки файл в списъка.
        {                                                             // Начало на for цикъла.
            wxString wxFile = wxString::FromUTF8(file);               // Преобразува името на файла от UTF8 на wxString.
            wxString extension = wxFileName(wxFile).GetExt().Lower(); // Взема разширението на файла и го преобразува в малки букви.

            if (extension == "squashfs" || extension == "sfs" || extension == "img" || extension == "cramfs") // Проверява дали разширението съответства на известен тип файлова система.
            {                                                                                                 // Начало на if блока.
                foundFiles.Add(wxFile);                                                                       // Добавя намереното име на файла към списъка.
                found = true;                                                                                 // Задава флага, че е намерен поне един файл.
            } // Край на if блока.
        } // Край на for цикъла.

        return found; // Връща true, ако са намерени файлове, в противен случай false.
    } // Край на try блока.
    catch (const std::exception &e)                              // Прихваща стандартни изключения.
    {                                                            // Начало на catch блока.
        wxMessageBox("Error reading ISO: " + wxString(e.what()), // Показва съобщение за грешка.
                     "Error", wxOK | wxICON_ERROR);              // Заглавие, бутон OK, икона за грешка.
        return false;                                            // Връща false.
    } // Край на catch блока.
} // Край на тялото на метода.

void MainFrame::OnDetect(wxCommandEvent &event)                   // Обработва събитието при натискане на бутона "Detect".
{                                                                 // Начало на тялото на метода.
    wxString isoPath = m_isoPathCtrl->GetValue();                 // Взема пътя до ISO файла от текстовото поле.
    if (isoPath.IsEmpty())                                        // Проверява дали пътят до ISO файла е празен.
    {                                                             // Начало на if блока.
        wxMessageBox("Please select an ISO file first.", "Error", // Показва съобщение за грешка.
                     wxOK | wxICON_ERROR);                        // Заглавие, бутон OK, икона за грешка.
        return;                                                   // Излиза от метода.
    } // Край на if блока.

    m_distroCtrl->SetValue("Detecting...");          // Задава текст "Detecting..." в полето за дистрибуция.
    m_progressGauge->SetValue(10);                   // Задава прогрес бара на 10%.
    m_statusText->SetLabel("Analyzing ISO file..."); // Задава статус текст.
    Update();                                        // Обновява прозореца, за да се покажат промените.

    wxString distributionName;                                                    // Променлива за съхранение на името на дистрибуцията.
    if (SearchGrubEnv(isoPath, distributionName))                                 // Опитва да засече дистрибуцията чрез grubenv файл.
    {                                                                             // Начало на if блока.
        m_distroCtrl->SetValue(distributionName);                                 // Задава засеченото име на дистрибуцията.
        m_progressGauge->SetValue(100);                                           // Задава прогрес бара на 100%.
        m_statusText->SetLabel("Detection completed successfully using grubenv"); // Задава статус текст.

        if (m_projectNameCtrl->IsEmpty())                  // Проверява дали полето за име на проекта е празно.
        {                                                  // Начало на if блока.
            m_projectNameCtrl->SetValue(distributionName); // Ако е празно, задава името на дистрибуцията като име на проекта.
        } // Край на if блока.
        return; // Излиза от метода, тъй като детекцията е успешна.
    } // Край на if блока.

    wxString releaseContent;                                                        // Променлива за съхранение на съдържанието на release файла.
    if (!SearchReleaseFile(isoPath, releaseContent))                                // Ако grubenv не е намерен, опитва да засече дистрибуцията чрез release файл.
    {                                                                               // Начало на if блока.
        m_distroCtrl->SetValue("Detection failed - No identification files found"); // Задава статус, че детекцията е неуспешна.
        m_progressGauge->SetValue(0);                                               // Задава прогрес бара на 0.
        m_statusText->SetLabel("Failed to find identification files");              // Задава статус текст.
        return;                                                                     // Излиза от метода.
    } // Край на if блока.

    m_progressGauge->SetValue(50);                                 // Задава прогрес бара на 50%.
    m_statusText->SetLabel("Reading distribution information..."); // Задава статус текст.
    Update();                                                      // Обновява прозореца.

    wxString distribution = DetectDistribution(releaseContent);           // Извиква DetectDistribution за да анализира съдържанието на release файла.
    m_distroCtrl->SetValue(distribution);                                 // Задава засечената дистрибуция в полето.
    m_progressGauge->SetValue(100);                                       // Задава прогрес бара на 100%.
    SetStatusText("Detection completed successfully using release file"); // Задава статус текст.

    if (m_projectNameCtrl->IsEmpty())              // Проверява дали полето за име на проекта е празно.
    {                                              // Начало на if блока.
        m_projectNameCtrl->SetValue(distribution); // Ако е празно, задава името на дистрибуцията като име на проекта.
    } // Край на if блока.
} // Край на тялото на метода.

bool MainFrame::SearchReleaseFile(const wxString &isoPath, wxString &releaseContent) // Метод за търсене на release файл в ISO.
{                                                                                    // Начало на тялото на метода.
    try                                                                              // Започва блок за обработка на изключения.
    {                                                                                // Начало на try блока.
        ISOReader reader(isoPath.ToStdString());                                     // Създава обект ISOReader.
        if (!reader.open())                                                          // Опитва да отвори ISO файла.
            return false;                                                            // Връща false, ако е неуспешно.

        const auto &releasePaths = m_config["release_paths"];                  // Взема списък с предварително дефинирани пътища за release файлове от конфигурацията.
        if (releasePaths && releasePaths.IsSequence())                         // Проверява дали има дефинирани пътища и дали са списък.
        {                                                                      // Начало на if блока.
            for (const auto &pathNode : releasePaths)                          // Обхожда всеки път в списъка.
            {                                                                  // Начало на for цикъла.
                std::vector<char> content;                                     // Вектор за съхранение на съдържанието на файла.
                std::string path = pathNode.as<std::string>();                 // Преобразува пътя в std::string.
                if (reader.readFile(path, content))                            // Опитва да прочете файла по дадения път.
                {                                                              // Начало на if блока.
                    releaseContent = wxString(content.data(), content.size()); // Преобразува съдържанието във wxString.
                    return true;                                               // Връща true, ако файлът е намерен и прочетен.
                } // Край на if блока.
            } // Край на for цикъла.
        } // Край на if блока.

        auto files = reader.listFiles();                                                      // Взема списък с всички файлове в ISO-то.
        for (const auto &file : files)                                                        // Обхожда всеки файл.
        {                                                                                     // Начало на for цикъла.
            std::string lowerFile = file;                                                     // Копира името на файла.
            std::transform(lowerFile.begin(), lowerFile.end(), lowerFile.begin(), ::tolower); // Преобразува името на файла в малки букви.

            if (lowerFile.find("release") != std::string::npos ||              // Проверява дали името на файла съдържа "release".
                lowerFile.find("version") != std::string::npos)                // Или дали съдържа "version".
            {                                                                  // Начало на if блока.
                std::vector<char> content;                                     // Вектор за съдържанието.
                if (reader.readFile(file, content))                            // Опитва да прочете файла.
                {                                                              // Начало на if блока.
                    releaseContent = wxString(content.data(), content.size()); // Преобразува съдържанието във wxString.
                    return true;                                               // Връща true, ако файлът е намерен и прочетен.
                } // Край на if блока.
            } // Край на if блока.
        } // Край на for цикъла.
        return false; // Връща false, ако не е намерен release файл.
    } // Край на try блока.
    catch (const std::exception &e)                                                             // Прихваща стандартни изключения.
    {                                                                                           // Начало на catch блока.
        wxMessageBox("Error reading ISO: " + wxString(e.what()), "Error", wxOK | wxICON_ERROR); // Показва съобщение за грешка.
        return false;                                                                           // Връща false.
    } // Край на catch блока.
} // Край на тялото на метода.

bool MainFrame::SearchGrubEnv(const wxString &isoPath, wxString &distributionName) // Метод за търсене на grubenv файл в ISO.
{                                                                                  // Начало на тялото на метода.
    try                                                                            // Започва блок за обработка на изключения.
    {                                                                              // Начало на try блока.
        ISOReader reader(isoPath.ToStdString());                                   // Създава обект ISOReader.
        if (!reader.open())                                                        // Опитва да отвори ISO файла.
            return false;                                                          // Връща false, ако е неуспешно.

        const auto &grubenvPaths = m_config["grubenv_paths"];              // Взема списък с предварително дефинирани пътища за grubenv файлове от конфигурацията.
        if (grubenvPaths && grubenvPaths.IsSequence())                     // Проверява дали има дефинирани пътища и дали са списък.
        {                                                                  // Начало на if блока.
            for (const auto &pathNode : grubenvPaths)                      // Обхожда всеки път в списъка.
            {                                                              // Начало на for цикъла.
                std::vector<char> content;                                 // Вектор за съхранение на съдържанието.
                std::string path = pathNode.as<std::string>();             // Преобразува пътя в std::string.
                if (reader.readFile(path, content))                        // Опитва да прочете файла по дадения път.
                {                                                          // Начало на if блока.
                    wxString envContent(content.data(), content.size());   // Преобразува съдържанието във wxString.
                    distributionName = ExtractNameFromGrubEnv(envContent); // Извлича името на дистрибуцията от съдържанието.
                    if (!distributionName.IsEmpty())                       // Проверява дали е извлечено име.
                        return true;                                       // Връща true, ако е успешно.
                } // Край на if блока.
            } // Край на for цикъла.
        } // Край на if блока.

        auto files = reader.listFiles();                                                      // Взема списък с всички файлове в ISO-то.
        for (const auto &file : files)                                                        // Обхожда всеки файл.
        {                                                                                     // Начало на for цикъла.
            std::string lowerFile = file;                                                     // Копира името на файла.
            std::transform(lowerFile.begin(), lowerFile.end(), lowerFile.begin(), ::tolower); // Преобразува името на файла в малки букви.

            if (lowerFile.find("grubenv") != std::string::npos)            // Проверява дали името на файла съдържа "grubenv".
            {                                                              // Начало на if блока.
                std::vector<char> content;                                 // Вектор за съдържанието.
                if (reader.readFile(file, content))                        // Опитва да прочете файла.
                {                                                          // Начало на if блока.
                    wxString envContent(content.data(), content.size());   // Преобразува съдържанието във wxString.
                    distributionName = ExtractNameFromGrubEnv(envContent); // Извлича името на дистрибуцията от съдържанието.
                    if (!distributionName.IsEmpty())                       // Проверява дали е извлечено име.
                        return true;                                       // Връща true, ако е успешно.
                } // Край на if блока.
            } // Край на if блока.
        } // Край на for цикъла.
        return false; // Връща false, ако не е намерен grubenv файл.
    } // Край на try блока.
    catch (const std::exception &e)                                                             // Прихваща стандартни изключения.
    {                                                                                           // Начало на catch блока.
        wxMessageBox("Error reading ISO: " + wxString(e.what()), "Error", wxOK | wxICON_ERROR); // Показва съобщение за грешка.
        return false;                                                                           // Връща false.
    } // Край на catch блока.
} // Край на тялото на метода.

wxString MainFrame::ExtractNameFromGrubEnv(const wxString &content) // Метод за извличане на име на дистрибуция от съдържанието на grubenv файл.
{                                                                   // Начало на тялото на метода.
    wxString name;                                                  // Променлива за името на дистрибуцията.
    wxString version;                                               // Променлива за версията на дистрибуцията.

    wxStringTokenizer tokenizer(content, "\n");                          // Създава токенизатор, който разбива съдържанието по нов ред.
    while (tokenizer.HasMoreTokens())                                    // Докато има още токени (редове).
    {                                                                    // Начало на while цикъла.
        wxString line = tokenizer.GetNextToken().Trim(true).Trim(false); // Взема следващия ред и премахва водещи/крайни интервали.

        if (line.StartsWith("NAME=")) // Проверява дали редът започва с "NAME=".
        {                             // Начало на if блока.
            name = line.Mid(5);       // Взема частта след "NAME=".
            name.Replace("\"", "");   // Премахва кавичките.
        } // Край на if блока.
        else if (line.StartsWith("VERSION=")) // Проверява дали редът започва с "VERSION=".
        {                                     // Начало на else if блока.
            version = line.Mid(8);            // Взема частта след "VERSION=".
            version.Replace("\"", "");        // Премахва кавичките.
        } // Край на else if блока.
    } // Край на while цикъла.

    if (!name.IsEmpty() && !version.IsEmpty()) // Ако са намерени и име, и версия.
    {                                          // Начало на if блока.
        return name + " " + version;           // Връща името и версията.
    } // Край на if блока.
    else if (!name.IsEmpty()) // Ако е намерено само име.
    {                         // Начало на else if блока.
        return name;          // Връща само името.
    } // Край на else if блока.
    return ""; // Връща празен низ, ако нищо не е намерено.
} // Край на тялото на метода.

wxString MainFrame::DetectDistribution(const wxString &releaseContent) // Метод за детекция на дистрибуция от съдържанието на release файл.
{                                                                      // Начало на тялото на метода.
    wxString lowerContent = releaseContent.Lower();                    // Преобразува съдържанието в малки букви за сравнение.

    const auto &distributions = m_config["distributions"]; // Взема списък с дефинирани дистрибуции от конфигурацията.
    if (distributions && distributions.IsSequence())       // Проверява дали има дефинирани дистрибуции и дали са списък.
    {                                                      // Начало на if блока.
        for (const auto &distro : distributions)           // Обхожда всяка дистрибуция в списъка.
        {                                                  // Начало на for цикъла.
            if (!distro["pattern"] || !distro["name"])     // Проверява дали дистрибуцията има дефиниран "pattern" и "name".
                continue;                                  // Преминава към следващата дистрибуция, ако липсват.

            wxString pattern = wxString::FromUTF8(distro["pattern"].as<std::string>()); // Взема шаблона за търсене.
            if (lowerContent.Contains(pattern))                                         // Проверява дали съдържанието съдържа шаблона.
            {                                                                           // Начало на if блока.
                wxString name = wxString::FromUTF8(distro["name"].as<std::string>());   // Взема името на дистрибуцията.

                wxString version;                                            // Променлива за версията.
                int versionPos = lowerContent.Find("version");               // Търси позицията на подниза "version".
                if (versionPos != wxNOT_FOUND)                               // Ако "version" е намерен.
                {                                                            // Начало на if блока.
                    wxString remaining = releaseContent.Mid(versionPos + 7); // Взема останалата част от реда след "version" и седем символа (за да пропусне "=1.2.3").
                    for (size_t i = 0; i < remaining.Length(); i++)          // Обхожда символите в останалата част.
                    {                                                        // Начало на for цикъла.
                        if (isdigit(remaining[i]) || remaining[i] == '.')    // Проверява дали символът е цифра или точка.
                        {                                                    // Начало на if блока.
                            version += remaining[i];                         // Добавя символа към версията.
                        } // Край на if блока.
                        else if (!version.IsEmpty()) // Ако вече е събрана част от версията и текущият символ не е цифра/точка.
                        {                            // Начало на else if блока.
                            break;                   // Прекъсва цикъла (версията е завършена).
                        } // Край на else if блока.
                    } // Край на for цикъла.
                } // Край на if блока.
                return version.IsEmpty() ? name : name + " " + version; // Връща името (и версията, ако е намерена).
            } // Край на if блока.
        } // Край на for цикъла.
    } // Край на if блока.
    return "Unknown Distribution"; // Връща "Unknown Distribution", ако не е намерена съвпадаща дистрибуция.
} // Край на тялото на метода.

void MainFrame::OnGUIDetected(wxCommandEvent &event) // Обработва персонализирано събитие за засечен GUI.
{                                                    // Начало на тялото на метода.
    wxString guiName = event.GetString();            // Взема името на GUI (графичната среда) от събитието.
    if (m_desktopTab)                                // Проверява дали обектът m_desktopTab е валиден.
    {                                                // Начало на if блока.
        m_desktopTab->UpdateGUILabel(guiName);       // Актуализира етикета за GUI в DesktopTab (трябва да има такъв метод).
    } // Край на if блока.
} // Край на тялото на метода.