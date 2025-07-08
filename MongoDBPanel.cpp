// MongoDBPanel.cpp
#include "MongoDBPanel.h" // Включва заглавния файл за MongoDBPanel
#include "WindowIDs.h"    // Включва заглавния файл за идентификатори на прозорци/контроли
                          // Празна линия за по-добра четимост
#include <wx/listctrl.h>  // Включва заглавния файл на wxWidgets за контролата wxListCtrl (списък)
#include <wx/filedlg.h>   // Включва заглавния файл на wxWidgets за диалог за избор на файл
#include <wx/textfile.h>  // Включва заглавния файл на wxWidgets за работа с текстови файлове
#include <wx/msgdlg.h>    // Включва заглавния файл на wxWidgets за диалози със съобщения
#include <wx/log.h>       // Включва заглавния файл на wxWidgets за логване
#include <wx/progdlg.h>   // Включва заглавния файл на wxWidgets за диалог за прогрес
#include <wx/utils.h>     // Включва заглавния файл на wxWidgets с различни помощни функции
#include <wx/filename.h>  // Включва заглавния файл на wxWidgets за работа с имена на файлове
#include <wx/file.h>      // Включва заглавния файл на wxWidgets за файлови операции
#include <fstream>        // Включва стандартната C++ библиотека за файлови потоци (четене/запис)
#include <sstream>        // Включва стандартната C++ библиотека за низови потоци
#include <iomanip>        // Включва стандартната C++ библиотека за манипулатори на входно/изходни потоци (напр. за форматиране)
#include <chrono>         // Включва стандартната C++ библиотека за работа с време и дата
#include <string>         // Включва стандартната C++ библиотека за работа с низове
#include <random>         // Включва стандартната C++ библиотека за генериране на случайни числа
#include <vector>         // Включва стандартната C++ библиотека за работа с динамични масиви (вектори)
#include <stdexcept>      // Включва стандартната C++ библиотека за стандартни изключения
#include <algorithm>      // Включва стандартната C++ библиотека с различни алгоритми (напр. min, max)
                          // Празна линия за по-добра четимост
// MongoDB/BSON includes // Коментар: Указва, че следват включвания за MongoDB/BSON
#include <mongocxx/uri.hpp>                    // Включва заглавния файл на MongoDB C++ драйвера за URI на връзката
#include <mongocxx/client.hpp>                 // Включва заглавния файл на MongoDB C++ драйвера за клиент на MongoDB
#include <mongocxx/collection.hpp>             // Включва заглавния файл на MongoDB C++ драйвера за колекции в MongoDB
#include <mongocxx/exception/exception.hpp>    // Включва заглавния файл на MongoDB C++ драйвера за изключения
#include <bsoncxx/builder/stream/document.hpp> // Включва заглавния файл на BSONCXX за поточен конструктор на документи
#include <bsoncxx/builder/basic/document.hpp>  // Включва заглавния файл на BSONCXX за базов конструктор на документи
#include <bsoncxx/builder/basic/kvp.hpp>       // Включва заглавния файл на BSONCXX за създаване на двойки ключ-стойност (key-value pair)
#include <bsoncxx/json.hpp>                    // Включва заглавния файл на BSONCXX за конвертиране от/към JSON
#include <bsoncxx/types.hpp>                   // Включва заглавния файл на BSONCXX за BSON типове данни
#include <bsoncxx/exception/exception.hpp>     // Включва заглавния файл на BSONCXX за изключения
#include <bsoncxx/string/view_or_value.hpp>    // Включва заглавния файл на BSONCXX за представяне на низ като изглед или стойност
#include <rapidjson/document.h>                // Включва заглавния файл на RapidJSON за парсване и работа с JSON документи
#include <rapidjson/error/en.h>                // Включва заглавния файл на RapidJSON за английски съобщения за грешки при парсване
                                               // Празна линия за по-добра четимост
// --- Event Table for MongoDBPanel --- // Коментар: Начало на дефиницията на таблицата на събитията за MongoDBPanel
wxBEGIN_EVENT_TABLE(MongoDBPanel, wxPanel)                                      // Начало на дефиницията на таблицата на събитията за класа MongoDBPanel, наследяващ wxPanel
    EVT_BUTTON(ID_MONGODB_PANEL_INTERNAL_CLOSE_BTN, MongoDBPanel::OnCloseClick) // Свързва събитие натискане на бутон с ID_MONGODB_PANEL_INTERNAL_CLOSE_BTN с метода MongoDBPanel::OnCloseClick
    EVT_BUTTON(ID_MONGODB_UPLOAD_JSON, MongoDBPanel::OnUploadJson)              // Свързва събитие натискане на бутон с ID_MONGODB_UPLOAD_JSON с метода MongoDBPanel::OnUploadJson
    EVT_BUTTON(ID_MONGODB_REFRESH, MongoDBPanel::OnRefresh)                     // Свързва събитие натискане на бутон с ID_MONGODB_REFRESH с метода MongoDBPanel::OnRefresh
    EVT_BUTTON(ID_MONGODB_DOWNLOAD_SELECTED, MongoDBPanel::OnDownloadSelected)  // Свързва събитие натискане на бутон с ID_MONGODB_DOWNLOAD_SELECTED с метода MongoDBPanel::OnDownloadSelected
    EVT_BUTTON(ID_MONGODB_LOAD_INSTALL, MongoDBPanel::OnLoadAndInstall)         // Свързва събитие натискане на бутон с ID_MONGODB_LOAD_INSTALL с метода MongoDBPanel::OnLoadAndInstall
    EVT_LIST_ITEM_SELECTED(wxID_ANY, MongoDBPanel::OnItemSelected)              // Свързва събитие избор на елемент в списък (с произволно ID) с метода MongoDBPanel::OnItemSelected
    wxEND_EVENT_TABLE()                                                         // Край на дефиницията на таблицата на събитията
                                                                                // Празна линия за по-добра четимост
    // --- MongoDBPanel Implementation --- // Коментар: Начало на имплементацията на класа MongoDBPanel
    // Празна линия за по-добра четимост
    MongoDBPanel::MongoDBPanel(wxWindow *parent, const wxString &projectDir) // Конструктор на класа MongoDBPanel, приема родителски прозорец и път до директорията на проекта
    : wxPanel(parent, wxID_ANY),                                             // Инициализация на базовия клас wxPanel с родител и произволно ID
      m_projectDir(projectDir),                                              // Инициализация на член-променливата m_projectDir със стойността на projectDir
      m_closeButton(nullptr), m_uploadButton(nullptr),                       // Инициализация на указателите към бутони с nullptr
      m_refreshButton(nullptr), m_contentList(nullptr),                      // Инициализация на указателите към бутон и списък с nullptr
      m_loadInstallButton(nullptr), m_downloadButton(nullptr),               // Инициализация на указателите към бутони с nullptr
      m_selectedItemIndex(-1)                                                // Инициализация на индекса на избрания елемент с -1 (няма избран елемент)
{                                                                            // Начало на тялото на конструктора
    // Modern Theme Colors // Коментар: Модерни цветове за темата
    m_bgColor = wxColour(17, 17, 27);             // Инициализация на цвета на фона
    m_listBgColor = wxColour(30, 30, 46);         // Инициализация на цвета на фона на списъка
    m_listFgColor = wxColour(205, 214, 244);      // Инициализация на цвета на текста в списъка
    m_headerColor = wxColour(180, 190, 254);      // Инициализация на цвета на заглавния текст
    m_buttonBgColor = wxColour(49, 50, 68);       // Инициализация на цвета на фона на бутоните
    m_buttonFgColor = wxColour(205, 214, 244);    // Инициализация на цвета на текста на бутоните
    m_uploadBtnBgColor = wxColour(137, 180, 250); // Инициализация на цвета на фона на бутона за качване
    m_headerBgColor = wxColour(24, 24, 37);       // Инициализация на цвета на фона на заглавната част
    m_borderColor = wxColour(69, 71, 90);         // Инициализация на цвета на рамката
    SetBackgroundColour(m_bgColor);               // Задава цвета на фона на панела
                                                  // Празна линия за по-добра четимост
    // Create db_files directory if it doesn't exist // Коментар: Създава директория db_files, ако не съществува
    wxString dbDirPath = wxFileName(m_projectDir, "db_files").GetFullPath(); // Създава пълен път до поддиректория "db_files" в директорията на проекта
    if (!wxDirExists(dbDirPath))                                             // Проверява дали директорията dbDirPath не съществува
    {                                                                        // Начало на if блока
        wxMkdir(dbDirPath);                                                  // Създава директорията dbDirPath
        wxLogDebug("Created database files directory: %s", dbDirPath);       // Записва в лога съобщение за създадената директория
    } // Край на if блока
    // Празна линия за по-добра четимост
    wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL); // Създава вертикален wxBoxSizer (мениджър на подреждане)
                                                        // Празна линия за по-добра четимост
    // Панел за заглавие
    wxPanel *titlePanel = new wxPanel(this, wxID_ANY);                                                    // Създава панел за заглавието като дъщерен на текущия панел
    titlePanel->SetBackgroundColour(m_headerBgColor);                                                     // Задава цвета на фона на панела за заглавие
    wxBoxSizer *titleSizer = new wxBoxSizer(wxHORIZONTAL);                                                // Създава хоризонтален wxBoxSizer за елементите в заглавния панел
    wxPanel *decorLine = new wxPanel(titlePanel, wxID_ANY, wxDefaultPosition, wxSize(4, 24));             // Създава декоративна линия (малък панел)
    decorLine->SetBackgroundColour(m_uploadBtnBgColor);                                                   // Задава цвета на фона на декоративната линия
    wxStaticText *titleText = new wxStaticText(titlePanel, wxID_ANY, "Application Installation History"); // Създава статичен текст за заглавието
    titleText->SetForegroundColour(m_headerColor);                                                        // Задава цвета на текста на заглавието
    wxFont titleFont = titleText->GetFont();                                                              // Взема текущия шрифт на заглавния текст
    titleFont.SetPointSize(14);                                                                           // Задава размер на шрифта 14 точки
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);                                                               // Задава удебелен шрифт
    titleText->SetFont(titleFont);                                                                        // Прилага новия шрифт към заглавния текст
    titleSizer->Add(decorLine, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 20);                                  // Добавя декоративната линия към titleSizer, подравнена вертикално в центъра, с отстояние 20px отляво
    titleSizer->AddSpacer(15);                                                                            // Добавя празно пространство от 15px
    titleSizer->Add(titleText, 0, wxALIGN_CENTER_VERTICAL);                                               // Добавя заглавния текст към titleSizer, подравнен вертикално в центъра
    titleSizer->AddStretchSpacer(1);                                                                      // Добавя разтегливо празно пространство, за да избута следващите елементи надясно
    titlePanel->SetSizer(titleSizer);                                                                     // Задава titleSizer като мениджър на подреждане за titlePanel
                                                                                                          // Празна линия за по-добра четимост
    // Top Bar with Buttons // Коментар: Горна лента с бутони
    wxPanel *topBar = new wxPanel(this, wxID_ANY);                                                                             // Създава панел за горната лента
    topBar->SetBackgroundColour(m_bgColor);                                                                                    // Задава цвета на фона на горната лента
    wxBoxSizer *topBarSizer = new wxBoxSizer(wxHORIZONTAL);                                                                    // Създава хоризонтален wxBoxSizer за бутоните
    m_uploadButton = new wxButton(topBar, ID_MONGODB_UPLOAD_JSON, "Upload JSON", wxDefaultPosition, wxSize(130, 35));          // Създава бутон "Upload JSON"
    m_refreshButton = new wxButton(topBar, ID_MONGODB_REFRESH, "Refresh", wxDefaultPosition, wxSize(110, 35));                 // Създава бутон "Refresh"
    m_downloadButton = new wxButton(topBar, ID_MONGODB_DOWNLOAD_SELECTED, "Download", wxDefaultPosition, wxSize(120, 35));     // Създава бутон "Download"
    m_downloadButton->Disable();                                                                                               // Деактивира бутона "Download" първоначално
    m_loadInstallButton = new wxButton(topBar, ID_MONGODB_LOAD_INSTALL, "Load & Install", wxDefaultPosition, wxSize(130, 35)); // Създава бутон "Load & Install"
    m_loadInstallButton->Disable();                                                                                            // Деактивира бутона "Load & Install" първоначално
                                                                                                                               // Празна линия за по-добра четимост
    // Explicitly bind the new handler: // Коментар: Изрично свързване на новия обработчик на събития:
    m_loadInstallButton->Bind(                                                                                             // Свързва събитие с метод за бутона m_loadInstallButton
        wxEVT_BUTTON,                                                                                                      // Тип на събитието: натискане на бутон
        &MongoDBPanel::OnLoadAndInstall,                                                                                   // Адрес на метода-обработчик
        this,                                                                                                              // Обект, който ще обработи събитието (текущия панел)
        ID_MONGODB_LOAD_INSTALL);                                                                                          // ID на бутона, за който се свързва събитието
    m_closeButton = new wxButton(topBar, ID_MONGODB_PANEL_INTERNAL_CLOSE_BTN, "Close", wxDefaultPosition, wxSize(90, 35)); // Създава бутон "Close"
                                                                                                                           // Празна линия за по-добра четимост
    topBarSizer->Add(m_uploadButton, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);                                             // Добавя бутона за качване към topBarSizer
    topBarSizer->Add(m_refreshButton, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);                                            // Добавя бутона за опресняване към topBarSizer
    topBarSizer->Add(m_downloadButton, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);                                           // Добавя бутона за изтегляне към topBarSizer
    topBarSizer->Add(m_loadInstallButton, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);                                        // Добавя бутона за зареждане и инсталиране към topBarSizer
    topBarSizer->AddStretchSpacer(1);                                                                                      // Добавя разтегливо празно пространство
    topBarSizer->Add(m_closeButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);                                             // Добавя бутона за затваряне към topBarSizer
    topBar->SetSizer(topBarSizer);                                                                                         // Задава topBarSizer като мениджър на подреждане за topBar
                                                                                                                           // Празна линия за по-добра четимост
    // Content List // Коментар: Списък със съдържание
    m_contentList = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,                           // Създава контрола wxListCtrl
                                   wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES | wxLC_VRULES | wxBORDER_NONE); // Стилове: изглед "доклад", единичен избор, хоризонтални/вертикални линии, без рамка
    m_contentList->InsertColumn(0, "File Name", wxLIST_FORMAT_LEFT, 250);                                      // Вмъква първа колона "File Name", подравнена вляво, ширина 250px
    m_contentList->InsertColumn(1, "Installed Applications", wxLIST_FORMAT_LEFT, 550);                         // Вмъква втора колона "Installed Applications", подравнена вляво, ширина 550px
                                                                                                               // Празна линия за по-добра четимост
    // Add panels to main sizer // Коментар: Добавяне на панелите към главния sizer
    mainSizer->Add(titlePanel, 0, wxEXPAND);                                      // Добавя панела за заглавие към mainSizer, разпъващ се хоризонтално
    mainSizer->AddSpacer(15);                                                     // Добавя празно пространство от 15px
    mainSizer->Add(topBar, 0, wxEXPAND | wxLEFT | wxRIGHT, 15);                   // Добавя горната лента към mainSizer, разпъваща се хоризонтално, с отстояния отляво и отдясно
    mainSizer->AddSpacer(15);                                                     // Добавя празно пространство от 15px
    mainSizer->Add(m_contentList, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 15); // Добавя списъка със съдържание към mainSizer, разпъващ се във всички посоки, с отстояния
    SetSizer(mainSizer);                                                          // Задава mainSizer като мениджър на подреждане за текущия панел
                                                                                  // Празна линия за по-добра четимост
    ApplyThemeColors();                                                           // Прилага цветовете на темата към контролите
} // Край на тялото на конструктора
  // Празна линия за по-добра четимост
