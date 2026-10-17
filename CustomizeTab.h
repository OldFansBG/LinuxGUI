#pragma once // Директива към компилатора да включи този хедър файл само веднъж, дори ако е указан многократно

#include <wx/wx.h>     // Включва основния хедър файл на wxWidgets
#include <wx/thread.h> // Включва хедър файла за работа с нишки в wxWidgets (wxThread)
#include <atomic>      // std::atomic за флага за спиране на нишката

class WallpaperLoadThread; // Предварителна декларация на класа WallpaperLoadThread

class CustomizeTab : public wxPanel // Декларация на клас CustomizeTab, който наследява публично от wxPanel
{                                   // Начало на тялото на класа
public:                             // Начало на публичната секция на класа
    CustomizeTab(wxWindow *parent, const wxString &projectDir = wxEmptyString); // Декларация на конструктора, който приема указател към родителски прозорец
    ~CustomizeTab();                // Декларация на деструктора
    void LoadWallpaper();           // Декларация на метод за зареждане на тапети

    // Helper method to check if wallpaperPanel is ready for use
    bool IsWallpaperPanelReady() const;

private:                  // Начало на частната секция на класа
    void CreateContent(); // Декларация на метод за създаване на съдържанието на таба

    void OnAddWallpaper(wxCommandEvent &event);          // Декларация на метод за обработка на събитие при добавяне на тапет
    void OnWallpaperReady(wxCommandEvent &event);        // Декларация на метод за обработка на събитие, когато един тапет е готов
    void OnWallpaperLoadComplete(wxCommandEvent &event); // Декларация на метод за обработка на събитие, когато всички тапети са заредени
    void OnOpenWallpaperFolder(wxCommandEvent &event);   // Декларация на метод за отваряне на папката с тапети

    // Нови методи за адаптивно разположение
    void OnWallpaperPanelSize(wxSizeEvent &event); // Декларация на метод за обработка на събитие при промяна на размера на панела с тапети
    void UpdateWallpaperGrid();                    // Декларация на метод за обновяване на мрежата с тапети
    void SelectWallpaper(wxPanel *container);      // Маркира избрания тапет (пътят е в името на контейнера)
    void OnApplyWallpaper(wxCommandEvent &event);  // Задава избрания тапет като фон по подразбиране в ISO образа
    void StopWallpaperThread();                    // Спира и изчаква нишката за зареждане на тапети

    wxScrolledWindow *wallpaperPanel; // Указател към скролируем прозорец, който ще съдържа галерията с тапети
    wxStaticText *m_loadingText;      // Указател към статичен текст, показван по време на зареждане
    std::atomic<bool> m_stopWallpaperLoad; // Флаг, указващ дали зареждането на тапети трябва да бъде спряно
    WallpaperLoadThread *m_loadThread;     // Текущата нишка за зареждане (joinable)
    int m_loadGeneration;                  // Увеличава се при всяко зареждане; старите събития се игнорират
    wxString m_projectDir;                 // Директорията на проекта (съдържа container_id.txt)
    wxString m_wallpaperDir;               // Папка с тапетите (%APPDATA%\LinuxISOPro\wallpapers)
    wxButton *m_applyWallpaperButton;      // Бутон "Apply Wallpaper to ISO"
    bool m_isDestroyed;               // Флаг, указващ дали табът е в процес на унищожаване

    DECLARE_EVENT_TABLE() // Макрос за деклариране на таблицата със събития за този клас

    // Make WallpaperLoadThread a friend class so it can access private members
    friend class WallpaperLoadThread;
}; // Край на тялото на класа CustomizeTab

// Актуализирайте структурата WallpaperImageData, за да включва състояние на избор
struct WallpaperImageData    // Дефиниция на структура за съхранение на данни за изображение на тапет
{                            // Начало на тялото на структурата
    wxImage image;           // Обект wxImage, съдържащ самото изображение
    wxString originalPath;   // Низ, съдържащ оригиналния път до файла на тапета
    bool isSelected = false; // Булев флаг, указващ дали тапетът е избран (инициализиран на false)
}; // Край на тялото на структурата

class WallpaperLoadThread : public wxThread                // Декларация на клас WallpaperLoadThread, който наследява публично от wxThread
{                                                          // Начало на тялото на класа
public:                                                    // Начало на публичната секция на класа
    WallpaperLoadThread(CustomizeTab *tab, std::atomic<bool> &stopFlag, const wxString &dir, int generation) // Конструкторулев флаг за спиране
        : wxThread(wxTHREAD_JOINABLE), m_tab(tab), m_stopFlag(stopFlag), m_dir(dir), m_generation(generation)
    {
    } // Инициализира базовия клас wxThread като отделена нишка (detached) и член-променливите m_tab и m_stopFlag

    virtual ExitCode Entry() override; // Декларация на виртуален метод Entry, който съдържа кода, изпълняван от нишката (пренаписва метода от wxThread)

private:                 // Начало на частната секция на класа
    CustomizeTab *m_tab; // Указател към обекта CustomizeTab, с който работи нишката
    std::atomic<bool> &m_stopFlag; // Референция към флага за спиране, споделен с CustomizeTab
    wxString m_dir;                // Папката с тапетите
    int m_generation;              // Поколение на зареждането (виж CustomizeTab::m_loadGeneration)
}; // Край на тялото на класа WallpaperLoadThread