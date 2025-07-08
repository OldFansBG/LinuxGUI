#include "ConfigTab.h"    // Включва заглавния файл ConfigTab.h
#include "FlatpakStore.h" // Включва заглавния файл FlatpakStore.h
#include "DesktopTab.h"   // Включва заглавния файл DesktopTab.h
#include "CustomizeTab.h" // Включва заглавния файл CustomizeTab.h // Add new header
#include "SecondWindow.h" // Включва заглавния файл SecondWindow.h // Added to provide full definition of SecondWindow
#include "SystemTab.h"    // Включва заглавния файл SystemTab.h // Add this include at the top with other includes

wxBEGIN_EVENT_TABLE(SQLTab, wxPanel)                            // Начало на таблицата със събития за SQLTab, наследяващ wxPanel
    EVT_BUTTON(ID_CONFIG_DESKTOP, SQLTab::OnConfigTabChanged)   // Декларира събитие за бутон ID_CONFIG_DESKTOP, извиква SQLTab::OnConfigTabChanged
    EVT_BUTTON(ID_CONFIG_APPS, SQLTab::OnConfigTabChanged)      // Декларира събитие за бутон ID_CONFIG_APPS, извиква SQLTab::OnConfigTabChanged
    EVT_BUTTON(ID_CONFIG_SYSTEM, SQLTab::OnConfigTabChanged)    // Декларира събитие за бутон ID_CONFIG_SYSTEM, извиква SQLTab::OnConfigTabChanged
    EVT_BUTTON(ID_CONFIG_CUSTOMIZE, SQLTab::OnConfigTabChanged) // Декларира събитие за бутон ID_CONFIG_CUSTOMIZE, извиква SQLTab::OnConfigTabChanged
    wxEND_EVENT_TABLE()                                         // Край на таблицата със събития

    SQLTab::SQLTab(wxWindow *parent, const wxString &workDir) // Конструктор на SQLTab, приема указател към родителски прозорец и работна директория
    : wxPanel(parent), m_workDir(workDir)                     // Инициализира базовия клас wxPanel с родител и член-променливата m_workDir с подадената работна директория
{                                                             // Начало на тялото на конструктора
    wxBoxSizer *contentSizer = new wxBoxSizer(wxVERTICAL);    // Създава нов вертикален wxBoxSizer за организиране на съдържанието

    wxPanel *tabPanel = new wxPanel(this);               // Създава нов wxPanel (панел за табове) като дъщерен на текущия SQLTab обект
    tabPanel->SetBackgroundColour(wxColour(26, 26, 26)); // Задава цвета на фона на панела за табове на тъмно сиво (RGB: 26, 26, 26)
    wxBoxSizer *tabSizer = new wxBoxSizer(wxHORIZONTAL); // Създава нов хоризонтален wxBoxSizer за подреждане на бутоните на табовете

    const struct                                     // Декларира константна анонимна структура
    {                                                // Начало на дефиницията на структурата
        wxString label;                              // Член на структурата: етикет (текст) на таба
        int id;                                      // Член на структурата: идентификатор (ID) на таба
    } tabs[] = {                                     // Инициализира масив от обекти от горната структура, дефиниращ табовете
                {"Desktop", ID_CONFIG_DESKTOP},      // Първи таб: етикет "Desktop", ID_CONFIG_DESKTOP
                {"Applications", ID_CONFIG_APPS},    // Втори таб: етикет "Applications", ID_CONFIG_APPS
                {"System", ID_CONFIG_SYSTEM},        // Трети таб: етикет "System", ID_CONFIG_SYSTEM
                {"Customize", ID_CONFIG_CUSTOMIZE}}; // Четвърти таб: етикет "Customize", ID_CONFIG_CUSTOMIZE

    m_configTabButtons.clear(); // Изчиства вектора m_configTabButtons, премахвайки всички указатели към бутони, които може да е съдържал преди

    for (const auto &tab : tabs)                                               // Цикъл for-each, който итерира през всеки елемент (наречен 'tab') в масива 'tabs'
    {                                                                          // Начало на тялото на цикъла
        wxButton *tabButton = new wxButton(tabPanel, tab.id, tab.label,        // Създава нов бутон, като родител е tabPanel, с ID и етикет от текущия елемент 'tab'
                                           wxDefaultPosition, wxSize(120, 50), // Задава позиция по подразбиране и размер на бутона 120x50 пиксела
                                           wxBORDER_NONE);                     // Задава стил на рамката wxBORDER_NONE (без видима рамка)
        tabButton->SetBackgroundColour(wxColour(26, 26, 26));                  // Задава цвета на фона на бутона на тъмно сиво
        tabButton->SetForegroundColour(wxColour(156, 163, 175));               // Задава цвета на текста (предния план) на бутона на светло сиво
        tabSizer->Add(tabButton, 0, wxEXPAND);                                 // Добавя бутона към tabSizer, като му позволява да се разширява хоризонтално, без да заема пропорционално повече място

        tabButton->Bind(wxEVT_BUTTON, &SQLTab::OnConfigTabChanged, this); // Свързва събитието за натискане на бутона (wxEVT_BUTTON) с метода OnConfigTabChanged на текущия SQLTab обект
        m_configTabButtons.push_back(tabButton);                          // Добавя указателя към новосъздадения бутон в края на вектора m_configTabButtons
    } // Край на тялото на цикъла

    tabPanel->SetSizer(tabSizer);             // Задава tabSizer като мениджър на оформлението за tabPanel
    contentSizer->Add(tabPanel, 0, wxEXPAND); // Добавя tabPanel към contentSizer, като му позволява да се разширява хоризонтално, но не и вертикално пропорционално

    m_configContent = new wxPanel(this);                         // Създава нов wxPanel (панел за съдържанието на конфигурацията) като дъщерен на текущия SQLTab обект
    m_configContent->SetBackgroundColour(wxColour(17, 24, 39));  // Задава цвета на фона на панела за съдържание на много тъмно синьо-сиво
    contentSizer->Add(m_configContent, 1, wxEXPAND | wxALL, 10); // Добавя m_configContent към contentSizer, позволявайки му да се разширява и запълва наличното пространство, с отстъп 10 пиксела от всички страни

    this->SetSizer(contentSizer); // Задава contentSizer като основен мениджър на оформлението за текущия SQLTab панел

    m_currentConfigTab = ID_CONFIG_DESKTOP; // Инициализира член-променливата m_currentConfigTab с ID-то на "Desktop" таба
    CreateDesktopTab();                     // Извиква метода CreateDesktopTab за създаване и показване на съдържанието на "Desktop" таба по подразбиране
} // Край на тялото на конструктора

