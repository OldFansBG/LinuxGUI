// MongoDBPanel.h // Име на заглавния файл
#ifndef MONGODBPANEL_H // Предварителна директива за условна компилация (include guard) - ако MONGODBPANEL_H не е дефиниран
#define MONGODBPANEL_H // Дефинира MONGODBPANEL_H, за да предотврати многократно включване на файла

#include <wx/wx.h>       // Включва основния заглавен файл на wxWidgets (обикновено wx.h или wxprec.h)
#include <wx/panel.h>    // Включва заглавния файл за wxPanel (базов клас за панели)
#include <wx/listctrl.h> // Включва заглавния файл за wxListCtrl (контрола за списъци)
#include <wx/button.h>   // Включва заглавния файл за wxButton (контрола за бутони)
#include <wx/sizer.h>    // Включва заглавния файл за wxSizer (мениджъри на подреждане)
#include <map>           // Включва стандартната C++ библиотека за асоциативни контейнери (карти)
#include <wx/filename.h> // <-- Add include for wxFileName // Включва заглавния файл на wxWidgets за работа с имена на файлове <-- Коментар: Добавяне на включване за wxFileName
#include <wx/file.h>     // <-- Add include for wxFile // Включва заглавния файл на wxWidgets за файлови операции <-- Коментар: Добавяне на включване за wxFile
#include <wx/dir.h>      // <-- Add include for wxDir // Включва заглавния файл на wxWidgets за работа с директории <-- Коментар: Добавяне на включване за wxDir

// MongoDB specific includes // Коментар: Специфични включвания за MongoDB
// These are kept commented out to prevent build errors unless really needed // Коментар: Те са оставени коментирани, за да се предотвратят грешки при компилация, освен ако не са наистина необходими
// #include <mongocxx/uri.hpp> // Коментирано включване за MongoDB URI
// #include <mongocxx/client.hpp> // Коментирано включване за MongoDB клиент
// #include <mongocxx/collection.hpp> // Коментирано включване за MongoDB колекции
// #include <mongocxx/exception/exception.hpp> // Коментирано включване за MongoDB изключения
// #include <bsoncxx/builder/stream/document.hpp> // Коментирано включване за BSON поточен конструктор на документи
// #include <bsoncxx/builder/basic/document.hpp> // Коментирано включване за BSON базов конструктор на документи
// #include <bsoncxx/builder/basic/kvp.hpp> // Коментирано включване за BSON ключ-стойност двойки
// #include <bsoncxx/json.hpp> // Коментирано включване за BSON/JSON конвертиране
// #include <bsoncxx/types.hpp> // Коментирано включване за BSON типове
// #include <bsoncxx/exception/exception.hpp> // Коментирано включване за BSON изключения
// #include <bsoncxx/string/view_or_value.hpp> // Коментирано включване за BSON низове

// Forward Declarations // Коментар: Предварителни декларации
class wxListEvent; // Needed for OnItemSelected // Предварителна декларация на клас wxListEvent (необходима за метода OnItemSelected)

// Define IDs used *within* MongoDBPanel here // Коментар: Дефиниране на ID-та, използвани *вътре* в MongoDBPanel
enum                                                          // Дефиниция на изброен тип за ID-та на контроли
{                                                             // Начало на изброяването
    ID_MONGODB_PANEL_INTERNAL_CLOSE_BTN = wxID_HIGHEST + 200, // ID за вътрешния бутон за затваряне на панела, базирано на най-високото стандартно ID + отместване
    ID_MONGODB_UPLOAD_JSON,                                   // ID за бутона за качване на JSON
    ID_MONGODB_REFRESH,                                       // ID за бутона за опресняване
    ID_MONGODB_DOWNLOAD_SELECTED,                             // ID за бутона за изтегляне на избраното
    ID_MONGODB_LOAD_INSTALL                                   // <-- Add an ID for Load & Install button // ID за бутона "Load & Install" <-- Коментар: Добавяне на ID за бутона Load & Install
}; // Край на изброяването

