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
#include "ContainerCommand.h"                   // Команди в chroot средата на Docker контейнера (Flatpak инсталации)
#include <wx/weakref.h>                        // wxWeakRef за безопасен достъп до панела от асинхронни callback-и
#include <memory>                              // std::unique_ptr
#include <cstring>                             // strlen
#include <rapidjson/document.h>                // Включва заглавния файл на RapidJSON за парсване и работа с JSON документи
#include <rapidjson/error/en.h>                // Включва заглавния файл на RapidJSON за английски съобщения за грешки при парсване

#ifdef LINUXISOPRO_HAS_MONGODB                 // MongoDB поддръжката е по избор (виж CMakeLists.txt)
#include <mongocxx/instance.hpp>               // MongoDB драйверът изисква точно една инстанция на процес
#include <mongocxx/uri.hpp>                    // Включва заглавния файл на MongoDB C++ драйвера за URI на връзката
#include <mongocxx/client.hpp>                 // Включва заглавния файл на MongoDB C++ драйвера за клиент на MongoDB
#include <mongocxx/collection.hpp>             // Включва заглавния файл на MongoDB C++ драйвера за колекции в MongoDB
#include <mongocxx/exception/exception.hpp>    // Включва заглавния файл на MongoDB C++ драйвера за изключения
#include <bsoncxx/builder/basic/document.hpp>  // Включва заглавния файл на BSONCXX за базов конструктор на документи
#include <bsoncxx/builder/basic/kvp.hpp>       // Включва заглавния файл на BSONCXX за създаване на двойки ключ-стойност (key-value pair)
#include <bsoncxx/json.hpp>                    // Включва заглавния файл на BSONCXX за конвертиране от/към JSON
#include <bsoncxx/types.hpp>                   // Включва заглавния файл на BSONCXX за BSON типове данни
#include <bsoncxx/exception/exception.hpp>     // Включва заглавния файл на BSONCXX за изключения
#endif

namespace
{
    // Примерна конфигурация, която се създава, ако папката db_files е празна (показва очаквания формат)
    const char *const EXAMPLE_CONFIG_JSON = R"({
  "config_name": "Example - everyday apps",
  "installations": [
    { "appId": "org.mozilla.firefox", "appName": "Firefox" },
    { "appId": "org.videolan.VLC", "appName": "VLC" },
    { "appId": "org.gimp.GIMP", "appName": "GIMP" }
  ]
}
)";

    // Връща обекта с конфигурацията: MongoDB документите я пазят в полето "config_data"
    const rapidjson::Value &ConfigRoot(const rapidjson::Document &doc)
    {
        if (doc.HasMember("config_data") && doc["config_data"].IsObject())
            return doc["config_data"];
        return doc;
    }

#ifdef LINUXISOPRO_HAS_MONGODB
    const char *const MONGODB_DATABASE = "testdb";         // База данни в MongoDB Atlas
    const char *const MONGODB_COLLECTION = "config_files"; // Колекция с качените конфигурации

    // Connection string-ът се чете от променливата на средата LINUXISOPRO_MONGODB_URI,
    // за да не се пази парола в изходния код. Празен низ = MongoDB е изключен.
    std::string GetMongoUri()
    {
        wxString uri;
        if (!wxGetEnv("LINUXISOPRO_MONGODB_URI", &uri))
            return std::string();
        uri.Trim(true).Trim(false);
        return std::string(uri.ToUTF8().data());
    }

    std::unique_ptr<mongocxx::client> ConnectToMongo(const std::string &uri)
    {
        static mongocxx::instance instance{}; // Създава се веднъж, преди първия клиент
        mongocxx::options::client clientOptions;
        clientOptions.server_api_opts(mongocxx::options::server_api{mongocxx::options::server_api::version::k_version_1});
        return std::make_unique<mongocxx::client>(mongocxx::uri{uri}, clientOptions);
    }

    mongocxx::collection ConfigCollection(mongocxx::client &client)
    {
        return client[MONGODB_DATABASE][MONGODB_COLLECTION];
    }

    // MongoDB ObjectId е 24 шестнадесетични символа; локалните файлове имат други имена
    bool LooksLikeObjectId(const wxString &id)
    {
        if (id.length() != 24)
            return false;
        for (wxUniChar c : id)
        {
            if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')))
                return false;
        }
        return true;
    }