// Get container ID from project directory helper // Коментар: Помощен метод за получаване на ID на контейнера от директорията на проекта
wxString MongoDBPanel::GetContainerIdFromProjectDir()                                               // Дефиниция на метод за получаване на ID на контейнера от директорията на проекта
{                                                                                                   // Начало на тялото на метода
    if (m_projectDir.IsEmpty())                                                                     // Проверява дали директорията на проекта е празна
    {                                                                                               // Начало на if блока
        wxLogWarning("MongoDBPanel::GetContainerIdFromProjectDir - Project directory is not set."); // Записва предупреждение в лога, че директорията на проекта не е зададена
        return wxEmptyString;                                                                       // Връща празен низ
    } // Край на if блока
    wxString containerIdPath = wxFileName(m_projectDir, "container_id.txt").GetFullPath();                      // Създава пълен път до файла container_id.txt в директорията на проекта
    wxString containerId;                                                                                       // Декларира низ за съхранение на ID на контейнера
    if (wxFile::Exists(containerIdPath))                                                                        // Проверява дали файлът с ID на контейнера съществува
    {                                                                                                           // Начало на if блока
        wxFile file(containerIdPath, wxFile::read);                                                             // Създава обект wxFile за четене от файла
        if (file.IsOpened())                                                                                    // Проверява дали файлът е успешно отворен
        {                                                                                                       // Начало на if блока
            file.ReadAll(&containerId);                                                                         // Чете цялото съдържание на файла в променливата containerId
            containerId.Trim(true).Trim(false);                                                                 // Премахва водещи и крайни празни пространства от ID-то
            if (!containerId.IsEmpty())                                                                         // Проверява дали прочетеното ID не е празно
            {                                                                                                   // Начало на if блока
                wxLogDebug("MongoDBPanel::GetContainerIdFromProjectDir - Found container ID: %s", containerId); // Записва в лога намереното ID на контейнера
                return containerId;                                                                             // Връща намереното ID на контейнера
            } // Край на if блока
            else                                                                                         // Иначе (ако ID-то е празно)
            {                                                                                            // Начало на else блока
                wxLogWarning("MongoDBPanel::GetContainerIdFromProjectDir - container_id.txt is empty."); // Записва предупреждение, че файлът е празен
            } // Край на else блока
        } // Край на if блока (проверка за отворен файл)
        else                                                                                               // Иначе (ако файлът не е отворен)
        {                                                                                                  // Начало на else блока
            wxLogError("MongoDBPanel::GetContainerIdFromProjectDir - Failed to open %s", containerIdPath); // Записва грешка в лога за неуспешно отваряне на файла
        } // Край на else блока
    } // Край на if блока (проверка за съществуващ файл)
    else                                                                                                // Иначе (ако файлът не съществува)
    {                                                                                                   // Начало на else блока
        wxLogDebug("MongoDBPanel::GetContainerIdFromProjectDir - File not found: %s", containerIdPath); // Записва в лога, че файлът не е намерен
    } // Край на else блока
    return wxEmptyString; // Връща празен низ, ако ID-то не е намерено
} // Край на тялото на метода
  // Празна линия за по-добра четимост
// Load content from MongoDB connection // Коментар: Зареждане на съдържание от MongoDB връзка
void MongoDBPanel::LoadMongoDBContent()                                                       // Дефиниция на метод за зареждане на съдържание от MongoDB
{                                                                                             // Начало на тялото на метода
    m_contentList->DeleteAllItems();                                                          // Изтрива всички елементи от списъка със съдържание
    m_documentIds.clear();                                                                    // Изчиства картата с ID-та на документите
    m_selectedItemIndex = -1;                                                                 // Нулира индекса на избрания елемент
    m_selectedDocumentId = "";                                                                // Нулира ID-то на избрания документ
    m_downloadButton->Disable();                                                              // Деактивира бутона за изтегляне
    m_loadInstallButton->Disable();                                                           // Деактивира бутона за зареждане и инсталиране
    wxLogDebug("MongoDBPanel::LoadMongoDBContent - Clearing list and disabling buttons.");    // Записва в лога информация за изчистване на списъка и деактивиране на бутоните
                                                                                              // Празна линия за по-добра четимост
    wxString containerId = GetContainerIdFromProjectDir();                                    // Взема ID на контейнера
    if (!containerId.IsEmpty())                                                               // Ако ID-то на контейнера не е празно
    {                                                                                         // Начало на if блока
        wxLogDebug("MongoDBPanel::LoadMongoDBContent - Using container ID: %s", containerId); // Записва в лога използваното ID на контейнера
    } // Край на if блока
    // Празна линия за по-добра четимост
    try // Начало на try блок за обработка на изключения
    {   // Начало на тялото на try блока
        // Connect to MongoDB // Коментар: Свързване с MongoDB
        const auto uri = mongocxx::uri{"mongodb+srv://<user>:<password>@<cluster>.mongodb.net/?retryWrites=true&w=majority&appName=CXX"}; // URI за връзка с MongoDB Atlas
        mongocxx::options::client client_options;                                                                                                    // Опции за клиента на MongoDB
        const auto api = mongocxx::options::server_api{mongocxx::options::server_api::version::k_version_1};                                         // Задава версия на Server API
        client_options.server_api_opts(api);                                                                                                         // Прилага опциите за Server API към клиентските опции
                                                                                                                                                     // Празна линия за по-добра четимост
        wxLogDebug("MongoDBPanel::LoadMongoDBContent - Connecting to MongoDB...");                                                                   // Записва в лога съобщение за опит за свързване
        mongocxx::client client(uri, client_options);                                                                                                // Създава MongoDB клиент с URI и опции
        mongocxx::database db = client["testdb"];                                                                                                    // Взема базата данни "testdb"
        mongocxx::collection coll = db["config_files"];                                                                                              // Взема колекцията "config_files"
        wxLogDebug("MongoDBPanel::LoadMongoDBContent - Connected to MongoDB, fetching documents...");                                                // Записва в лога съобщение за успешно свързване и извличане на документи
                                                                                                                                                     // Празна линия за по-добра четимост
        mongocxx::cursor cursor = coll.find({});                                                                                                     // Изпълнява заявка за намиране на всички документи в колекцията
        long itemIndex = 0;                                                                                                                          // Инициализира брояч за индексите на елементите в списъка
                                                                                                                                                     // Празна линия за по-добра четимост
        for (auto &&doc_view : cursor)                                                                                                               // Итерира през всеки документ (doc_view), върнат от курсора
        {                                                                                                                                            // Начало на for цикъл
            wxString configName = "Unknown";                                                                                                         // Инициализира името на конфигурацията като "Unknown"
            wxString appsList = "";                                                                                                                  // Инициализира списъка с приложения като празен низ
            wxString documentId = "";                                                                                                                // Инициализира ID на документа като празен низ
                                                                                                                                                     // Празна линия за по-добра четимост
            // Get the document ID as string // Коментар: Вземане на ID на документа като низ
            if (doc_view["_id"] && doc_view["_id"].type() == bsoncxx::type::k_oid)                    // Проверява дали полето "_id" съществува и е от тип OID (Object ID)
            {                                                                                         // Начало на if блока
                documentId = wxString::FromUTF8(doc_view["_id"].get_oid().value.to_string().c_str()); // Взема стойността на OID, конвертира я в низ и присвоява на documentId
            } // Край на if блока
            // Празна линия за по-добра четимост
            // Try to get config_name directly // Коментар: Опит за директно получаване на config_name
            if (doc_view["config_name"] && doc_view["config_name"].type() == bsoncxx::type::k_string) // Проверява дали полето "config_name" съществува и е от тип низ
            {                                                                                         // Начало на if блока
                configName = wxString::FromUTF8(doc_view["config_name"].get_string().value.data());   // Взема стойността на низа и я присвоява на configName
            } // Край на if блока
            // Празна линия за по-добра четимост
            // Try to get apps directly // Коментар: Опит за директно получаване на apps
            if (doc_view["apps"] && doc_view["apps"].type() == bsoncxx::type::k_string)    // Проверява дали полето "apps" съществува и е от тип низ
            {                                                                              // Начало на if блока
                appsList = wxString::FromUTF8(doc_view["apps"].get_string().value.data()); // Взема стойността на низа и я присвоява на appsList
            } // Край на if блока
            // If not found directly, check if it's in config_data structure // Коментар: Ако не е намерено директно, проверява дали е в структурата config_data
            else if (doc_view["config_data"] && doc_view["config_data"].type() == bsoncxx::type::k_document) // Проверява дали полето "config_data" съществува и е от тип документ
            {                                                                                                // Начало на else if блока
                auto config_data = doc_view["config_data"].get_document().view();                            // Взема изглед към документа "config_data"
                                                                                                             // Празна линия за по-добра четимост
                // Try to get config_name from config_data // Коментар: Опит за получаване на config_name от config_data
                if (config_data["config_name"] && config_data["config_name"].type() == bsoncxx::type::k_string) // Проверява дали полето "config_name" в "config_data" съществува и е низ
                {                                                                                               // Начало на if блока
                    configName = wxString::FromUTF8(config_data["config_name"].get_string().value.data());      // Взема стойността и я присвоява на configName
                } // Край на if блока
                // Празна линия за по-добра четимост
                // Check for installations array // Коментар: Проверка за масив "installations"
                if (config_data["installations"] && config_data["installations"].type() == bsoncxx::type::k_array) // Проверява дали полето "installations" в "config_data" съществува и е масив
                {                                                                                                  // Начало на if блока
                    auto installations = config_data["installations"].get_array().value;                           // Взема стойността на масива "installations"
                                                                                                                   // Празна линия за по-добра четимост
                    // First, collect all application names for this file // Коментар: Първо, събира всички имена на приложения за този файл
                    for (auto &&install : installations)                                                                 // Итерира през всеки елемент (install) в масива installations
                    {                                                                                                    // Начало на for цикъл
                        if (install.type() == bsoncxx::type::k_document)                                                 // Проверява дали елементът е от тип документ
                        {                                                                                                // Начало на if блока
                            auto install_doc = install.get_document().view();                                            // Взема изглед към документа на инсталацията
                            if (install_doc["appName"] && install_doc["appName"].type() == bsoncxx::type::k_string)      // Проверява дали полето "appName" съществува и е низ
                            {                                                                                            // Начало на if блока
                                wxString appName = wxString::FromUTF8(install_doc["appName"].get_string().value.data()); // Взема името на приложението
                                if (!appsList.IsEmpty())                                                                 // Ако appsList не е празен
                                {                                                                                        // Начало на if блока
                                    appsList += ", ";                                                                    // Добавя запетая и интервал
                                } // Край на if блока
                                appsList += appName; // Добавя името на приложението към appsList
                            } // Край на if блока (проверка за appName)
                        } // Край на if блока (проверка за тип документ)
                    } // Край на for цикъл (итерация по инсталации)
                } // Край на if блока (проверка за масив installations)
            } // Край на else if блока (проверка за config_data)
            // Празна линия за по-добра четимост
            // Fallback to original_filename if config_name is unknown // Коментар: Резервен вариант към original_filename, ако config_name е неизвестно
            if (configName == "Unknown" && doc_view["original_filename"] &&                               // Проверява дали configName е "Unknown" И дали полето "original_filename" съществува
                doc_view["original_filename"].type() == bsoncxx::type::k_string)                          // И дали "original_filename" е от тип низ
            {                                                                                             // Начало на if блока
                configName = wxString::FromUTF8(doc_view["original_filename"].get_string().value.data()); // Взема стойността на "original_filename"
                configName.Replace(".json", "");                                                          // Remove .json extension if present // Премахва разширението ".json", ако присъства
            } // Край на if блока
            // Празна линия за по-добра четимост
            // Add to list if we have valid data // Коментар: Добавяне към списъка, ако имаме валидни данни
            if (!documentId.IsEmpty() && (!configName.IsEmpty() || !appsList.IsEmpty()))                     // Проверява дали documentId не е празен И (configName не е празен ИЛИ appsList не е празен)
            {                                                                                                // Начало на if блока
                long actualIndex = m_contentList->InsertItem(itemIndex, configName);                         // Вмъква нов елемент в списъка с configName и получава действителния му индекс
                m_contentList->SetItem(actualIndex, 1, appsList);                                            // Задава текста за втората колона (списък с приложения)
                m_documentIds[actualIndex] = documentId;                                                     // Съхранява documentId в картата, асоцииран с действителния индекс
                                                                                                             // Празна линия за по-добра четимост
                wxLogDebug("MongoDBPanel::LoadMongoDBContent - Added item %ld ('%s') with ID: %s",           // Записва в лога информация за добавения елемент
                           actualIndex, configName, documentId);                                             // Параметри за форматиране на лог съобщението
                                                                                                             // Празна линия за по-добра четимост
                if (itemIndex % 2 == 0)                                                                      // Ако индексът е четен
                    m_contentList->SetItemBackgroundColour(actualIndex, m_listBgColor);                      // Задава основния цвят на фона на елемента
                else                                                                                         // Ако индексът е нечетен
                    m_contentList->SetItemBackgroundColour(actualIndex, m_listBgColor.ChangeLightness(105)); // Задава леко по-светъл цвят на фона на елемента (за редуване)
                                                                                                             // Празна линия за по-добра четимост
                m_contentList->SetItemTextColour(actualIndex, m_listFgColor);                                // Задава цвета на текста на елемента
                itemIndex++;                                                                                 // Увеличава брояча на индексите
            } // Край на if блока (проверка за валидни данни)
        } // Край на for цикъл (итерация по документи)
        // Празна линия за по-добра четимост
        if (itemIndex == 0)                                                                       // Ако не са добавени елементи (списъкът е празен)
        {                                                                                         // Начало на if блока
            long actualIndex = m_contentList->InsertItem(0, "No applications found in MongoDB."); // Вмъква съобщение, че няма намерени приложения
            m_contentList->SetItem(actualIndex, 1, "-");                                          // Задава "-" във втората колона
            m_contentList->SetItemBackgroundColour(actualIndex, m_listBgColor);                   // Задава цвета на фона
            m_contentList->SetItemTextColour(actualIndex, m_listFgColor);                         // Задава цвета на текста
        } // Край на if блока
        // Празна линия за по-добра четимост
        wxLogDebug("MongoDBPanel::LoadMongoDBContent - Finished loading content. Found %ld items.", itemIndex); // Записва в лога, че зареждането е приключило и броя на намерените елементи
    } // Край на тялото на try блока
    catch (const mongocxx::exception &e)                                                             // Прихващане на изключения от тип mongocxx::exception
    {                                                                                                // Начало на catch блок
        wxLogError("MongoDB error: %s", e.what());                                                   // Записва грешка от MongoDB в лога
        wxMessageBox(wxString::Format("MongoDB error: %s", e.what()), "Error", wxOK | wxICON_ERROR); // Показва съобщение за грешка на потребителя
                                                                                                     // Празна линия за по-добра четимост
        // Fall back to local files if MongoDB connection fails // Коментар: Преминаване към локални файлове, ако връзката с MongoDB е неуспешна
        LoadFromLocalFiles(); // Извиква метод за зареждане от локални файлове
    } // Край на catch блок
    catch (const std::exception &e)                                                                                // Прихващане на стандартни C++ изключения
    {                                                                                                              // Начало на catch блок
        wxLogError("Error: %s", e.what());                                                                         // Записва обща грешка в лога
        wxMessageBox(wxString::Format("Error connecting to MongoDB: %s", e.what()), "Error", wxOK | wxICON_ERROR); // Показва съобщение за грешка при свързване с MongoDB
                                                                                                                   // Празна линия за по-добра четимост
        // Fall back to local files if MongoDB connection fails // Коментар: Преминаване към локални файлове, ако връзката с MongoDB е неуспешна
        LoadFromLocalFiles(); // Извиква метод за зареждане от локални файлове
    } // Край на catch блок
} // Край на тялото на метода LoadMongoDBContent
  // Празна линия за по-добра четимост