void SQLTab::ShowTab(int tabId) // Дефиниция на метода ShowTab, който приема ID на таб като аргумент
{                               // Начало на тялото на метода ShowTab
    // Унищожава всички съществуващи дъщерни елементи на m_configContent, за да се предотвратят конфликти
    m_configContent->DestroyChildren(); // Извиква метода DestroyChildren() на m_configContent, за да премахне всички негови дъщерни контроли

    // Изчиства и изтрива стария sizer, ако има такъв, за да се избегнат проблеми с повторното му използване
    if (m_configContent->GetSizer())              // Проверява дали m_configContent има текущо зададен sizer
    {                                             // Начало на if блока
        m_configContent->SetSizer(nullptr, true); // Задава sizer-а на m_configContent на nullptr и указва, че старият sizer трябва да бъде изтрит (вторият аргумент 'true')
    } // Край на if блока

    // Обновява ID-то на текущия таб
    m_currentConfigTab = tabId; // Задава член-променливата m_currentConfigTab на новото ID на таба

    // Превключва към съответния таб и създава неговото съдържание
    switch (tabId)          // Начало на switch-case конструкция, базирана на стойността на tabId
    {                       // Начало на тялото на switch
    case ID_CONFIG_DESKTOP: // Ако tabId е ID_CONFIG_DESKTOP
        CreateDesktopTab(); // Извиква метода CreateDesktopTab за създаване на съдържанието на "Desktop" таба
        // Предизвиква събитие за размер, за да наложи преизчисляване на оформлението
        if (!m_configContent->GetChildren().IsEmpty())                                   // Проверява дали m_configContent има поне един дъщерен елемент (т.е. дали CreateDesktopTab е добавил нещо)
        {                                                                                // Начало на if блока
            wxWindow *desktopTab = m_configContent->GetChildren().GetFirst()->GetData(); // Взема указател към първия дъщерен елемент на m_configContent
            wxSizeEvent event(desktopTab->GetSize());                                    // Създава ново събитие от тип wxSizeEvent с текущия размер на desktopTab
            desktopTab->GetEventHandler()->ProcessEvent(event);                          // Ръчно изпраща събитието за промяна на размера към обработчика на събития на desktopTab
        } // Край на if блока
        break; // Прекратява изпълнението на switch-case

    case ID_CONFIG_APPS: // Ако tabId е ID_CONFIG_APPS
        CreateAppsTab(); // Извиква метода CreateAppsTab за създаване на съдържанието на "Applications" таба
        break;           // Прекратява изпълнението на switch-case

    case ID_CONFIG_SYSTEM: // Ако tabId е ID_CONFIG_SYSTEM
        CreateSystemTab(); // Извиква метода CreateSystemTab за създаване на съдържанието на "System" таба
        break;             // Прекратява изпълнението на switch-case

    case ID_CONFIG_CUSTOMIZE: // Ако tabId е ID_CONFIG_CUSTOMIZE
        CreateCustomizeTab(); // Извиква метода CreateCustomizeTab за създаване на съдържанието на "Customize" таба
        break;                // Прекратява изпълнението на switch-case

    default:                                     // Ако tabId не съвпада с нито един от горните случаи
        wxLogDebug("Unknown tab ID: %d", tabId); // Записва съобщение в дебъг лога за неизвестно ID на таб
        break;                                   // Прекратява изпълнението на switch-case
    } // Край на тялото на switch

    // Гарантира, че оформлението е обновено след добавяне на ново съдържание
    m_configContent->Layout(); // Преизчислява и прилага оформлението на m_configContent и неговите дъщерни елементи
} // Край на тялото на метода ShowTab

