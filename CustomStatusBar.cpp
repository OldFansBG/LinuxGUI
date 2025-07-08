#include "CustomStatusBar.h" // Включване на хедър файла за класа CustomStatusBar

BEGIN_EVENT_TABLE(CustomStatusBar, wxPanel) // Начало на таблицата със събития за класа CustomStatusBar
EVT_PAINT(CustomStatusBar::OnPaint)         // Регистриране на събитието за рисуване и функцията, която да го обработи
END_EVENT_TABLE()                           // Край на таблицата със събития

CustomStatusBar::CustomStatusBar(wxWindow *parent)                 // Конструктор на класа CustomStatusBar
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, 24)) // Инициализация на базовия клас wxPanel с родител, позиция по подразбиране и фиксирана височина от 24 пиксела
{
    CreateControls(); // Извикване на метода за създаване на контролите
}

void CustomStatusBar::CreateControls()
{                                                     // Метод за създаване на контролите в статус лентата
    wxBoxSizer *sizer = new wxBoxSizer(wxHORIZONTAL); // Създаване на хоризонтален sizer за подредба на елементите

    m_statusText = new wxStaticText(this, wxID_ANY, "Ready"); // Създаване на статичен текст с начална стойност "Ready"
    wxFont statusFont = m_statusText->GetFont();              // Вземане на текущия шрифт на текста
    statusFont.SetPointSize(9);                               // Задаване на размер на шрифта 9 пункта
    m_statusText->SetFont(statusFont);                        // Прилагане на новия шрифт към текста

    SetBackgroundColour(*wxLIGHT_GREY);          // Задаване на светло сив фон за статус лентата
    m_statusText->SetForegroundColour(*wxBLACK); // Задаване на черен цвят за текста

    sizer->Add(m_statusText, 0, wxALIGN_CENTER_VERTICAL | wxALL, 4); // Добавяне на текста към sizer-а с центриране по вертикала и отстъп от 4 пиксела от всички страни

    SetSizer(sizer); // Задаване на sizer-а за панела
}

void CustomStatusBar::SetStatusText(const wxString &text)
{ // Метод за задаване на текст в статус лентата
    if (m_statusText)
    {                                 // Проверка дали текстовият контрол съществува
        m_statusText->SetLabel(text); // Задаване на новия текст
        Layout();                     // Преоразмеряване на контролите, за да се покаже новият текст правилно
    }
}

void CustomStatusBar::OnPaint(wxPaintEvent &event)
{                                               // Обработчик за събитието за рисуване
    wxPaintDC dc(this);                         // Създаване на контекст за рисуване за този панел
    dc.SetPen(wxPen(*wxBLACK));                 // Задаване на черна писалка
    dc.DrawLine(0, 0, GetSize().GetWidth(), 0); // Рисуване на хоризонтална линия в най-горната част на панела по цялата ширина
}