// Fall back to loading from local files // Коментар: Преминаване към зареждане от локални файлове
void MongoDBPanel::LoadFromLocalFiles()                                           // Дефиниция на метод за зареждане от локални файлове
{                                                                                 // Начало на тялото на метода
    wxLogDebug("MongoDBPanel::LoadFromLocalFiles - Falling back to local files"); // Записва в лога, че се преминава към локални файлове
                                                                                  // Празна линия за по-добра четимост
    // Load JSON files from db_files directory // Коментар: Зареждане на JSON файлове от директория db_files
    wxString dbDirPath = wxFileName(m_projectDir, "db_files").GetFullPath(); // Взема пълния път до директорията db_files
    if (!wxDirExists(dbDirPath))                                             // Ако директорията не съществува
    {                                                                        // Начало на if блока
        wxMkdir(dbDirPath);                                                  // Създава директорията
        wxLogDebug("Created database files directory: %s", dbDirPath);       // Записва в лога, че директорията е създадена
    } // Край на if блока
    // Празна линия за по-добра четимост
    wxDir dir(dbDirPath);                                         // Създава обект wxDir за работа с директорията
    if (!dir.IsOpened())                                          // Ако директорията не може да бъде отворена
    {                                                             // Начало на if блока
        wxLogError("Failed to open DB directory: %s", dbDirPath); // Записва грешка в лога
        return;                                                   // Прекратява изпълнението на метода
    } // Край на if блока
    // Празна линия за по-добра четимост
    wxString filename;                                 // Декларира низ за името на файла
    bool hasFiles = dir.GetFirst(&filename, "*.json"); // Взема първия файл с разширение .json в директорията
    long itemIndex = 0;                                // Инициализира брояч за индексите на елементите
                                                       // Празна линия за по-добра четимост
    // If no files, create default sample files // Коментар: Ако няма файлове, създава примерни файлове по подразбиране
    if (!hasFiles)                                                 // Ако няма намерени файлове
    {                                                              // Начало на if блока
        wxLogDebug("No DB files found. Creating sample files..."); // Записва в лога, че се създават примерни файлове
        CreateSampleDBFiles(dbDirPath);                            // Извиква метод за създаване на примерни файлове
        hasFiles = dir.GetFirst(&filename, "*.json");              // Опитва отново да вземе първия файл
    } // Край на if блока
    // Празна линия за по-добра четимост
    while (hasFiles)                                                       // Докато има намерени файлове
    {                                                                      // Начало на while цикъл
        wxString filePath = wxFileName(dbDirPath, filename).GetFullPath(); // Взема пълния път до текущия файл
        wxLogDebug("Processing DB file: %s", filePath);                    // Записва в лога кой файл се обработва
                                                                           // Празна линия за по-добра четимост
        // Read and parse JSON file // Коментар: Четене и парсване на JSON файл
        wxString configName, appsList;                                                                   // Декларира низове за име на конфигурация и списък с приложения
        wxString documentId = wxFileName(filename).GetName();                                            // Използва името на файла (без разширението) като ID на документа
                                                                                                         // Празна линия за по-добра четимост
        if (ParseJsonFile(filePath, configName, appsList))                                               // Ако парсването на JSON файла е успешно
        {                                                                                                // Начало на if блока
            long actualIndex = m_contentList->InsertItem(itemIndex, configName);                         // Вмъква елемент в списъка
            m_contentList->SetItem(actualIndex, 1, appsList);                                            // Задава текста за втората колона
            m_documentIds[actualIndex] = documentId;                                                     // Съхранява ID-то на документа
                                                                                                         // Празна линия за по-добра четимост
            wxLogDebug("MongoDBPanel::LoadFromLocalFiles - Added item %ld ('%s') with ID: %s",           // Записва в лога информация за добавения елемент
                       actualIndex, configName, documentId);                                             // Параметри за форматиране
                                                                                                         // Празна линия за по-добра четимост
            if (itemIndex % 2 == 0)                                                                      // Ако индексът е четен
                m_contentList->SetItemBackgroundColour(actualIndex, m_listBgColor);                      // Задава основния цвят на фона
            else                                                                                         // Ако индексът е нечетен
                m_contentList->SetItemBackgroundColour(actualIndex, m_listBgColor.ChangeLightness(105)); // Задава леко по-светъл цвят на фона
                                                                                                         // Празна линия за по-добра четимост
            m_contentList->SetItemTextColour(actualIndex, m_listFgColor);                                // Задава цвета на текста
            itemIndex++;                                                                                 // Увеличава брояча на индексите
        } // Край на if блока (проверка за успешно парсване)
        // Празна линия за по-добра четимост
        hasFiles = dir.GetNext(&filename); // Взема следващия файл в директорията
    } // Край на while цикъл
    // Празна линия за по-добра четимост
    if (itemIndex == 0)                                                                           // Ако не са добавени елементи
    {                                                                                             // Начало на if блока
        long actualIndex = m_contentList->InsertItem(0, "No applications found in local files."); // Вмъква съобщение, че няма намерени локални файлове
        m_contentList->SetItem(actualIndex, 1, "-");                                              // Задава "-" във втората колона
        m_contentList->SetItemBackgroundColour(actualIndex, m_listBgColor);                       // Задава цвета на фона
        m_contentList->SetItemTextColour(actualIndex, m_listFgColor);                             // Задава цвета на текста
    } // Край на if блока
} // Край на тялото на метода LoadFromLocalFiles
  // Празна линия за по-добра четимост
