// LinuxTerminalPanel.h // Име на заглавния файл
#ifndef LINUXTERMINALPANEL_H // Предварителна директива за условна компилация (include guard) - ако LINUXTERMINALPANEL_H не е дефиниран
#define LINUXTERMINALPANEL_H // Дефинира LINUXTERMINALPANEL_H, за да предотврати многократно включване

#include <wx/wx.h> // Включва основния заглавен файл на wxWidgets

#ifdef __WXGTK__        // Начало на блок за условна компилация (само ако се компилира за GTK+ среда, типично за Linux)
#include <wx/process.h> // Включва заглавния файл на wxWidgets за работа с външни процеси
#include <X11/Xlib.h>   // Включва заглавния файл на Xlib (библиотека за работа с X Window System)
#include <thread>       // Включва стандартната C++ библиотека за работа с нишки (threads)
#include <chrono>       // Включва стандартната C++ библиотека за работа с време (за закъснения)
#endif                  // Край на #ifdef __WXGTK__

class LinuxTerminalPanel : public wxPanel
{                                         // Декларация на клас LinuxTerminalPanel, наследяващ wxPanel
public:                                   // Публична секция на класа
    LinuxTerminalPanel(wxWindow *parent); // Конструктор, приемащ родителски прозорец
    virtual ~LinuxTerminalPanel();        // Виртуален деструктор

private: // Частна секция на класа
    // Terminal constants // Коментар: Константи за терминала
    static const int TERM_CHAR_WIDTH;      // Average character width in pixels // Статична константа: Средна ширина на символ в пиксели
    static const int TERM_CHAR_HEIGHT;     // Average character height in pixels // Статична константа: Средна височина на символ в пиксели
    static const int TERM_SCROLLBAR_WIDTH; // Typical scrollbar width // Статична константа: Типична ширина на плъзгача
    static const int TERM_PADDING;         // Padding on each side // Статична константа: Отстояние от всяка страна
    static const int TERM_MIN_COLS;        // Minimum number of columns // Статична константа: Минимален брой колони
    static const int TERM_MIN_ROWS;        // Minimum number of rows // Статична константа: Минимален брой редове

#ifdef __WXGTK__                             // Начало на блок за условна компилация (само за GTK+)
    void InitializeTerminal();               // Метод за инициализация на терминала
    void CleanupTerminal();                  // Метод за почистване/затваряне на терминала
    void FocusOnTerminal();                  // Метод за фокусиране върху терминала
    void OnSize(wxSizeEvent &event);         // Метод-обработчик за събитието "промяна на размера" на панела
    void OnTerminalSize(wxSizeEvent &event); // Метод-обработчик за събитието "промяна на размера" на контейнера на терминала
    void OnMouseEvent(wxMouseEvent &event);  // Метод-обработчик за събития от мишката
    void OnKeyEvent(wxKeyEvent &event);      // Метод-обработчик за събития от клавиатурата
    void ResizeTerminal(const wxSize &size); // Метод за преоразмеряване на вградения терминал

    Window panelId;               // Идентификатор на прозореца на X11 за панела
    wxProcess *terminalProcess;   // Указател към обект wxProcess, представляващ външния терминален процес
    wxPanel *m_terminalContainer; // Указател към панел-контейнер, в който ще бъде вграден терминалът
    Display *m_display;           // Указател към X11 Display структура
    bool m_initialized;           // Флаг, показващ дали терминалът е инициализиран
#else                             // Начало на else блок (за системи, които не са GTK+, напр. Windows, macOS)
    wxTextCtrl *m_textCtrl; // Fallback for non-Linux systems // Указател към wxTextCtrl, използван като резервен вариант за не-Linux системи (обикновено многоредово текстово поле)
#endif                            // Край на #ifdef __WXGTK__

    wxDECLARE_EVENT_TABLE(); // Макрос за деклариране на таблицата на събитията
}; // Край на дефиницията на клас LinuxTerminalPanel

#endif // LINUXTERMINALPANEL_H // Край на include guard