#endif
}

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
// Път до папката с локалните копия на конфигурациите (създава я при нужда)
wxString MongoDBPanel::GetDbDirPath() const
{
    wxString dbDirPath = wxFileName(m_projectDir, "db_files").GetFullPath(); // <проект>/db_files
    if (!wxDirExists(dbDirPath))                                             // Ако папката не съществува
        wxMkdir(dbDirPath);                                                  // Създава я
    return dbDirPath;
}

// Изчиства списъка и състоянието на избрания елемент
void MongoDBPanel::ResetList()
{
    m_contentList->DeleteAllItems(); // Изтрива всички редове
    m_documentIds.clear();           // Изчиства картата ред -> ID на документ
    m_selectedItemIndex = -1;        // Няма избран елемент
    m_selectedDocumentId = "";
    m_downloadButton->Disable();     // Бутоните изискват избран елемент
    m_loadInstallButton->Disable();
}

// Добавя ред в списъка (празно documentId = информационен ред без действия)
void MongoDBPanel::AddListItem(const wxString &configName, const wxString &appsList, const wxString &documentId)
{
    long index = m_contentList->GetItemCount();                   // Новият ред е последен
    long actualIndex = m_contentList->InsertItem(index, configName);
    m_contentList->SetItem(actualIndex, 1, appsList);
    if (!documentId.IsEmpty())
        m_documentIds[actualIndex] = documentId;                  // Запомня ID-то за Download / Load & Install
    m_contentList->SetItemBackgroundColour(actualIndex, index % 2 == 0 ? m_listBgColor : m_listBgColor.ChangeLightness(105)); // Редуващи се цветове
    m_contentList->SetItemTextColour(actualIndex, m_listFgColor);
}

// Зарежда списъка от MongoDB (ако е конфигуриран) или от локалните файлове
void MongoDBPanel::LoadMongoDBContent()
{
    ResetList();

#ifdef LINUXISOPRO_HAS_MONGODB
    const std::string uri = GetMongoUri();
    if (!uri.empty())
    {
        try
        {
            wxLogDebug("MongoDBPanel::LoadMongoDBContent - Connecting to MongoDB...");
            std::unique_ptr<mongocxx::client> client = ConnectToMongo(uri);
            mongocxx::collection coll = ConfigCollection(*client);
            mongocxx::cursor cursor = coll.find({});
            int count = 0;

            for (auto &&docView : cursor) // Обхожда всички документи в колекцията
            {
                if (!docView["_id"] || docView["_id"].type() != bsoncxx::type::k_oid)
                    continue;
                wxString documentId = wxString::FromUTF8(docView["_id"].get_oid().value.to_string().c_str());

                // Името и приложенията се извличат от JSON представянето на документа (същата логика като за локалните файлове)
                wxString configName, appsList;
                ExtractConfigSummary(bsoncxx::to_json(docView), configName, appsList);
                if (configName.IsEmpty() && docView["original_filename"] && docView["original_filename"].type() == bsoncxx::type::k_string)
                {
                    auto nameView = docView["original_filename"].get_string().value;
                    configName = wxString::FromUTF8(nameView.data(), nameView.size());
                    configName.Replace(".json", "");
                }
                if (configName.IsEmpty())
                    configName = documentId;

                AddListItem(configName, appsList, documentId);
                count++;
            }

            if (count == 0)
                AddListItem("No configurations found in MongoDB.", "-", "");
            wxLogDebug("MongoDBPanel::LoadMongoDBContent - Loaded %d documents.", count);
            return;
        }
        catch (const std::exception &e) // mongocxx::exception наследява std::exception
        {
            wxLogError("MongoDB error: %s", e.what());
            wxMessageBox(wxString::Format("Could not load configurations from MongoDB:\n%s\n\nShowing the local copies instead.", e.what()),
                         "MongoDB Error", wxOK | wxICON_WARNING, this);
            ResetList();
        }
    }
#endif

    LoadFromLocalFiles(); // Без MongoDB се използват файловете в <проект>/db_files
}