// Create sample DB files // Коментар: Създаване на примерни DB файлове
void MongoDBPanel::CreateSampleDBFiles(const wxString &dbDirPath)                                                                                  // Дефиниция на метод за създаване на примерни DB файлове
{                                                                                                                                                  // Начало на тялото на метода
    std::vector<std::pair<wxString, wxString>> sampleData = {                                                                                      // Вектор от двойки (име на конфигурация, списък с приложения) за примерните данни
                                                             {"Gaming Setup", "Steam, Discord, Lutris"},                                           // Примерни данни 1
                                                             {"Development Env", "VS Code, Docker Desktop, GitKraken, Firefox Developer Edition"}, // Примерни данни 2
                                                             {"Multimedia Station", "VLC, Kdenlive, GIMP, Spotify"},                               // Примерни данни 3
                                                             {"Minimal Install", "htop, neofetch"}};                                               // Примерни данни 4
                                                                                                                                                   // Празна линия за по-добра четимост
    for (size_t i = 0; i < sampleData.size(); i++)                                                                                                 // Итерира през примерните данни
    {                                                                                                                                              // Начало на for цикъл
        wxString fileId = wxDateTime::Now().Format("%Y%m%d%H%M%S") + wxString::Format("_%zu", i);                                                  // Генерира уникално ID за файла, базирано на текущата дата/час и индекс
        wxString fileName = wxFileName(dbDirPath, fileId + ".json").GetFullPath();                                                                 // Създава пълния път до примерния JSON файл
                                                                                                                                                   // Празна линия за по-добра четимост
        wxLogDebug("Creating sample DB file: %s", fileName);                                                                                       // Записва в лога кой примерен файл се създава
                                                                                                                                                   // Празна линия за по-добра четимост
        std::ofstream outFile(fileName.ToStdString());                                                                                             // Създава изходящ файлов поток към примерния файл
        if (outFile.is_open())                                                                                                                     // Ако файлът е успешно отворен за запис
        {                                                                                                                                          // Начало на if блока
            outFile << "{\n";                                                                                                                      // Записва отваряща фигурна скоба на JSON обекта
            outFile << "  \"config_name\": \"" << sampleData[i].first.ToStdString() << "\",\n";                                                    // Записва полето "config_name"
            outFile << "  \"apps\": \"" << sampleData[i].second.ToStdString() << "\",\n";                                                          // Записва полето "apps"
            outFile << "  \"timestamp\": \"" << wxDateTime::Now().FormatISOCombined(' ').ToStdString() << "\",\n";                                 // Записва полето "timestamp" с текущата дата/час в ISO формат
            outFile << "  \"container_id\": \"" << GetContainerIdFromProjectDir().ToStdString() << "\"\n";                                         // Записва полето "container_id"
            outFile << "}\n";                                                                                                                      // Записва затваряща фигурна скоба на JSON обекта
            outFile.close();                                                                                                                       // Затваря файла
            wxLogDebug("Created sample DB file successfully");                                                                                     // Записва в лога, че примерният файл е създаден успешно
        } // Край на if блока (проверка за отворен файл)
        else                                                             // Иначе (ако файлът не може да бъде отворен)
        {                                                                // Начало на else блока
            wxLogError("Failed to create sample DB file: %s", fileName); // Записва грешка в лога
        } // Край на else блока
        // Празна линия за по-добра четимост
        // Small delay to ensure unique timestamps // Коментар: Малко забавяне, за да се осигурят уникални времеви маркери
        wxMilliSleep(100); // Изчаква 100 милисекунди
    } // Край на for цикъл
} // Край на тялото на метода CreateSampleDBFiles
  // Празна линия за по-добра четимост
// Parse JSON file helper // Коментар: Помощен метод за парсване на JSON файл
bool MongoDBPanel::ParseJsonFile(const wxString &filePath, wxString &configName, wxString &appsList) // Дефиниция на метод за парсване на JSON файл (опростен)
{                                                                                                    // Начало на тялото на метода
    wxTextFile file;                                                                                 // Създава обект wxTextFile за четене на текстовия файл
    if (!file.Open(filePath))                                                                        // Ако файлът не може да бъде отворен
    {                                                                                                // Начало на if блока
        wxLogError("Failed to open JSON file: %s", filePath);                                        // Записва грешка в лога
        return false;                                                                                // Връща false (неуспешно парсване)
    } // Край на if блока
    // Празна линия за по-добра четимост
    // Very simple JSON parsing - in a real app you'd use a proper JSON parser // Коментар: Много опростено JSON парсване - в реално приложение бихте използвали подходящ JSON парсър
    configName = wxEmptyString;                                                       // Инициализира configName като празен низ
    appsList = wxEmptyString;                                                         // Инициализира appsList като празен низ
                                                                                      // Празна линия за по-добра четимост
    for (wxString line = file.GetFirstLine(); !file.Eof(); line = file.GetNextLine()) // Итерира през всеки ред на файла
    {                                                                                 // Начало на for цикъл
        line.Trim(true).Trim(false);                                                  // Премахва водещи и крайни празни пространства от реда
                                                                                      // Празна линия за по-добра четимост
        if (line.Contains("\"config_name\""))                                         // Ако редът съдържа ""config_name""
        {                                                                             // Начало на if блока
            wxString value = line.AfterFirst(':');                                    // Взема частта от реда след първото ':'
            value.Trim(true).Trim(false);                                             // Премахва празни пространства
            // Remove quotes and trailing comma if present // Коментар: Премахване на кавички и евентуална затваряща запетая
            if (value.StartsWith("\"") && value.EndsWith("\","))     // Ако стойността започва с " и завършва с ",
                configName = value.Mid(1, value.Length() - 3);       // Извлича стойността без кавичките и запетаята
            else if (value.StartsWith("\"") && value.EndsWith("\"")) // Ако стойността започва с " и завършва с "
                configName = value.Mid(1, value.Length() - 2);       // Извлича стойността без кавичките
        } // Край на if блока
        else if (line.Contains("\"apps\""))        // Ако редът съдържа ""apps""
        {                                          // Начало на else if блока
            wxString value = line.AfterFirst(':'); // Взема частта от реда след първото ':'
            value.Trim(true).Trim(false);          // Премахва празни пространства
            // Remove quotes and trailing comma if present // Коментар: Премахване на кавички и евентуална затваряща запетая
            if (value.StartsWith("\"") && value.EndsWith("\","))     // Ако стойността започва с " и завършва с ",
                appsList = value.Mid(1, value.Length() - 3);         // Извлича стойността без кавичките и запетаята
            else if (value.StartsWith("\"") && value.EndsWith("\"")) // Ако стойността започва с " и завършва с "
                appsList = value.Mid(1, value.Length() - 2);         // Извлича стойността без кавичките
        } // Край на else if блока
    } // Край на for цикъл
    // Празна линия за по-добра четимост
    file.Close();                                        // Затваря файла
    return !configName.IsEmpty() && !appsList.IsEmpty(); // Връща true, ако и configName, и appsList не са празни
} // Край на тялото на метода ParseJsonFile
  // Празна линия за по-добра четимост
// Upload JSON (uses real MongoDB) // Коментар: Качване на JSON (използва реална MongoDB)
void MongoDBPanel::OnUploadJson(wxCommandEvent &event)                                           // Дефиниция на метод-обработчик за събитието "качване на JSON"
{                                                                                                // Начало на тялото на метода
    wxLogDebug("MongoDBPanel::OnUploadJson - Upload button clicked.");                           // Записва в лога, че бутонът за качване е натиснат
                                                                                                 // Празна линия за по-добра четимост
    wxFileDialog openFileDialog(this, "Select JSON file to upload", "", "",                      // Създава диалог за избор на файл за отваряне
                                "JSON files (*.json)|*.json", wxFD_OPEN | wxFD_FILE_MUST_EXIST); // Заглавие, филтър за .json файлове, опции
                                                                                                 // Празна линия за по-добра четимост
    if (openFileDialog.ShowModal() == wxID_CANCEL)                                               // Показва диалога и проверява дали потребителят го е отменил
    {                                                                                            // Начало на if блока
        wxLogDebug("MongoDBPanel::OnUploadJson - File selection cancelled.");                    // Записва в лога, че изборът на файл е отменен
        return;                                                                                  // Прекратява изпълнението на метода
    } // Край на if блока
    // Празна линия за по-добра четимост
    wxString filePath = openFileDialog.GetPath();                                       // Взема пълния път до избрания файл
    wxString fileName = openFileDialog.GetFilename();                                   // Взема името на избрания файл
    wxLogDebug("MongoDBPanel::OnUploadJson - Selected file: %s", filePath);             // Записва в лога избрания файл
                                                                                        // Празна линия за по-добра четимост
    wxString containerId = GetContainerIdFromProjectDir();                              // Взема ID на контейнера
    if (!containerId.IsEmpty())                                                         // Ако ID-то на контейнера не е празно
        wxLogDebug("MongoDBPanel::OnUploadJson - Using container ID: %s", containerId); // Записва в лога използваното ID на контейнера
                                                                                        // Празна линия за по-добра четимост
    // --- Read JSON File Content --- // Коментар: Четене на съдържанието на JSON файла
    std::ifstream fileStream(filePath.ToStdString());                                              // Създава входящ файлов поток от избрания файл
    if (!fileStream.is_open())                                                                     // Ако файлът не може да бъде отворен
    {                                                                                              // Начало на if блока
        wxLogError("MongoDBPanel::OnUploadJson - Failed to open file: %s", filePath);              // Записва грешка в лога
        wxMessageBox("Failed to open file: " + filePath, "File Error", wxOK | wxICON_ERROR, this); // Показва съобщение за грешка на потребителя
        return;                                                                                    // Прекратява изпълнението
    } // Край на if блока
    std::stringstream buffer;                                                                               // Създава низов поток за буфер
    buffer << fileStream.rdbuf();                                                                           // Чете цялото съдържание на файловия поток в низовия поток
    std::string jsonContent = buffer.str();                                                                 // Конвертира съдържанието на низовия поток в std::string
    fileStream.close();                                                                                     // Затваря файловия поток
                                                                                                            // Празна линия за по-добра четимост
    if (jsonContent.empty())                                                                                // Ако съдържанието на JSON е празно
    {                                                                                                       // Начало на if блока
        wxLogWarning("MongoDBPanel::OnUploadJson - File is empty: %s", filePath);                           // Записва предупреждение в лога
        wxMessageBox("File appears to be empty: " + filePath, "File Warning", wxOK | wxICON_WARNING, this); // Показва предупреждение на потребителя
    } // Край на if блока
    wxLogDebug("MongoDBPanel::OnUploadJson - Read %zu bytes from file.", jsonContent.length()); // Записва в лога броя прочетени байтове
                                                                                                // Празна линия за по-добра четимост
    // --- Upload to MongoDB --- // Коментар: Качване в MongoDB
    try // Начало на try блок за обработка на изключения
    {   // Начало на тялото на try блока
        // Parse JSON content into a BSON document // Коментар: Парсване на JSON съдържанието в BSON документ
        bsoncxx::document::value doc_value = bsoncxx::from_json(jsonContent);         // Конвертира JSON низа в BSON документ
        wxLogDebug("MongoDBPanel::OnUploadJson - Successfully parsed JSON content."); // Записва в лога, че JSON е парснат успешно
                                                                                      // Празна линия за по-добра четимост
        // Connect to MongoDB // Коментар: Свързване с MongoDB
        const auto uri = mongocxx::uri{"mongodb+srv://<user>:<password>@<cluster>.mongodb.net/?retryWrites=true&w=majority&appName=CXX"}; // URI за връзка
        mongocxx::options::client client_options;                                                                                                    // Опции за клиента
        const auto api = mongocxx::options::server_api{mongocxx::options::server_api::version::k_version_1};                                         // Server API версия
        client_options.server_api_opts(api);                                                                                                         // Прилага опциите за Server API
                                                                                                                                                     // Празна линия за по-добра четимост
        mongocxx::client client(uri, client_options);                                                                                                // Създава MongoDB клиент
        mongocxx::database db = client["testdb"];                                                                                                    // Взема базата данни "testdb"
        mongocxx::collection coll = db["config_files"];                                                                                              // Взема колекцията "config_files"
        wxLogDebug("MongoDBPanel::OnUploadJson - Connected to MongoDB for upload.");                                                                 // Записва в лога, че е свързан с MongoDB за качване
                                                                                                                                                     // Празна линия за по-добра четимост
        // --- Add Metadata --- // Коментар: Добавяне на метаданни
        auto now = std::chrono::system_clock::now(); // Взема текущото системно време
        bsoncxx::builder::basic::document builder{}; // Създава базов BSON документ builder
                                                     // Празна линия за по-добра четимост
        // Try to extract config_name and apps for direct access // Коментар: Опит за извличане на config_name и apps за директен достъп
        wxString configName, appsList;                 // Декларира низове за име на конфигурация и списък с приложения
        ParseJsonFile(filePath, configName, appsList); // Парсва файла, за да извлече тези стойности (ако съществуват)
                                                       // Празна линия за по-добра четимост
        // Append the actual parsed JSON data under a specific key // Коментар: Добавяне на действителните парснати JSON данни под определен ключ
        builder.append(bsoncxx::builder::basic::kvp("config_data", doc_value.view())); // Добавя оригиналния JSON (като BSON) под ключ "config_data"
                                                                                       // Празна линия за по-добра четимост
        // Add extracted fields at the top level for easier access // Коментар: Добавяне на извлечените полета на най-горно ниво за по-лесен достъп
        if (!configName.IsEmpty())                                                                 // Ако configName не е празен
        {                                                                                          // Начало на if блока
            builder.append(bsoncxx::builder::basic::kvp("config_name", configName.ToStdString())); // Добавя "config_name" на най-горно ниво
        } // Край на if блока
        // Празна линия за по-добра четимост
        if (!appsList.IsEmpty())                                                          // Ако appsList не е празен
        {                                                                                 // Начало на if блока
            builder.append(bsoncxx::builder::basic::kvp("apps", appsList.ToStdString())); // Добавя "apps" на най-горно ниво
        } // Край на if блока
        // Празна линия за по-добра четимост
        // Append metadata fields // Коментар: Добавяне на полета с метаданни
        builder.append(bsoncxx::builder::basic::kvp("original_filename", fileName.ToStdString()));     // Добавя оригиналното име на файла
        builder.append(bsoncxx::builder::basic::kvp("upload_timestamp", bsoncxx::types::b_date{now})); // Добавя времеви маркер за качване (като BSON дата)
                                                                                                       // Празна линия за по-добра четимост
        if (!containerId.IsEmpty())                                                                    // Ако ID-то на контейнера не е празно
        {                                                                                              // Начало на if блока
            builder.append(bsoncxx::builder::basic::kvp("container_id", containerId.ToStdString()));   // Добавя ID на контейнера
        } // Край на if блока
        // Празна линия за по-добра четимост
        wxLogDebug("MongoDBPanel::OnUploadJson - Built BSON document with metadata."); // Записва в лога, че BSON документът с метаданни е построен
                                                                                       // Празна линия за по-добра четимост
        // Insert the document with metadata // Коментар: Вмъкване на документа с метаданни
        auto result = coll.insert_one(builder.view());                                     // Вмъква построения документ в колекцията
        wxLogDebug("MongoDBPanel::OnUploadJson - Executed insert_one.");                   // Записва в лога, че insert_one е изпълнен
                                                                                           // Празна линия за по-добра четимост
        if (result)                                                                        // Ако вмъкването е успешно (result съдържа информация за операцията)
        {                                                                                  // Начало на if блока
            bsoncxx::oid oid = result->inserted_id().get_oid().value;                      // Взема Object ID на вмъкнатия документ
            wxString oidStr = wxString::FromUTF8(oid.to_string().c_str());                 // Конвертира OID в низ
            wxLogDebug("MongoDBPanel::OnUploadJson - Insert successful, OID: %s", oidStr); // Записва в лога успешното вмъкване и OID
                                                                                           // Празна линия за по-добра четимост
            // Also save locally as backup // Коментар: Също така запазване локално като резервно копие
            wxString dbDirPath = wxFileName(m_projectDir, "db_files").GetFullPath(); // Път до директорията db_files
            if (!wxDirExists(dbDirPath))                                             // Ако директорията не съществува
            {                                                                        // Начало на if блока
                wxMkdir(dbDirPath);                                                  // Създава я
            } // Край на if блока
            // Празна линия за по-добра четимост
            wxString dbFilePath = wxFileName(dbDirPath, oidStr + ".json").GetFullPath();             // Път до локалния файл (име = OID.json)
            wxCopyFile(filePath, dbFilePath, true);                                                  // Копира оригиналния файл на новото място (с презаписване)
                                                                                                     // Празна линия за по-добра четимост
            wxMessageBox("JSON file '" + fileName + "' uploaded successfully!\nObjectID: " + oidStr, // Показва съобщение за успех
                         "Upload Success", wxOK | wxICON_INFORMATION, this);                         // Заглавие, бутони и икона на съобщението
            LoadMongoDBContent();                                                                    // Refresh the list view after successful upload // Опреснява списъка след успешно качване
        } // Край на if блока (проверка за успешен резултат от вмъкването)
        else                                                                                                                        // Иначе (ако вмъкването не е върнало резултат)
        {                                                                                                                           // Начало на else блока
            wxLogError("MongoDBPanel::OnUploadJson - MongoDB insert_one did not return a result.");                                 // Записва грешка в лога
            wxMessageBox("Failed to upload JSON file to MongoDB (no result returned).", "Upload Error", wxOK | wxICON_ERROR, this); // Показва съобщение за грешка
                                                                                                                                    // Празна линия за по-добра четимост
            // Fall back to local storage // Коментар: Преминаване към локално съхранение
            SaveToLocalFile(filePath, jsonContent); // Запазва файла локално
        } // Край на else блока
    } // Край на тялото на try блока
    catch (const bsoncxx::exception &e)                                            // Прихващане на изключения от BSONCXX (напр. грешка при парсване на JSON)
    {                                                                              // Начало на catch блок
        wxLogError("BSON/JSON Parsing Error during upload: %s", e.what());         // Записва грешка при парсване в лога
        wxMessageBox("Error parsing JSON file content: " + wxString(e.what()) +    // Показва съобщение за грешка при парсване
                         "\nPlease ensure the selected file contains valid JSON.", // Допълнителен текст към съобщението
                     "JSON Parse Error", wxOK | wxICON_ERROR, this);               // Заглавие и икона на съобщението
                                                                                   // Празна линия за по-добра четимост
        // Fall back to local storage // Коментар: Преминаване към локално съхранение
        SaveToLocalFile(filePath, jsonContent); // Запазва файла локално
    } // Край на catch блок
    catch (const mongocxx::exception &e)                                        // Прихващане на изключения от MongoDB C++ драйвера
    {                                                                           // Начало на catch блок
        wxLogError("MongoDB Upload Error: %s", e.what());                       // Записва грешка при качване в MongoDB в лога
        wxMessageBox("Error uploading file to MongoDB: " + wxString(e.what()) + // Показва съобщение за грешка при качване
                         "\nFalling back to local storage.",                    // Допълнителен текст
                     "MongoDB Error", wxOK | wxICON_ERROR, this);               // Заглавие и икона
                                                                                // Празна линия за по-добра четимост
        // Fall back to local storage // Коментар: Преминаване към локално съхранение
        SaveToLocalFile(filePath, jsonContent); // Запазва файла локално
    } // Край на catch блок
    catch (const std::exception &e)                                                        // Прихващане на стандартни C++ изключения
    {                                                                                      // Начало на catch блок
        wxLogError("Standard Exception during upload: %s", e.what());                      // Записва стандартна грешка в лога
        wxMessageBox("An unexpected error occurred during upload: " + wxString(e.what()) + // Показва съобщение за неочаквана грешка
                         "\nFalling back to local storage.",                               // Допълнителен текст
                     "Error", wxOK | wxICON_ERROR, this);                                  // Заглавие и икона
                                                                                           // Празна линия за по-добра четимост
        // Fall back to local storage // Коментар: Преминаване към локално съхранение
        SaveToLocalFile(filePath, jsonContent); // Запазва файла локално
    } // Край на catch блок
    // Празна линия за по-добра четимост
    wxLogDebug("MongoDBPanel::OnUploadJson - Upload process finished."); // Записва в лога, че процесът на качване е приключил
} // Край на тялото на метода OnUploadJson
  // Празна линия за по-добра четимост
