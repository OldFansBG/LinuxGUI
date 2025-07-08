#ifndef CUSTOMSTATUSBAR_H // Предпазител от многократно включване на хедър файла
#define CUSTOMSTATUSBAR_H // Дефиниране на предпазителя за хедър файла

#include <wx/wx.h> // Включване на основната wxWidgets библиотека

class CustomStatusBar : public wxPanel
{                                             // Дефиниция на класа CustomStatusBar, наследяващ wxPanel
public:                                       // Публичен интерфейс на класа
    CustomStatusBar(wxWindow *parent);        // Конструктор, който приема родителски прозорец
    void SetStatusText(const wxString &text); // Метод за задаване на текст в статус лентата

private:                               // Частни членове на класа
    void OnPaint(wxPaintEvent &event); // Частен метод за обработка на събитието за рисуване
    void CreateControls();             // Частен метод за създаване на контролите

    wxStaticText *m_statusText; // Указател към статичен текстов контрол за показване на статуса

    DECLARE_EVENT_TABLE() // Макро за деклариране на таблица със събития
};

#endif // CUSTOMSTATUSBAR_H // Край на предпазителя за хедър файла