class MongoDBPanel : public wxPanel // Декларация на клас MongoDBPanel, наследяващ wxPanel
{                                   // Начало на дефиницията на класа
public:                             // Публична секция на класа
    // ***** MODIFIED CONSTRUCTOR ***** // Коментар: ***** ПРОМЕНЕН КОНСТРУКТОР *****
    MongoDBPanel(wxWindow *parent, const wxString &projectDir); // Added projectDir // Конструктор, приема родителски прозорец и път до директорията на проекта (добавен projectDir)
    virtual ~MongoDBPanel();                                    // Виртуален деструктор
    void LoadMongoDBContent();                                  // Public method to load/refresh data // Публичен метод за зареждане/опресняване на данни

private: // Частна секция на класа
    // --- Event Handlers --- // Коментар: Обработчици на събития
    void OnCloseClick(wxCommandEvent &event);       // Метод-обработчик за натискане на бутона за затваряне
    void OnUploadJson(wxCommandEvent &event);       // Метод-обработчик за качване на JSON
    void OnRefresh(wxCommandEvent &event);          // Метод-обработчик за опресняване
    void OnLoadAndInstall(wxCommandEvent &event);   // Метод-обработчик за зареждане и инсталиране
    void OnItemSelected(wxListEvent &event);        // Метод-обработчик за избор на елемент в списъка
    void OnDownloadSelected(wxCommandEvent &event); // Метод-обработчик за изтегляне на избраното

    // --- UI Controls --- // Коментар: UI контроли
    wxButton *m_closeButton;       // Указател към бутона за затваряне
    wxButton *m_uploadButton;      // Указател към бутона за качване
    wxButton *m_refreshButton;     // Указател към бутона за опресняване
    wxButton *m_loadInstallButton; // Указател към бутона "Load & Install"
    wxButton *m_downloadButton;    // Указател към бутона за изтегляне
    wxListCtrl *m_contentList;     // Указател към контролата за списък със съдържание

    // --- Document tracking --- // Коментар: Проследяване на документи
    std::map<long, wxString> m_documentIds; // Карта, асоциираща индекс на елемент в списъка (long) с ID на документ (wxString)
    int m_selectedItemIndex;                // Индекс на текущо избрания елемент в списъка
    wxString m_selectedDocumentId;          // ID на текущо избрания документ

    // --- Styling Helpers (optional) --- // Коментар: Помощни методи за стилизиране (опционално)
    void ApplyThemeColors();     // Метод за прилагане на цветовете на темата
    wxColour m_bgColor;          // Цвят на фона
    wxColour m_listBgColor;      // Цвят на фона на списъка
    wxColour m_listFgColor;      // Цвят на текста в списъка
    wxColour m_headerColor;      // Цвят на заглавния текст
    wxColour m_buttonBgColor;    // Цвят на фона на бутоните
    wxColour m_buttonFgColor;    // Цвят на текста на бутоните
    wxColour m_uploadBtnBgColor; // Цвят на фона на бутона за качване
    wxColour m_headerBgColor;    // Цвят на фона на заглавната част
    wxColour m_borderColor;      // Цвят на рамката

    // --- Logic Helpers --- // Коментар: Помощни методи за логика
    void ProcessInstallation(const wxString &configName, const wxString &appNames); // Метод за обработка на инсталацията
    wxString GetContainerIdFromProjectDir();                                        // <-- New helper function // Метод за получаване на ID на контейнера от директорията на проекта <-- Коментар: Нова помощна функция

    // --- Added JSON File Handling Helpers --- // Коментар: Добавени помощни методи за работа с JSON файлове
    bool ParseJsonFile(const wxString &filePath, wxString &configName, wxString &appsList); // Метод за парсване на JSON файл
    void CreateSampleDBFiles(const wxString &dbDirPath);                                    // Метод за създаване на примерни DB файлове
    void LoadFromLocalFiles();                                                              // <-- Add fallback loading function // Метод за зареждане от локални файлове (резервен вариант) <-- Коментар: Добавяне на функция за резервно зареждане
    void SaveToLocalFile(const wxString &sourcePath, const std::string &jsonContent);       // <-- Add local saving function // Метод за запазване в локален файл <-- Коментар: Добавяне на функция за локално запазване

    // ***** ADDED MEMBER ***** // Коментар: ***** ДОБАВЕНА ЧЛЕН-ПРОМЕНЛИВА *****
    wxString m_projectDir; // Съхранява пътя до директорията на проекта

    // --- Event Table --- // Коментар: Таблица на събитията
    wxDECLARE_EVENT_TABLE(); // Макрос за деклариране на таблицата на събитията
}; // Край на дефиницията на класа

#endif // MONGODBPANEL_H // Край на include guard