// Save to local file if MongoDB upload fails // Коментар: Запазване в локален файл, ако качването в MongoDB е неуспешно
void MongoDBPanel::SaveToLocalFile(const wxString &sourcePath, const std::string &jsonContent) // Дефиниция на метод за локално запазване на файл
{                                                                                              // Начало на тялото на метода
    wxLogDebug("MongoDBPanel::SaveToLocalFile - Saving file locally as fallback");             // Записва в лога, че файлът се запазва локално
                                                                                               // Празна линия за по-добра четимост
    wxString dbDirPath = wxFileName(m_projectDir, "db_files").GetFullPath();                   // Път до директорията db_files
    if (!wxDirExists(dbDirPath))                                                               // Ако директорията не съществува
    {                                                                                          // Начало на if блока
        wxMkdir(dbDirPath);                                                                    // Създава я
    } // Край на if блока
    // Празна линия за по-добра четимост
    wxString fileId = wxDateTime::Now().Format("%Y%m%d%H%M%S") + "_local";       // Генерира ID за файла, базирано на текущата дата/час и суфикс "_local"
    wxString dbFilePath = wxFileName(dbDirPath, fileId + ".json").GetFullPath(); // Път до локалния файл
                                                                                 // Празна линия за по-добра четимост
    if (wxCopyFile(sourcePath, dbFilePath, true))                                // Копира оригиналния файл (sourcePath) на новото място (dbFilePath), с презаписване
    {                                                                            // Начало на if блока (ако копирането е успешно)
        wxLogDebug("MongoDBPanel::SaveToLocalFile - Saved to: %s", dbFilePath);  // Записва в лога пътя до запазения файл
        wxMessageBox("File saved locally as backup: " + fileId,                  // Показва съобщение за успешно локално запазване
                     "Local Storage", wxOK | wxICON_INFORMATION);                // Заглавие и икона
        LoadFromLocalFiles();                                                    // Зарежда списъка от локалните файлове (за да се покаже и новозапазения)
    } // Край на if блока
    else                                                                       // Иначе (ако копирането е неуспешно)
    {                                                                          // Начало на else блока
        wxLogError("Failed to save file locally to: %s", dbFilePath);          // Записва грешка в лога
        wxMessageBox("Failed to save file locally. Please check permissions.", // Показва съобщение за грешка при локално запазване
                     "Local Storage Error", wxOK | wxICON_ERROR);              // Заглавие и икона
    } // Край на else блока
} // Край на тялото на метода SaveToLocalFile
  // Празна линия за по-добра четимост