void SQLTab::OnConfigTabChanged(wxCommandEvent &event)      // Дефиниция на метода OnConfigTabChanged, който обработва събитие от натискане на бутон за таб
{                                                           // Начало на тялото на метода OnConfigTabChanged
    for (wxButton *btn : m_configTabButtons)                // Итерира през всеки указател към wxButton в вектора m_configTabButtons
    {                                                       // Начало на for цикъла
        if (btn->GetId() == event.GetId())                  // Проверява дали ID-то на текущия бутон 'btn' съвпада с ID-то на бутона, предизвикал събитието
        {                                                   // Ако ID-тата съвпадат (това е натиснатият бутон)
            btn->SetBackgroundColour(wxColour(44, 49, 58)); // Задава цвета на фона на натиснатия бутон на по-светло сиво (активен вид)
            btn->SetForegroundColour(*wxWHITE);             // Задава цвета на текста на натиснатия бутон на бял
        } // Край на if блока
        else                                                   // Ако ID-тата не съвпадат (това е някой от останалите, неактивни бутони)
        {                                                      // Начало на else блока
            btn->SetBackgroundColour(wxColour(26, 26, 26));    // Задава цвета на фона на неактивния бутон на тъмно сиво (неактивен вид)
            btn->SetForegroundColour(wxColour(156, 163, 175)); // Задава цвета на текста на неактивния бутон на светло сиво
        } // Край на else блока
        btn->Refresh(); // Обновява бутона, за да се приложат визуалните промени
    } // Край на for цикъла

    ShowTab(event.GetId()); // Извиква метода ShowTab с ID-то на натиснатия бутон, за да се покаже съдържанието на съответния таб
} // Край на тялото на метода OnConfigTabChanged

void SQLTab::CreateDesktopTab()                               // Дефиниция на метода CreateDesktopTab за създаване на съдържанието на "Desktop" таба
{                                                             // Начало на тялото на метода CreateDesktopTab
    DesktopTab *desktopTab = new DesktopTab(m_configContent); // Създава нов обект DesktopTab, като родител е m_configContent
    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);           // Създава нов вертикален wxBoxSizer
    sizer->Add(desktopTab, 1, wxEXPAND);                      // Добавя desktopTab към sizer-а, като му позволява да се разширява и запълва вертикалното пространство
    m_configContent->SetSizer(sizer);                         // Задава новосъздадения sizer като мениджър на оформлението за m_configContent

    // Налага незабавно преизчисляване на оформлението
    m_configContent->Layout(); // Преизчислява и прилага оформлението на m_configContent

    // Използва CallAfter, за да гарантира правилно оразмеряване след прилагане на оформлението
    CallAfter([this, desktopTab]()                                                      // Планира изпълнението на следната ламбда функция след приключване на текущата обработка на събития
              {                                                                         // Начало на тялото на ламбда функцията
                  desktopTab->RecalculateLayout(m_configContent->GetSize().GetWidth()); // Извиква метода RecalculateLayout на desktopTab, предавайки текущата ширина на m_configContent
                  m_configContent->Layout();
              }); // Отново преизчислява оформлението на m_configContent, за да отрази евентуални промени от RecalculateLayout
} // Край на тялото на метода CreateDesktopTab

