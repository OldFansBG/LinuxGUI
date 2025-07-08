#ifndef CUSTOMTITLEBAR_H // Директива за предпроцесора: ако CUSTOMTITLEBAR_H не е дефиниран
#define CUSTOMTITLEBAR_H // Директива за предпроцесора: дефинирай CUSTOMTITLEBAR_H, за да се избегне многократно включване

#include <wx/wx.h>       // Включва основния заглавен файл на wxWidgets
#include <wx/graphics.h> // За wxGraphicsContext в MyButton // Включва заглавния файл за wxGraphicsContext, използван в класа MyButton
#include <wx/stdpaths.h> // За намиране на ресурси в MyButton // Включва заглавния файл за стандартни пътища, използван за намиране на ресурси в MyButton
#include <wx/filename.h> // За манипулация на пътища в MyButton // Включва заглавния файл за работа с имена на файлове, използван за манипулация на пътища в MyButton
#include <memory>        // За std::unique_ptr в MyButton // Включва заглавния файл за умни указатели, като std::unique_ptr, потенциално използван в MyButton (въпреки че не е видим в декларацията)
#include "ThemeConfig.h" // Необходимо за достъп до темата // Включва заглавния файл "ThemeConfig.h", нужен за достъп до конфигурацията на темата
#include "WindowIDs.h"   // Включва за идентификатори на бутони като ID_MINIMIZE, ID_CLOSE_WINDOW // Включва заглавния файл "WindowIDs.h" за дефиниции на идентификатори на прозорци/контроли

//------------------------------------------------------------------------ // Разделителна линия в коментар
// MyButton Class (Used by CustomTitleBar) // Клас MyButton (Използван от CustomTitleBar) // Коментар, описващ предназначението на клас MyButton
//------------------------------------------------------------------------ // Разделителна линия в коментар
class MyButton : public wxWindow                                         // Декларация на клас MyButton, който наследява от wxWindow
{                                                                        // Начало на тялото на клас MyButton
public:                                                                  // Декларира публичната секция на класа
    MyButton(wxWindow *parent, wxWindowID id, const wxString &imagePath, // Декларация на конструктора на MyButton, първи ред на параметрите
             const wxPoint &pos = wxDefaultPosition,                     // Параметър на конструктора: позиция, с подразбираща се стойност wxDefaultPosition
             const wxSize &size = wxDefaultSize,                         // Параметър на конструктора: размер, с подразбираща се стойност wxDefaultSize
             long style = 0,                                             // Параметър на конструктора: стил на прозореца, с подразбираща се стойност 0
             const wxString &name = wxPanelNameStr);                     // Параметър на конструктора: име на прозореца, с подразбираща се стойност wxPanelNameStr

    void SetAlwaysShowButton(bool always) { m_alwaysShow = always; } // Публичен метод за задаване дали бутонът винаги да се показва; реализация в заглавния файл

    // --- Event Handlers --- // --- Обработчици на събития --- // Коментар, указващ секция с обработчици на събития
    void OnPaint(wxPaintEvent &event);      // Декларация на метод за обработка на събитието за рисуване (OnPaint)
    void OnMouseDown(wxMouseEvent &event);  // Декларация на метод за обработка на събитието за натискане на бутон на мишката (OnMouseDown)
    void OnMouseUp(wxMouseEvent &event);    // Декларация на метод за обработка на събитието за отпускане на бутон на мишката (OnMouseUp)
    void OnMouseEnter(wxMouseEvent &event); // Декларация на метод за обработка на събитието за влизане на курсора на мишката в областта на контрола (OnMouseEnter)
    void OnMouseLeave(wxMouseEvent &event); // Декларация на метод за обработка на събитието за излизане на курсора на мишката от областта на контрола (OnMouseLeave)

private:                                       // Декларира частната секция на класа
    void DrawOnContext(wxGraphicsContext &gc); // Помощен метод за логиката на рисуване // Декларация на частен помощен метод за рисуване върху графичен контекст
                                               // Член-променливи // Коментар, указващ секция с член-променливи
    // Member Variables
    wxImage m_image;         // Член-променлива от тип wxImage за съхранение на текущото изображение на бутона
    wxImage m_originalImage; // Член-променлива от тип wxImage за съхранение на оригиналното изображение на бутона
    bool m_isHovered;        // Булева член-променлива, указваща дали курсорът на мишката е над бутона
    bool m_isPressed;        // Булева член-променлива, указваща дали бутонът е натиснат
    bool m_alwaysShow;       // Булева член-променлива, указваща дали бутонът трябва винаги да се показва