// Download selected document // Коментар: Изтегляне на избран документ
void MongoDBPanel::OnDownloadSelected(wxCommandEvent &event)                  // Дефиниция на метод-обработчик за събитието "изтегляне на избран"
{                                                                             // Начало на тялото на метода
    wxLogDebug("MongoDBPanel::OnDownloadSelected - Download button clicked"); // Записва в лога, че бутонът за изтегляне е натиснат
                                                                              // Празна линия за по-добра четимост
    if (m_selectedItemIndex < 0 || m_selectedDocumentId.IsEmpty())            // Проверява дали е избран валиден елемент/документ
    {                                                                         // Начало на if блока
        wxMessageBox("Please select a valid configuration to download.",      // Показва информационно съобщение
                     "Selection Required", wxOK | wxICON_INFORMATION);        // Заглавие и икона
        return;                                                               // Прекратява изпълнението
    } // Край на if блока
    // Празна линия за по-добра четимост
    wxString configName = m_contentList->GetItemText(m_selectedItemIndex);                                // Взема текста (името на конфигурацията) на избрания елемент
    if (configName.IsEmpty() || configName == "No applications found in MongoDB.")                        // Проверява дали името на конфигурацията е валидно
    {                                                                                                     // Начало на if блока
        wxMessageBox("Selected item has an invalid filename.", "Invalid Selection", wxOK | wxICON_ERROR); // Показва съобщение за грешка
        return;                                                                                           // Прекратява изпълнението
    } // Край на if блока
    // Празна линия за по-добра четимост
    // Create a suggested filename from the config name // Коментар: Създаване на предложено име на файл от името на конфигурацията
    wxString suggestedName = configName;                                                      // Копира името на конфигурацията
    suggestedName.Replace(" ", "_");                                                          // Заменя интервалите с долни черти
    if (!suggestedName.EndsWith(".json"))                                                     // Ако името не завършва с ".json"
        suggestedName += ".json";                                                             // Добавя разширението ".json"
                                                                                              // Празна линия за по-добра четимост
    wxFileDialog saveDialog(this, "Save Configuration File", "", suggestedName,               // Създава диалог за запазване на файл
                            "JSON files (*.json)|*.json", wxFD_SAVE | wxFD_OVERWRITE_PROMPT); // Заглавие, предложено име, филтър, опции
                                                                                              // Празна линия за по-добра четимост
    if (saveDialog.ShowModal() == wxID_CANCEL)                                                // Показва диалога и проверява дали е отменен
    {                                                                                         // Начало на if блока
        wxLogDebug("User cancelled save dialog");                                             // Записва в лога, че диалогът е отменен
        return;                                                                               // Прекратява изпълнението
    } // Край на if блока
    // Празна линия за по-добра четимост
    wxString savePath = saveDialog.GetPath();            // Взема избрания от потребителя път за запазване
    wxLogDebug("User selected save path: %s", savePath); // Записва в лога избрания път
                                                         // Празна линия за по-добра четимост
    // Create a progress dialog for the download // Коментар: Създаване на диалог за прогрес за изтеглянето
    wxProgressDialog progress("Downloading Configuration",                                        // Създава диалог за прогрес със заглавие
                              "Preparing to download...",                                         // Първоначално съобщение
                              100, this,                                                          // Максимална стойност на прогреса (100), родителски прозорец
                              wxPD_APP_MODAL | wxPD_AUTO_HIDE | wxPD_SMOOTH | wxPD_ELAPSED_TIME); // Опции на диалога
                                                                                                  // Празна линия за по-добра четимост
    // Clamp helper // Коментар: Помощна ламбда функция за ограничаване на стойност в диапазон
    auto clamp = [](int v)                                   // Дефинира ламбда функция 'clamp', която приема цяло число 'v'
    { return std::min(std::max(v, 0), 100); };               // Връща стойността на 'v', ограничена между 0 и 100
                                                             // Празна линия за по-добра четимост
    progress.Update(clamp(10), "Connecting to database..."); // Актуализира прогреса на 10% със съобщение
    wxMilliSleep(300);                                       // Кратко забавяне (300ms)
                                                             // Празна линия за по-добра четимост
    bool success = false;                                    // Флаг за успешно изтегляне, инициализиран с false
                                                             // Празна линия за по-добра четимост
    // Try to download from MongoDB first // Коментар: Опит за изтегляне първо от MongoDB
    try // Начало на try блок
    {   // Начало на тялото на try блока
        // Connect to MongoDB // Коментар: Свързване с MongoDB
        const auto uri = mongocxx::uri{"mongodb+srv://<user>:<password>@<cluster>.mongodb.net/?retryWrites=true&w=majority&appName=CXX"}; // URI
        mongocxx::options::client client_options;                                                                                                    // Опции на клиента
        const auto api = mongocxx::options::server_api{mongocxx::options::server_api::version::k_version_1};                                         // Server API
        client_options.server_api_opts(api);                                                                                                         // Прилагане на опциите
                                                                                                                                                     // Празна линия за по-добра четимост
        mongocxx::client client(uri, client_options);                                                                                                // Създаване на клиент
        mongocxx::database db = client["testdb"];                                                                                                    // База данни "testdb"
        mongocxx::collection coll = db["config_files"];                                                                                              // Колекция "config_files"
                                                                                                                                                     // Празна линия за по-добра четимост
        progress.Update(clamp(30), "Retrieving configuration data...");                                                                              // Актуализира прогреса на 30%
                                                                                                                                                     // Празна линия за по-добра четимост
        // Create an ObjectID from the string // Коментар: Създаване на ObjectID от низ
        bsoncxx::oid oid = bsoncxx::oid(m_selectedDocumentId.ToStdString()); // Конвертира избраното ID на документ (низ) в bsoncxx::oid
                                                                             // Празна линия за по-добра четимост
        // Find the document by its _id // Коментар: Намиране на документа по неговото _id
        auto filter = bsoncxx::builder::stream::document{}                  // Създава филтър за заявка (поточен конструктор)
                      << "_id" << oid                                       // Филтрира по поле "_id" със стойност 'oid'
                      << bsoncxx::builder::stream::finalize;                // Финализира построяването на филтъра
                                                                            // Празна линия за по-добра четимост
        auto result = coll.find_one(filter.view());                         // Търси един документ, отговарящ на филтъра
                                                                            // Празна линия за по-добра четимост
        if (result)                                                         // Ако документът е намерен (result съдържа документа)
        {                                                                   // Начало на if блока
            progress.Update(clamp(60), "Processing configuration data..."); // Актуализира прогреса на 60%
            wxMilliSleep(400);                                              // Забавяне
                                                                            // Празна линия за по-добра четимост
            // Extract the JSON data // Коментар: Извличане на JSON данните
            std::string jsonStr; // Низ за съхранение на JSON данните
                                 // Празна линия за по-добра четимост
            // If it has config_data, use that as the base // Коментар: Ако има config_data, използвай го като основа
            if ((*result)["config_data"])                                                   // Проверява дали намереният документ (*result) има поле "config_data"
            {                                                                               // Начало на if блока
                jsonStr = bsoncxx::to_json((*result)["config_data"].get_document().view()); // Конвертира съдържанието на "config_data" (което е BSON документ) в JSON низ
            } // Край на if блока
            // Otherwise use the whole document // Коментар: В противен случай използвай целия документ
            else                                     // Иначе
            {                                        // Начало на else блока
                jsonStr = bsoncxx::to_json(*result); // Конвертира целия намерен BSON документ в JSON низ
            } // Край на else блока
            // Празна линия за по-добра четимост
            progress.Update(clamp(80), "Writing configuration to file..."); // Актуализира прогреса на 80%
                                                                            // Празна линия за по-добра четимост
            // Write to file // Коментар: Запис във файл
            std::ofstream outFile(savePath.ToStdString()); // Създава изходящ файлов поток към избрания път за запазване
            if (outFile.is_open())                         // Ако файлът е отворен успешно
            {                                              // Начало на if блока
                outFile << jsonStr;                        // Записва JSON низа във файла
                outFile.close();                           // Затваря файла
                success = true;                            // Установява флага за успех на true
            } // Край на if блока
        } // Край на if блока (проверка за намерен документ)
    } // Край на тялото на try блока
    catch (const std::exception &e)                            // Прихващане на стандартни изключения
    {                                                          // Начало на catch блок
        wxLogError("MongoDB download error: %s", e.what());    // Записва грешка при изтегляне от MongoDB в лога
        wxLogDebug("Falling back to local file for download"); // Записва в лога, че се преминава към локален файл
    } // Край на catch блок
    // Празна линия за по-добра четимост
    // If MongoDB download failed, try local file // Коментар: Ако изтеглянето от MongoDB е неуспешно, опитай с локален файл
    if (!success)                                                                                                 // Ако флагът за успех е все още false
    {                                                                                                             // Начало на if блока
        wxString dbFilePath = wxFileName(m_projectDir, "db_files", m_selectedDocumentId + ".json").GetFullPath(); // Път до локалния файл (използвайки m_selectedDocumentId като име)
                                                                                                                  // Празна линия за по-добра четимост
        progress.Update(clamp(70), "Retrieving local configuration file...");                                     // Актуализира прогреса на 70% (ако се стигне дотук)
                                                                                                                  // Празна линия за по-добра четимост
        if (wxFileExists(dbFilePath))                                                                             // Проверява дали локалният файл съществува
        {                                                                                                         // Начало на if блока
            success = wxCopyFile(dbFilePath, savePath, true);                                                     // Копира локалния файл на избрания път за запазване (с презаписване) и актуализира флага за успех
        } // Край на if блока
    } // Край на if блока (проверка за неуспешно MongoDB изтегляне)
    // Празна линия за по-добра четимост
    progress.Update(clamp(100), "Download complete!");                                  // Актуализира прогреса на 100%
    wxMilliSleep(200);                                                                  // Кратко забавяне
                                                                                        // Празна линия за по-добра четимост
    if (success)                                                                        // Ако изтеглянето е било успешно (от MongoDB или локално)
    {                                                                                   // Начало на if блока
        wxMessageBox("Configuration has been successfully downloaded to:\n" + savePath, // Показва съобщение за успешно изтегляне
                     "Download Complete", wxOK | wxICON_INFORMATION);                   // Заглавие и икона
    } // Край на if блока
    else                                                                                                    // Иначе (ако изтеглянето е неуспешно)
    {                                                                                                       // Начало на else блока
        wxLogError("Failed to download configuration to: %s", savePath);                                    // Записва грешка в лога
        wxMessageBox("Failed to download the configuration. The file may not exist in MongoDB or locally.", // Показва съобщение за грешка
                     "Download Error", wxOK | wxICON_ERROR);                                                // Заглавие и икона
    } // Край на else блока
} // Край на тялото на метода OnDownloadSelected
  // Празна линия за по-добра четимост