void SQLTab::CreateAppsTab() // Дефиниция на метода CreateAppsTab за създаване на съдържанието на "Applications" таба
{                            // Начало на тялото на метода CreateAppsTab
    // Създава нов вертикален sizer за съдържанието на таба
    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL); // Създава нов вертикален wxBoxSizer

    // Създава нов екземпляр на FlatpakStore с m_configContent като негов родител
    FlatpakStore *flatpakStore = new FlatpakStore(m_configContent, m_workDir); // Създава нов обект FlatpakStore, като родител е m_configContent, и предава работната директория m_workDir
    wxLogDebug("Created new FlatpakStore for Apps tab");                       // Записва съобщение в дебъг лога за създаването на FlatpakStore

    // Добавя панела FlatpakStore към sizer-а с разширение и отстъп
    sizer->Add(flatpakStore, 1, wxEXPAND | wxALL, 10); // Добавя flatpakStore към sizer-а, позволявайки му да се разширява и запълва пространството, с отстъп 10 пиксела от всички страни

    // Задава sizer-а на m_configContent и гарантира правилното оформление
    m_configContent->SetSizer(sizer); // Задава новосъздадения sizer като мениджър на оформлението за m_configContent
    m_configContent->Layout();        // Преизчислява и прилага оформлението на m_configContent

    // По избор: Проверка на йерархията на родителите за отстраняване на грешки
    wxLogDebug("Parent of FlatpakStore: %p, m_configContent: %p", flatpakStore->GetParent(), m_configContent); // Записва в дебъг лога указателите към родителя на flatpakStore и към самия m_configContent
} // Край на тялото на метода CreateAppsTab

void SQLTab::SetContainerId(const wxString &containerId) // Дефиниция на метода SetContainerId, който задава ID-то на контейнера
{                                                        // Начало на тялото на метода SetContainerId
    m_containerId = containerId;                         // Присвоява подадената стойност containerId на член-променливата m_containerId
} // Край на тялото на метода SetContainerId

void SQLTab::CreateSystemTab()                             // Дефиниция на метода CreateSystemTab за създаване на съдържанието на "System" таба
{                                                          // Начало на тялото на метода CreateSystemTab
    SystemTab *systemTab = new SystemTab(m_configContent); // Създава нов обект SystemTab, като родител е m_configContent
    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);        // Създава нов вертикален wxBoxSizer
    sizer->Add(systemTab, 1, wxEXPAND);                    // Добавя systemTab към sizer-а, позволявайки му да се разширява и запълва вертикалното пространство
    m_configContent->SetSizer(sizer);                      // Задава новосъздадения sizer като мениджър на оформлението за m_configContent
    m_configContent->Layout();                             // Преизчислява и прилага оформлението на m_configContent
} // Край на тялото на метода CreateSystemTab

void SQLTab::CreateCustomizeTab()                                   // Дефиниция на метода CreateCustomizeTab за създаване на съдържанието на "Customize" таба
{                                                                   // Начало на тялото на метода CreateCustomizeTab
    CustomizeTab *customizeTab = new CustomizeTab(m_configContent); // Създава нов обект CustomizeTab, като родител е m_configContent
    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);                 // Създава нов вертикален wxBoxSizer
    sizer->Add(customizeTab, 1, wxEXPAND);                          // Добавя customizeTab към sizer-а, позволявайки му да се разширява и запълва вертикалното пространство
    m_configContent->SetSizer(sizer);                               // Задава новосъздадения sizer като мениджър на оформлението за m_configContent
    m_configContent->Layout();                                      // Преизчислява и прилага оформлението на m_configContent
    customizeTab->LoadWallpaper();                                  // Зарежда тапета след създаване на таба // Извиква метода LoadWallpaper на customizeTab, за да зареди тапета
} // Край на тялото на метода CreateCustomizeTab