    wxDECLARE_EVENT_TABLE(); // Макрос на wxWidgets за деклариране на таблицата със събития за този клас
}; // Край на тялото на клас MyButton

//------------------------------------------------------------------------ // Разделителна линия в коментар
// CustomTitleBar Class // Клас CustomTitleBar // Коментар, описващ предназначението на клас CustomTitleBar
//------------------------------------------------------------------------ // Разделителна линия в коментар
class CustomTitleBar : public wxPanel // Декларация на клас CustomTitleBar, който наследява от wxPanel
{                                     // Начало на тялото на клас CustomTitleBar
public:                               // Декларира публичната секция на класа
    CustomTitleBar(wxWindow *parent); // Декларация на конструктора на CustomTitleBar, приемащ указател към родителски прозорец

    wxStaticText *GetTitleTextWidget() { return m_titleText; } // Публичен метод за получаване на указател към уиджета за заглавен текст; реализация в заглавния файл

    // `RefreshButtons` е променен на по-обхватния `RefreshControls` // Коментар, обясняващ промяна в името на метод (предишно име RefreshButtons)
    void RefreshControls(); // Декларация на публичен метод за обновяване на контролите в заглавната лента

private: // Декларира частната секция на класа
    // UI Creation // Създаване на потребителския интерфейс // Коментар, указващ секция за методи, свързани със създаването на потребителския интерфейс
    void CreateControls(); // Декларация на частен метод за създаване на контролите в заглавната лента

    // Event Handlers // Обработчици на събития // Коментар, указващ секция с обработчици на събития
    void OnPaint(wxPaintEvent &event);                       // Декларация на метод за обработка на събитието за рисуване (OnPaint)
    void OnMouseLeftDown(wxMouseEvent &event);               // Декларация на метод за обработка на събитието за натискане на левия бутон на мишката (OnMouseLeftDown)
    void OnMouseLeftUp(wxMouseEvent &event);                 // Декларация на метод за обработка на събитието за отпускане на левия бутон на мишката (OnMouseLeftUp)
    void OnMouseMove(wxMouseEvent &event);                   // Декларация на метод за обработка на събитието за движение на мишката (OnMouseMove)
    void OnMouseCaptureLost(wxMouseCaptureLostEvent &event); // Декларация на метод за обработка на събитието за загуба на "улавянето" на мишката (OnMouseCaptureLost)
    void OnMouseLeave(wxMouseEvent &event);                  // Декларация на метод за обработка на събитието за излизане на курсора на мишката от областта на контрола (OnMouseLeave)
    void OnMinimize(wxCommandEvent &event);                  // Декларация на метод за обработка на командата за минимизиране (OnMinimize)
    void OnClose(wxCommandEvent &event);                     // Декларация на метод за обработка на командата за затваряне (OnClose)

    // Член-променливи // Коментар, указващ секция с член-променливи
    // Member Variables
    MyButton *m_minimizeButton; // Указател към обект от тип MyButton, представляващ бутона за минимизиране
    MyButton *m_closeButton;    // Указател към обект от тип MyButton, представляващ бутона за затваряне
    wxStaticText *m_titleText;  // Указател към обект от тип wxStaticText, показващ заглавието на прозореца
    bool m_isDragging;          // Булева член-променлива, указваща дали потребителят в момента влачи заглавната лента
    wxPoint m_dragStart;        // Член-променлива от тип wxPoint, съхраняваща началната точка на влачене

    wxDECLARE_EVENT_TABLE(); // Макрос на wxWidgets за деклариране на таблицата със събития за този клас
}; // Край на тялото на клас CustomTitleBar

#endif // CUSTOMTITLEBAR_H // Директива за предпроцесора: край на условния блок, започнат с #ifndef CUSTOMTITLEBAR_H