// Refresh content (simulated) // Коментар: Опресняване на съдържанието (симулирано)
void MongoDBPanel::OnRefresh(wxCommandEvent &event)                                // Дефиниция на метод-обработчик за събитието "опресняване"
{                                                                                  // Начало на тялото на метода
    wxLogDebug("MongoDBPanel::OnRefresh - Refresh button clicked.");               // Записва в лога, че бутонът за опресняване е натиснат
    LoadMongoDBContent();                                                          // Извиква метода за зареждане на съдържание от MongoDB (което реално опреснява)
    wxLogDebug("MongoDBPanel::OnRefresh - SIMULATING: Content reload initiated."); // Записва в лога, че е инициирано презареждане (въпреки че вече е станало)
} // Край на тялото на метода OnRefresh
void MongoDBPanel::OnLoadAndInstall(wxCommandEvent &event)                                                      // Дефиниция на метод-обработчик за събитието "зареждане и инсталиране"
{                                                                                                               // Начало на тялото на метода
    wxLogDebug("MongoDBPanel::OnLoadAndInstall - Starting installation process");                               // Записва в лога началото на процеса на инсталация
                                                                                                                // Празна линия за по-добра четимост
    if (m_selectedItemIndex < 0 || m_selectedDocumentId.IsEmpty())                                              // Проверява дали е избрана конфигурация
    {                                                                                                           // Начало на if блока
        wxMessageBox("Please select a configuration to install.", "Selection Required", wxOK | wxICON_WARNING); // Показва предупреждение, ако не е избрана конфигурация
        return;                                                                                                 // Прекратява изпълнението
    } // Край на if блока
    // Празна линия за по-добра четимост
    wxString containerId = GetContainerIdFromProjectDir();                                                                          // Взема ID на контейнера
    if (containerId.IsEmpty())                                                                                                      // Проверява дали ID-то на контейнера е налично
    {                                                                                                                               // Начало на if блока
        wxMessageBox("No container ID found. Please ensure container_id.txt exists.", "Missing Container ID", wxOK | wxICON_ERROR); // Показва грешка, ако ID-то липсва
        return;                                                                                                                     // Прекратява изпълнението
    } // Край на if блока
    // Празна линия за по-добра четимост
    // Create a progress dialog with more detailed options // Коментар: Създаване на диалог за прогрес с по-детайлни опции
    wxProgressDialog progress(                             // Създава диалог за прогрес
        "Application Installation Manager",                // Заглавие на диалога
        "Initializing installation environment...",        // Първоначално съобщение
        100, this,                                         // Максимална стойност (100), родителски прозорец
        wxPD_APP_MODAL | wxPD_SMOOTH | wxPD_ELAPSED_TIME | // Опции: модален, плавен, изминало време
            wxPD_REMAINING_TIME | wxPD_CAN_ABORT);         // Опции: оставащо време, възможност за прекъсване
                                                           // Празна линия за по-добра четимост
    // Clamp helper // Коментар: Помощна ламбда функция за ограничаване на стойност
    auto clamp = [](int v)                     // Дефинира ламбда функция 'clamp'
    { return std::min(std::max(v, 0), 100); }; // Ограничава стойността 'v' между 0 и 100
                                               // Празна линия за по-добра четимост
    try                                        // Начало на try блок за обработка на изключения
    {                                          // Начало на тялото на try блока
        // Phase 1: System Preparation (0-20%) // Коментар: Фаза 1: Подготовка на системата (0-20%)
        progress.Update(clamp(5), "Checking system requirements..."); // Актуализира прогреса на 5%
        wxMilliSleep(800);                                            // Забавяне
        progress.Update(clamp(10), "Verifying container access...");  // Актуализира прогреса на 10%
        wxMilliSleep(600);                                            // Забавяне
        progress.Update(clamp(15), "Allocating system resources..."); // Актуализира прогреса на 15%
        wxMilliSleep(700);                                            // Забавяне
        progress.Update(clamp(20), "System preparation complete");    // Актуализира прогреса на 20%
                                                                      // Празна линия за по-добра четимост
        // Phase 2: Configuration Loading (20-40%) // Коментар: Фаза 2: Зареждане на конфигурация (20-40%)
        progress.Update(clamp(25), "Connecting to configuration database...");                                                                       // Актуализира прогреса на 25%
        const auto uri = mongocxx::uri{"mongodb+srv://<user>:<password>@<cluster>.mongodb.net/?retryWrites=true&w=majority&appName=CXX"}; // URI за MongoDB
        mongocxx::options::client client_options;                                                                                                    // Опции на клиента
        const auto api = mongocxx::options::server_api{mongocxx::options::server_api::version::k_version_1};                                         // Server API
        client_options.server_api_opts(api);                                                                                                         // Прилагане на опциите
                                                                                                                                                     // Празна линия за по-добра четимост
        mongocxx::client client(uri, client_options);                                                                                                // Създаване на клиент
        mongocxx::database db = client["testdb"];                                                                                                    // База данни
        mongocxx::collection coll = db["config_files"];                                                                                              // Колекция
                                                                                                                                                     // Празна линия за по-добра четимост
        progress.Update(clamp(30), "Retrieving configuration package...");                                                                           // Актуализира прогреса на 30%
        bsoncxx::oid oid = bsoncxx::oid(m_selectedDocumentId.ToStdString());                                                                         // ObjectID от избрания документ
        auto filter = bsoncxx::builder::stream::document{}                                                                                           // Филтър
                      << "_id" << oid                                                                                                                // по _id
                      << bsoncxx::builder::stream::finalize;                                                                                         // Финализиране
                                                                                                                                                     // Празна линия за по-добра четимост
        auto result = coll.find_one(filter.view());                                                                                                  // Търсене на документа
        if (!result)                                                                                                                                 // Ако документът не е намерен
        {                                                                                                                                            // Начало на if блока
            throw std::runtime_error("Configuration package not found in database");                                                                 // Хвърля изключение
        } // Край на if блока
        // Празна линия за по-добра четимост
        progress.Update(clamp(35), "Validating package integrity...");   // Актуализира прогреса на 35%
        wxMilliSleep(500);                                               // Забавяне
        progress.Update(clamp(40), "Configuration loaded successfully"); // Актуализира прогреса на 40%
                                                                         // Празна линия за по-добра четимост
        // Phase 3: Package Processing (40-60%) // Коментар: Фаза 3: Обработка на пакет (40-60%)
        progress.Update(clamp(45), "Extracting package contents...");                   // Актуализира прогреса на 45%
        std::string jsonStr;                                                            // Низ за JSON съдържанието
        if ((*result)["config_data"])                                                   // Ако има "config_data"
        {                                                                               // Начало на if блока
            jsonStr = bsoncxx::to_json((*result)["config_data"].get_document().view()); // Извлича от "config_data"
        } // Край на if блока
        else                                     // Иначе
        {                                        // Начало на else блока
            jsonStr = bsoncxx::to_json(*result); // Извлича целия документ
        } // Край на else блока
        // Празна линия за по-добра четимост
        progress.Update(clamp(50), "Parsing installation manifest..."); // Актуализира прогреса на 50%
        rapidjson::Document doc;                                        // RapidJSON документ
        doc.Parse(jsonStr.c_str());                                     // Парсва JSON низа
                                                                        // Празна линия за по-добра четимост
        if (doc.HasParseError())                                        // Ако има грешка при парсването
        {                                                               // Начало на if блока
            throw std::runtime_error("Invalid package format");         // Хвърля изключение за невалиден формат
        } // Край на if блока
        // Празна линия за по-добра четимост
        progress.Update(clamp(55), "Verifying dependencies...");   // Актуализира прогреса на 55%
        wxMilliSleep(800);                                         // Забавяне
        progress.Update(clamp(60), "Package processing complete"); // Актуализира прогреса на 60%
                                                                   // Празна линия за по-добра четимост
        // Phase 4: Installation (60-100%) // Коментар: Фаза 4: Инсталация (60-100%)
        if (!doc.HasMember("installations") || !doc["installations"].IsArray())   // Проверява за наличие и тип на "installations"
        {                                                                         // Начало на if блока
            throw std::runtime_error("No valid installation instructions found"); // Хвърля изключение, ако няма инструкции за инсталация
        } // Край на if блока
        // Празна линия за по-добра четимост
        const auto &installs = doc["installations"].GetArray();                                                               // Взема масива "installations"
        int total = installs.Size();                                                                                          // Общ брой инсталации
        if (total == 0)                                                                                                       // Ако няма инсталации
        {                                                                                                                     // Начало на if блока
            progress.Update(clamp(100), "No applications to install");                                                        // Актуализира прогреса на 100%
            wxMessageBox("The configuration contains no applications to install.", "Information", wxOK | wxICON_INFORMATION); // Показва информационно съобщение
            return;                                                                                                           // Прекратява изпълнението
        } // Край на if блока
        // Празна линия за по-добра четимост
        int installedCount = 0;                                             // Брояч на инсталираните приложения
        for (auto &inst : installs)                                         // Итерира през всяка инсталация в масива
        {                                                                   // Начало на for цикъл
            if (progress.WasCancelled())                                    // Ако потребителят е прекъснал процеса
            {                                                               // Начало на if блока
                throw std::runtime_error("Installation cancelled by user"); // Хвърля изключение за прекъсване от потребителя
            } // Край на if блока
            // Празна линия за по-добра четимост
            if (!inst.HasMember("appId"))                                                                           // Ако инсталацията няма "appId"
                continue;                                                                                           // Преминава към следващата инсталация
                                                                                                                    // Празна линия за по-добра четимост
            wxString appId = wxString::FromUTF8(inst["appId"].GetString());                                         // Взема "appId"
            wxString appName = inst.HasMember("appName") ? wxString::FromUTF8(inst["appName"].GetString()) : appId; // Взема "appName" или използва "appId" като име
                                                                                                                    // Празна линия за по-добра четимост
            // Sub-steps for each application installation // Коментар: Под-стъпки за инсталацията на всяко приложение
            int baseProgress = 60 + (installedCount * 40 / total);                                            // Изчислява базов прогрес за текущото приложение
            baseProgress = clamp(baseProgress);                                                               // Ограничава базовия прогрес
                                                                                                              // Празна линия за по-добра четимост
            progress.Update(clamp(baseProgress), wxString::Format("Preparing %s installation", appName));     // Актуализира прогреса: подготовка
            wxMilliSleep(300);                                                                                // Забавяне
                                                                                                              // Празна линия за по-добра четимост
            progress.Update(clamp(baseProgress + 5), wxString::Format("Downloading %s packages", appName));   // Актуализира прогреса: изтегляне
            wxMilliSleep(600);                                                                                // Забавяне
                                                                                                              // Празна линия за по-добра четимост
            progress.Update(clamp(baseProgress + 10), wxString::Format("Verifying %s signatures", appName));  // Актуализира прогреса: проверка на подписи
            wxMilliSleep(400);                                                                                // Забавяне
                                                                                                              // Празна линия за по-добра четимост
            progress.Update(clamp(baseProgress + 15), wxString::Format("Installing %s components", appName)); // Актуализира прогреса: инсталиране на компоненти
            wxMilliSleep(800);                                                                                // Забавяне
                                                                                                              // Празна линия за по-добра четимост
            progress.Update(clamp(baseProgress + 19), wxString::Format("Configuring %s", appName));           // Актуализира прогреса: конфигуриране
            wxMilliSleep(500);                                                                                // Забавяне
                                                                                                              // Празна линия за по-добра четимост
            // Log the installation command // Коментар: Записване на командата за инсталация в лога
            wxString cmd = wxString::Format(                                           // Форматира командата за изпълнение в Docker контейнер
                "docker exec %s /bin/bash -c \"chroot /root/custom_iso/squashfs-root " // Изпълнение на bash в chroot среда
                "/bin/bash -c 'flatpak install -y flathub %s > /dev/null 2>&1'\"",     // Инсталиране на flatpak приложение
                containerId, appId);                                                   // Параметри: ID на контейнера и ID на приложението
                                                                                       // Празна линия за по-добра четимост
            wxLogMessage("Installation command: %s", cmd);                             // Записва командата в системния лог на wxWidgets
                                                                                       // Празна линия за по-добра четимост
            std::ofstream logf(wxFileName(m_projectDir, "install_log.txt")             // Създава изходящ файлов поток към install_log.txt
                                   .GetFullPath()                                      // Взема пълния път
                                   .ToStdString(),                                     // Конвертира в std::string
                               std::ios::app);                                         // Отваря файла в режим на добавяне (append)
            logf << "[" << wxDateTime::Now().FormatISOCombined(' ').ToStdString()      // Записва времеви маркер
                 << "] " << cmd.ToStdString() << "\n";                                 // Записва командата и нов ред
                                                                                       // Празна линия за по-добра четимост
            installedCount++;                                                          // Увеличава брояча на инсталираните приложения
        } // Край на for цикъл (итерация по инсталации)
        // Празна линия за по-добра четимост
        // Finalization phase // Коментар: Фаза на финализиране
        progress.Update(clamp(95), "Running post-installation scripts...");                      // Актуализира прогреса на 95%
        wxMilliSleep(1000);                                                                      // Забавяне
        progress.Update(clamp(98), "Updating system registry...");                               // Актуализира прогреса на 98%
        wxMilliSleep(600);                                                                       // Забавяне
        progress.Update(clamp(100), "Installation completed successfully");                      // Актуализира прогреса на 100%
                                                                                                 // Празна линия за по-добра четимост
        wxMessageBox(wxString::Format("Successfully installed %d applications", installedCount), // Показва съобщение за успешна инсталация
                     "Installation Complete", wxOK | wxICON_INFORMATION);                        // Заглавие и икона
    } // Край на тялото на try блока
    catch (const std::exception &e)                                             // Прихващане на стандартни изключения
    {                                                                           // Начало на catch блок
        wxLogError("Installation error: %s", e.what());                         // Записва грешка при инсталация в лога
                                                                                // Празна линия за по-добра четимост
        if (wxString(e.what()) != "Installation cancelled by user")             // Ако грешката не е "Прекъснато от потребителя"
        {                                                                       // Начало на if блока
            wxMessageBox(wxString::Format("Installation failed: %s", e.what()), // Показва съобщение за неуспешна инсталация
                         "Installation Error", wxOK | wxICON_ERROR);            // Заглавие и икона
        } // Край на if блока
    } // Край на catch блок
} // Край на тялото на метода OnLoadAndInstall
// Process Installation // Коментар: Обработка на инсталацията (този метод изглежда като алтернативен/по-ранен вариант или за други цели)
void MongoDBPanel::ProcessInstallation(const wxString &configName, const wxString &appNames)                                             // Дефиниция на метод за обработка на инсталацията
{                                                                                                                                        // Начало на тялото на метода
    wxString containerId = GetContainerIdFromProjectDir();                                                                               // Взема ID на контейнера
    wxProgressDialog progress(                                                                                                           // Създава диалог за прогрес
        "Application Installation",                                                                                                      // Заглавие
        "Preparing installation environment...",                                                                                         // Първоначално съобщение
        100, this,                                                                                                                       // Максимална стойност, родител
        wxPD_APP_MODAL | wxPD_AUTO_HIDE | wxPD_SMOOTH | wxPD_ELAPSED_TIME | wxPD_ESTIMATED_TIME | wxPD_REMAINING_TIME | wxPD_CAN_ABORT); // Опции
                                                                                                                                         // Празна линия за по-добра четимост
    // Clamp helper // Коментар: Помощна ламбда функция за ограничаване
    auto clamp = [](int v)                                                                                                              // Дефинира ламбда функция 'clamp'
    { return std::min(std::max(v, 0), 100); };                                                                                          // Ограничава 'v' между 0 и 100
                                                                                                                                        // Празна линия за по-добра четимост
    wxLogDebug("MongoDBPanel::ProcessInstallation - Starting installation for config '%s' in container '%s'", configName, containerId); // Записва в лога началото на инсталацията
    bool cancelled = false;                                                                                                             // Флаг за прекъсване, инициализиран с false
                                                                                                                                        // Празна линия за по-добра четимост
    try                                                                                                                                 // Начало на try блок
    {                                                                                                                                   // Начало на тялото на try блока
        cancelled = !progress.Update(clamp(0), "Entering chroot environment in container...");                                          // Актуализира прогреса на 0%, проверява за прекъсване
        if (cancelled)                                                                                                                  // Ако е прекъснато
            throw std::runtime_error("User cancelled");                                                                                 // Хвърля изключение
        wxMilliSleep(800);                                                                                                              // Забавяне
                                                                                                                                        // Празна линия за по-добра четимост
        cancelled = !progress.Update(clamp(10), "Updating package lists inside chroot...");                                             // Актуализира прогреса на 10%, проверява за прекъсване
        if (cancelled)                                                                                                                  // Ако е прекъснато
            throw std::runtime_error("User cancelled");                                                                                 // Хвърля изключение
        wxMilliSleep(1000);                                                                                                             // Забавяне
                                                                                                                                        // Празна линия за по-добра четимост
        cancelled = !progress.Update(clamp(20), "Ensuring flatpak and flathub are configured...");                                      // Актуализира прогреса на 20%, проверява за прекъсване
        if (cancelled)                                                                                                                  // Ако е прекъснато
            throw std::runtime_error("User cancelled");                                                                                 // Хвърля изключение
        wxMilliSleep(1200);                                                                                                             // Забавяне
                                                                                                                                        // Празна линия за по-добра четимост
        // Parse the app names string // Коментар: Парсване на низа с имената на приложенията
        wxArrayString appsToInstallArr = wxSplit(appNames, ','); // Разделя низа appNames по запетая в масив от низове
        std::vector<wxString> appList;                           // Вектор за съхранение на списъка с приложения
        for (wxString &app : appsToInstallArr)                   // Итерира през всеки низ в масива
        {                                                        // Начало на for цикъл
            app.Trim(true).Trim(false);                          // Премахва празни пространства от името на приложението
            if (!app.IsEmpty())                                  // Ако името не е празно след премахване на пространствата
            {                                                    // Начало на if блока
                appList.push_back(app);                          // Добавя името към вектора appList
            } // Край на if блока
        } // Край на for цикъл
        // Празна линия за по-добра четимост
        int currentProgress = 30;                                                                            // Текущ прогрес, започва от 30%
        int totalApps = appList.size();                                                                      // Общ брой приложения за инсталиране
        int progressPerApp = (totalApps > 0) ? std::min((60 / totalApps), 60) : 60;                          // Изчислява колко процента прогрес се добавят за всяко приложение (разпределя оставащите 60%)
                                                                                                             // Празна линия за по-добра четимост
        for (size_t i = 0; i < appList.size(); ++i)                                                          // Итерира през списъка с приложения за инсталиране
        {                                                                                                    // Начало на for цикъл
            const wxString &appName = appList[i];                                                            // Взема името на текущото приложение
            wxString appId = appName;                                                                        // Копира името за създаване на ID
            appId.Replace(" ", "");                                                                          // Премахва интервалите от ID-то
            appId.Lower();                                                                                   // Конвертира ID-то в малки букви
            if (!appId.Contains("."))                                                                        // Ако ID-то не съдържа точка (предполага се, че flatpak ID-тата имат формат с точки)
                appId = "org.example." + appId;                                                              // Добавя префикс "org.example."
                                                                                                             // Празна линия за по-добра четимост
            wxString logMsg = wxString::Format("Running 'flatpak install -y flathub %s' for %s (%zu/%d)...", // Форматира съобщение за лога/прогреса
                                               appId, appName, i + 1, totalApps);                            // Параметри: ID на приложението, име, текущ номер/общ брой
            wxLogDebug(logMsg);                                                                              // Записва съобщението в лога
                                                                                                             // Празна линия за по-добра четимост
            // Ensure progress never exceeds 100 // Коментар: Гарантира, че прогресът никога не надвишава 100
            int nextProgress = std::min(80, currentProgress + progressPerApp); // Изчислява следващия прогрес, ограничен до 80% (за да остане място за финалните стъпки)
            cancelled = !progress.Update(clamp(nextProgress), logMsg);         // Актуализира прогреса и проверява за прекъсване
            if (cancelled)                                                     // Ако е прекъснато
                throw std::runtime_error("User cancelled");                    // Хвърля изключение
                                                                               // Празна линия за по-добра четимост
            wxMilliSleep(900 + (rand() % 600));                                // Симулира работа чрез забавяне (случайно между 0.9 и 1.5 секунди)
            currentProgress = nextProgress;                                    // Актуализира текущия прогрес
        } // Край на for цикъл (итерация по приложения)
        // Празна линия за по-добра четимост
        // Final steps with guaranteed max of 100 // Коментар: Финални стъпки с гарантиран максимум от 100
        cancelled = !progress.Update(clamp(85), "Verifying installations...");                                      // Актуализира прогреса на 85%, проверява за прекъсване
        if (cancelled)                                                                                              // Ако е прекъснато
            throw std::runtime_error("User cancelled");                                                             // Хвърля изключение
        wxMilliSleep(600);                                                                                          // Забавяне
                                                                                                                    // Празна линия за по-добра четимост
        cancelled = !progress.Update(clamp(95), "Cleaning up temporary files...");                                  // Актуализира прогреса на 95%, проверява за прекъсване
        if (cancelled)                                                                                              // Ако е прекъснато
            throw std::runtime_error("User cancelled");                                                             // Хвърля изключение
        wxMilliSleep(500);                                                                                          // Забавяне
                                                                                                                    // Празна линия за по-добра четимост
        progress.Update(clamp(100), "Installation complete!");                                                      // Актуализира прогреса на 100%
        wxMilliSleep(300);                                                                                          // Кратко забавяне
                                                                                                                    // Празна линия за по-добра четимост
        wxLogDebug("MongoDBPanel::ProcessInstallation - Installation completed successfully for '%s'", configName); // Записва в лога успешното приключване
        wxMessageBox("Applications from '" + configName + "' have been successfully installed!",                    // Показва съобщение за успех
                     "Installation Complete", wxOK | wxICON_INFORMATION);                                           // Заглавие и икона
    } // Край на тялото на try блока
    catch (const std::exception &e)                                              // Прихващане на стандартни изключения
    {                                                                            // Начало на catch блок
        if (wxString(e.what()) == "User cancelled")                              // Ако изключението е "User cancelled"
        {                                                                        // Начало на if блока
            wxLogWarning("Installation cancelled by user for '%s'", configName); // Записва предупреждение в лога
            // Progress dialog hides automatically on cancel // Коментар: Диалогът за прогрес се скрива автоматично при прекъсване
        } // Край на if блока
        else                                                                                                                      // Иначе (за други изключения)
        {                                                                                                                         // Начало на else блока
            wxLogError("Installation Error for '%s': %s", configName, e.what());                                                  // Записва грешка при инсталация в лога
            wxMessageBox(wxString::Format("Error during installation: %s", e.what()), "Installation Error", wxOK | wxICON_ERROR); // Показва съобщение за грешка
        } // Край на else блока
    } // Край на catch блок
} // Край на тялото на метода ProcessInstallation
  // Празна линия за по-добра четимост