// Зарежда конфигурациите от локалните JSON файлове в <проект>/db_files
void MongoDBPanel::LoadFromLocalFiles()
{
    wxString dbDirPath = GetDbDirPath();
    wxDir dir(dbDirPath);
    if (!dir.IsOpened())
    {
        wxLogError("Failed to open DB directory: %s", dbDirPath);
        return;
    }

    wxString filename;
    bool hasFiles = dir.GetFirst(&filename, "*.json", wxDIR_FILES);
    if (!hasFiles) // Ако папката е празна, създава пример с очаквания формат
    {
        CreateSampleDBFiles(dbDirPath);
        hasFiles = dir.GetFirst(&filename, "*.json", wxDIR_FILES);
    }

    int count = 0;
    while (hasFiles)
    {
        wxString filePath = wxFileName(dbDirPath, filename).GetFullPath();
        wxString configName, appsList;
        if (ParseJsonFile(filePath, configName, appsList))
        {
            AddListItem(configName, appsList, wxFileName(filename).GetName()); // ID на локалния документ = името на файла
            count++;
        }
        else
        {
            wxLogWarning("Skipping %s: not a valid configuration file.", filePath);
        }
        hasFiles = dir.GetNext(&filename);
    }

    if (count == 0)
        AddListItem("No configurations found in local files.", "-", "");
}

// Създава примерна конфигурация с реални Flathub приложения
void MongoDBPanel::CreateSampleDBFiles(const wxString &dbDirPath)
{
    wxString samplePath = wxFileName(dbDirPath, "example_config.json").GetFullPath();
    wxFile file;
    if (!file.Create(samplePath, true) || !file.Write(EXAMPLE_CONFIG_JSON, strlen(EXAMPLE_CONFIG_JSON)))
        wxLogError("Failed to create sample configuration: %s", samplePath);
}

// Чете целия файл като байтове (JSON файловете са UTF-8)
bool MongoDBPanel::ReadFileBytes(const wxString &filePath, std::string &content)
{
    wxFile file;
    if (!wxFileExists(filePath) || !file.Open(filePath))
        return false;
    wxFileOffset length = file.Length();
    if (length < 0)
        return false;
    content.assign(static_cast<size_t>(length), '\0');
    return length == 0 || file.Read(&content[0], content.size()) == static_cast<ssize_t>(content.size());
}

// Извлича името на конфигурацията и списъка с приложения.
// Поддържани формати: {"config_name", "installations": [{"appId", "appName"}]}, {"config_name", "apps": "A, B"}
// и същите, обвити в {"config_data": {...}}, както се пазят в MongoDB.
bool MongoDBPanel::ExtractConfigSummary(const std::string &json, wxString &configName, wxString &appsList)
{
    configName.clear();
    appsList.clear();

    rapidjson::Document doc;
    doc.Parse(json.c_str());
    if (doc.HasParseError() || !doc.IsObject())
        return false;

    const rapidjson::Value &root = ConfigRoot(doc);
    if (root.HasMember("config_name") && root["config_name"].IsString())
        configName = wxString::FromUTF8(root["config_name"].GetString());
    else if (doc.HasMember("config_name") && doc["config_name"].IsString())
        configName = wxString::FromUTF8(doc["config_name"].GetString());

    if (root.HasMember("installations") && root["installations"].IsArray())
    {
        for (const auto &install : root["installations"].GetArray())
        {
            if (!install.IsObject())
                continue;
            const char *key = (install.HasMember("appName") && install["appName"].IsString()) ? "appName" : "appId";
            if (install.HasMember(key) && install[key].IsString())
            {
                if (!appsList.IsEmpty())
                    appsList += ", ";
                appsList += wxString::FromUTF8(install[key].GetString());
            }
        }
    }
    if (appsList.IsEmpty() && root.HasMember("apps") && root["apps"].IsString())
        appsList = wxString::FromUTF8(root["apps"].GetString());

    return !configName.IsEmpty() || !appsList.IsEmpty();
}