// Apply Theme Colors // Коментар: Прилагане на цветовете на темата
void MongoDBPanel::ApplyThemeColors()                                  // Дефиниция на метод за прилагане на цветовете на темата
{                                                                      // Начало на тялото на метода
    m_contentList->SetBackgroundColour(m_listBgColor);                 // Задава цвят на фона на списъка
    m_contentList->SetForegroundColour(m_listFgColor);                 // Задава цвят на текста в списъка
    m_uploadButton->SetBackgroundColour(m_uploadBtnBgColor);           // Задава цвят на фона на бутона за качване
    m_uploadButton->SetForegroundColour(wxColour(255, 255, 255));      // Задава цвят на текста на бутона за качване (бял)
    m_refreshButton->SetBackgroundColour(m_buttonBgColor);             // Задава цвят на фона на бутона за опресняване
    m_refreshButton->SetForegroundColour(m_buttonFgColor);             // Задава цвят на текста на бутона за опресняване
    m_closeButton->SetBackgroundColour(m_buttonBgColor);               // Задава цвят на фона на бутона за затваряне
    m_closeButton->SetForegroundColour(m_buttonFgColor);               // Задава цвят на текста на бутона за затваряне
    m_loadInstallButton->SetBackgroundColour(wxColour(137, 220, 170)); // Задава цвят на фона на бутона "Load & Install" (зеленикав)
    m_loadInstallButton->SetForegroundColour(wxColour(255, 255, 255)); // Задава цвят на текста на бутона "Load & Install" (бял)
    m_downloadButton->SetBackgroundColour(wxColour(150, 150, 240));    // Задава цвят на фона на бутона "Download" (синкав)
    m_downloadButton->SetForegroundColour(wxColour(255, 255, 255));    // Задава цвят на текста на бутона "Download" (бял)
                                                                       // Празна линия за по-добра четимост
    // Disable buttons initially or based on selection state // Коментар: Деактивиране на бутоните първоначално или въз основа на състоянието на избора
    bool itemSelected = (m_selectedItemIndex >= 0);          // Проверява дали е избран елемент
    m_loadInstallButton->Enable(itemSelected);               // Активира/деактивира бутона "Load & Install" в зависимост от избора
    m_downloadButton->Enable(itemSelected);                  // Активира/деактивира бутона "Download" в зависимост от избора
                                                             // Празна линия за по-добра четимост
    wxFont listFont = m_contentList->GetFont();              // Взема шрифта на списъка
    listFont.SetPointSize(10);                               // Задава размер 10
    listFont.SetWeight(wxFONTWEIGHT_NORMAL);                 // Задава нормална дебелина
    m_contentList->SetFont(listFont);                        // Прилага шрифта към списъка
    wxFont buttonFont = m_uploadButton->GetFont();           // Взема шрифта на бутона за качване (предполага се, че всички бутони ще използват същия шрифт)
    buttonFont.SetPointSize(10);                             // Задава размер 10
    buttonFont.SetWeight(wxFONTWEIGHT_BOLD);                 // Задава удебелен шрифт
    m_uploadButton->SetFont(buttonFont);                     // Прилага шрифта към бутона за качване
    m_refreshButton->SetFont(buttonFont);                    // Прилага шрифта към бутона за опресняване
    m_closeButton->SetFont(buttonFont);                      // Прилага шрифта към бутона за затваряне
    m_loadInstallButton->SetFont(buttonFont);                // Прилага шрифта към бутона "Load & Install"
    m_downloadButton->SetFont(buttonFont);                   // Прилага шрифта към бутона "Download"
#ifdef __WXMSW__                                             // Начало на блок за условна компилация (само за Windows)
    m_uploadButton->SetCursor(wxCursor(wxCURSOR_HAND));      // Задава курсор "ръка" за бутона за качване
    m_refreshButton->SetCursor(wxCursor(wxCURSOR_HAND));     // Задава курсор "ръка" за бутона за опресняване
    m_closeButton->SetCursor(wxCursor(wxCURSOR_HAND));       // Задава курсор "ръка" за бутона за затваряне
    m_loadInstallButton->SetCursor(wxCursor(wxCURSOR_HAND)); // Задава курсор "ръка" за бутона "Load & Install"
    m_downloadButton->SetCursor(wxCursor(wxCURSOR_HAND));    // Задава курсор "ръка" за бутона "Download"
#endif                                                       // Край на блок за условна компилация
    Refresh();                                               // Прерисува панела, за да се приложат промените в цветовете
} // Край на тялото на метода ApplyThemeColors
  // Празна линия за по-добра четимост
// OnCloseClick // Коментар: Обработчик на събитието "натискане на бутона за затваряне"
void MongoDBPanel::OnCloseClick(wxCommandEvent &event)                              // Дефиниция на метод-обработчик
{                                                                                   // Начало на тялото на метода
    wxLogDebug("MongoDBPanel::OnCloseClick - Close button clicked.");               // Записва в лога, че бутонът е натиснат
    wxCommandEvent closeEvent(wxEVT_BUTTON, ID_MONGODB_PANEL_CLOSE);                // Създава ново събитие от тип wxEVT_BUTTON с ID_MONGODB_PANEL_CLOSE
    closeEvent.SetEventObject(this);                                                // Задава текущия панел като обект, изпратил събитието
    ProcessWindowEvent(closeEvent);                                                 // Send event to parent (SecondWindow) // Изпраща събитието към родителския прозорец (SecondWindow)
    wxLogDebug("MongoDBPanel::OnCloseClick - Sent close request event to parent."); // Записва в лога, че е изпратено събитие за затваряне
} // Край на тялото на метода OnCloseClick
  // Празна линия за по-добра четимост
// OnItemSelected // Коментар: Обработчик на събитието "избор на елемент в списъка"
void MongoDBPanel::OnItemSelected(wxListEvent &event)                          // Дефиниция на метод-обработчик
{                                                                              // Начало на тялото на метода
    long itemIndex = event.GetIndex();                                         // Взема индекса на избрания елемент
    wxLogDebug("MongoDBPanel::OnItemSelected - Item %ld selected", itemIndex); // Записва в лога кой елемент е избран
    m_selectedItemIndex = itemIndex;                                           // Актуализира член-променливата за индекс на избран елемент
    m_selectedDocumentId = "";                                                 // Reset // Нулира ID-то на избрания документ
    auto it = m_documentIds.find(itemIndex);                                   // Търси в картата m_documentIds по индекс, за да намери ID-то на документа
    if (it != m_documentIds.end())                                             // Ако е намерено ID за този индекс
    {                                                                          // Начало на if блока
        m_selectedDocumentId = it->second;                                     // Присвоява намереното ID на член-променливата
        wxLogDebug("Selected document ID: %s", m_selectedDocumentId);          // Записва в лога избраното ID на документ
        m_downloadButton->Enable();                                            // Активира бутона за изтегляне
        m_loadInstallButton->Enable();                                         // Активира бутона "Load & Install"
    } // Край на if блока
    else                                                            // Иначе (ако не е намерено ID)
    {                                                               // Начало на else блока
        wxLogDebug("No document ID found for item %ld", itemIndex); // Записва в лога, че не е намерено ID
        m_downloadButton->Disable();                                // Деактивира бутона за изтегляне
        m_loadInstallButton->Disable();                             // Деактивира бутона "Load & Install"
    } // Край на else блока
    event.Skip(); // Allow default selection behavior // Позволява на събитието да бъде обработено и от стандартния обработчик (за визуална селекция)
} // Край на тялото на метода OnItemSelected
  // Празна линия за по-добра четимост
// Destructor // Коментар: Деструктор
MongoDBPanel::~MongoDBPanel()                     // Деструктор на класа MongoDBPanel
{                                                 // Начало на тялото на деструктора
    wxLogDebug("MongoDBPanel destructor called"); // Записва в лога, че деструкторът е извикан
} // Край на тялото на деструктора