// Парсва локален JSON файл с конфигурация
bool MongoDBPanel::ParseJsonFile(const wxString &filePath, wxString &configName, wxString &appsList)
{
    std::string content;
    if (!ReadFileBytes(filePath, content))
    {
        wxLogError("Failed to open JSON file: %s", filePath);
        return false;
    }
    if (!ExtractConfigSummary(content, configName, appsList))
        return false;
    if (configName.IsEmpty())
        configName = wxFileName(filePath).GetName(); // Без име се показва името на файла
    return true;
}

// Зарежда JSON съдържанието на документ: от MongoDB (ако е конфигуриран и ID-то е ObjectId) или от db_files
bool MongoDBPanel::LoadConfigJson(const wxString &documentId, std::string &jsonOut)
{
#ifdef LINUXISOPRO_HAS_MONGODB
    const std::string uri = GetMongoUri();
    if (!uri.empty() && LooksLikeObjectId(documentId))
    {
        try
        {
            std::unique_ptr<mongocxx::client> client = ConnectToMongo(uri);
            mongocxx::collection coll = ConfigCollection(*client);
            bsoncxx::builder::basic::document filter{};
            filter.append(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid(documentId.ToStdString())));
            auto result = coll.find_one(filter.view());
            if (result)
            {
                bsoncxx::document::view view = result->view();
                if (view["config_data"] && view["config_data"].type() == bsoncxx::type::k_document)
                    jsonOut = bsoncxx::to_json(view["config_data"].get_document().view());
                else
                    jsonOut = bsoncxx::to_json(view);
                return true;
            }
        }
        catch (const std::exception &e)
        {
            wxLogWarning("MongoDB lookup failed, trying the local copy: %s", e.what());
        }
    }
#endif
    return ReadFileBytes(wxFileName(GetDbDirPath(), documentId + ".json").GetFullPath(), jsonOut);
}

// Качва JSON конфигурация: в MongoDB (ако е конфигуриран) и като локално копие в db_files
void MongoDBPanel::OnUploadJson(wxCommandEvent &event)
{
    wxFileDialog openFileDialog(this, "Select JSON file to upload", "", "",
                                "JSON files (*.json)|*.json", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (openFileDialog.ShowModal() == wxID_CANCEL)
        return;

    wxString filePath = openFileDialog.GetPath();
    wxString fileName = openFileDialog.GetFilename();

    std::string jsonContent;
    if (!ReadFileBytes(filePath, jsonContent))
    {
        wxMessageBox("Failed to open file: " + filePath, "File Error", wxOK | wxICON_ERROR, this);
        return;
    }

    wxString configName, appsList;
    if (!ExtractConfigSummary(jsonContent, configName, appsList))
    {
        wxMessageBox("The selected file is not a valid configuration.\n\n"
                     "Expected JSON like:\n{ \"config_name\": \"My apps\", \"installations\": [ { \"appId\": \"org.mozilla.firefox\", \"appName\": \"Firefox\" } ] }",
                     "Invalid Configuration", wxOK | wxICON_ERROR, this);
        return;
    }

#ifdef LINUXISOPRO_HAS_MONGODB
    const std::string uri = GetMongoUri();
    if (!uri.empty())
    {
        try
        {
            using bsoncxx::builder::basic::kvp;
            bsoncxx::document::value docValue = bsoncxx::from_json(jsonContent);
            std::unique_ptr<mongocxx::client> client = ConnectToMongo(uri);
            mongocxx::collection coll = ConfigCollection(*client);

            bsoncxx::builder::basic::document builder{};
            builder.append(kvp("config_data", docValue.view()));
            builder.append(kvp("config_name", std::string(configName.ToUTF8().data())));
            builder.append(kvp("apps", std::string(appsList.ToUTF8().data())));
            builder.append(kvp("original_filename", std::string(fileName.ToUTF8().data())));
            builder.append(kvp("upload_timestamp", bsoncxx::types::b_date{std::chrono::system_clock::now()}));
            wxString containerId = GetContainerIdFromProjectDir();
            if (!containerId.IsEmpty())
                builder.append(kvp("container_id", std::string(containerId.ToUTF8().data())));

            auto result = coll.insert_one(builder.view());
            if (result)
            {
                wxString oidStr = wxString::FromUTF8(result->inserted_id().get_oid().value.to_string().c_str());
                wxCopyFile(filePath, wxFileName(GetDbDirPath(), oidStr + ".json").GetFullPath(), true); // Локално копие със същото ID
                wxMessageBox("'" + fileName + "' was uploaded to MongoDB.\nObjectID: " + oidStr,
                             "Upload Success", wxOK | wxICON_INFORMATION, this);
                LoadMongoDBContent();
                return;
            }
            wxLogError("MongoDBPanel::OnUploadJson - insert_one did not return a result.");
        }
        catch (const std::exception &e)
        {
            wxLogError("MongoDB upload error: %s", e.what());
            wxMessageBox(wxString::Format("Uploading to MongoDB failed:\n%s\n\nThe file will be saved locally instead.", e.what()),
                         "MongoDB Error", wxOK | wxICON_WARNING, this);
        }
    }
#endif

    SaveToLocalFile(filePath, jsonContent);
}

// Запазва конфигурацията в <проект>/db_files
void MongoDBPanel::SaveToLocalFile(const wxString &sourcePath, const std::string &jsonContent)
{
    wxString fileId = wxFileName(sourcePath).GetName() + "_" + wxDateTime::Now().Format("%Y%m%d%H%M%S"); // Уникално име
    wxString dbFilePath = wxFileName(GetDbDirPath(), fileId + ".json").GetFullPath();

    wxFile file;
    if (file.Create(dbFilePath, true) && file.Write(jsonContent.data(), jsonContent.size()) == jsonContent.size())
    {
        file.Close();
        wxMessageBox("The configuration was saved in the project folder:\n" + dbFilePath,
                     "Saved Locally", wxOK | wxICON_INFORMATION, this);
        ResetList();
        LoadFromLocalFiles();
    }
    else
    {
        wxLogError("Failed to save file locally to: %s", dbFilePath);
        wxMessageBox("Failed to save the file locally. Please check the permissions of the project folder.",
                     "Local Storage Error", wxOK | wxICON_ERROR, this);
    }
}

// Записва избраната конфигурация като JSON файл, избран от потребителя
void MongoDBPanel::OnDownloadSelected(wxCommandEvent &event)
{
    if (m_selectedItemIndex < 0 || m_selectedDocumentId.IsEmpty())
    {
        wxMessageBox("Please select a configuration to download.", "Selection Required", wxOK | wxICON_INFORMATION, this);
        return;
    }

    wxString suggestedName = m_contentList->GetItemText(m_selectedItemIndex);
    suggestedName.Replace(" ", "_");
    if (!suggestedName.EndsWith(".json"))
        suggestedName += ".json";

    wxFileDialog saveDialog(this, "Save Configuration File", "", suggestedName,
                            "JSON files (*.json)|*.json", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (saveDialog.ShowModal() == wxID_CANCEL)
        return;
    wxString savePath = saveDialog.GetPath();

    std::string json;
    wxBusyCursor busy; // Връзката с MongoDB може да отнеме няколко секунди
    if (!LoadConfigJson(m_selectedDocumentId, json))
    {
        wxMessageBox("The configuration could not be found in MongoDB or in the local files.",
                     "Download Error", wxOK | wxICON_ERROR, this);
        return;
    }

    wxFile out;
    if (out.Create(savePath, true) && out.Write(json.data(), json.size()) == json.size())
        wxMessageBox("The configuration was saved to:\n" + savePath, "Download Complete", wxOK | wxICON_INFORMATION, this);
    else
        wxMessageBox("Could not write " + savePath, "Download Error", wxOK | wxICON_ERROR, this);
}

void MongoDBPanel::OnRefresh(wxCommandEvent &event)
{
    LoadMongoDBContent(); // Презарежда списъка
}

// Инсталира приложенията от избраната конфигурация в ISO образа (flatpak install в chroot средата)
void MongoDBPanel::OnLoadAndInstall(wxCommandEvent &event)
{
    if (m_selectedItemIndex < 0 || m_selectedDocumentId.IsEmpty())
    {
        wxMessageBox("Please select a configuration to install.", "Selection Required", wxOK | wxICON_WARNING, this);
        return;
    }

    wxString containerId = GetContainerIdFromProjectDir();
    if (containerId.IsEmpty())
    {
        wxMessageBox("No container ID found. Wait until the initial setup has finished.", "Missing Container ID", wxOK | wxICON_ERROR, this);
        return;
    }

    std::string json;
    if (!LoadConfigJson(m_selectedDocumentId, json))
    {
        wxMessageBox("The configuration could not be loaded.", "Installation Error", wxOK | wxICON_ERROR, this);
        return;
    }

    rapidjson::Document doc;
    doc.Parse(json.c_str());
    if (doc.HasParseError() || !doc.IsObject())
    {
        wxMessageBox("The configuration is not valid JSON.", "Installation Error", wxOK | wxICON_ERROR, this);
        return;
    }

    // Събира Flatpak ID-тата от "installations"
    const rapidjson::Value &root = ConfigRoot(doc);
    wxArrayString appIds;
    wxArrayString skipped;
    if (root.HasMember("installations") && root["installations"].IsArray())
    {
        for (const auto &install : root["installations"].GetArray())
        {
            if (!install.IsObject() || !install.HasMember("appId") || !install["appId"].IsString())
                continue;
            wxString appId = wxString::FromUTF8(install["appId"].GetString());
            if (ContainerCommand::IsValidFlatpakId(appId))
                appIds.Add(appId);
            else
                skipped.Add(appId);
        }
    }

    if (appIds.IsEmpty())
    {
        wxMessageBox("This configuration has no \"installations\" with Flatpak app IDs (for example org.mozilla.firefox).",
                     "Nothing to Install", wxOK | wxICON_INFORMATION, this);
        return;
    }

    wxString question = wxString::Format("Install %d application(s) into the ISO?\n\n%s", (int)appIds.GetCount(), wxJoin(appIds, '\n'));
    if (!skipped.IsEmpty())
        question += "\n\nSkipped (invalid app IDs):\n" + wxJoin(skipped, '\n');
    if (wxMessageBox(question, "Load & Install", wxYES_NO | wxICON_QUESTION, this) != wxYES)
        return;

    m_loadInstallButton->Disable();
    m_loadInstallButton->SetLabel("Installing...");

    wxWeakRef<MongoDBPanel> self(this);
    int total = (int)appIds.GetCount();
    long pid = ContainerCommand::RunInChrootAsync(containerId, ContainerCommand::FlatpakInstallScript(appIds),
                                                  [self, containerId, total](int status)
    {
        if (!self) // Панелът е унищожен, докато инсталацията е вървяла
            return;
        self->m_loadInstallButton->SetLabel("Load & Install");
        self->m_loadInstallButton->Enable(self->m_selectedItemIndex >= 0);

        if (status == 0)
        {
            wxMessageBox(wxString::Format("All %d application(s) were installed into the ISO.", total),
                         "Installation Complete", wxOK | wxICON_INFORMATION, self);
            return;
        }

        wxString reason = (status == ContainerCommand::FLATPAK_MISSING_EXIT_CODE)
                              ? wxString("Flatpak is not available in this image.")
                              : wxString::Format("%d of %d installation(s) failed.", status, total);
        wxMessageBox(reason + "\n\nLast lines of the log:\n" + ContainerCommand::FlatpakInstallLogTail(containerId),
                     "Installation Problem", wxOK | wxICON_WARNING, self);
    });

    if (pid == 0)
    {
        m_loadInstallButton->SetLabel("Load & Install");
        m_loadInstallButton->Enable();
        wxMessageBox("Could not start the installation. Is Docker Desktop running?", "Installation Error", wxOK | wxICON_ERROR, this);
    }
}
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