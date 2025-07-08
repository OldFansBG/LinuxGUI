#include "CustomTitleBar.h" // Включва заглавния файл за класа CustomTitleBar
#include "WindowIDs.h"      // Включва за идентификатори // Включва заглавния файл за WindowIDs (идентификатори на прозорци/контроли)
#include <wx/stdpaths.h>    // Включва wxStandardPaths за получаване на стандартни пътища в системата
#include <wx/filename.h>    // Включва wxFileName за работа с имена на файлове и пътища
#include <wx/dcbuffer.h>    // Включва wxDCBuffer за буферирано рисуване, което предотвратява трептене
#include <wx/graphics.h>    // Включва wxGraphicsContext // Включва wxGraphicsContext за разширени възможности за 2D рисуване
#include <algorithm>        // Включва за std::min // Включва стандартната C++ библиотека <algorithm> за функции като std::min

// --- Таблица със събития на MyButton ---
BEGIN_EVENT_TABLE(MyButton, wxWindow)    // Начало на дефиницията на таблицата със събития за класа MyButton, наследяващ wxWindow
EVT_PAINT(MyButton::OnPaint)             // Свързва събитието за рисуване (EVT_PAINT) с метода MyButton::OnPaint
EVT_LEFT_DOWN(MyButton::OnMouseDown)     // Свързва събитието за натискане на левия бутон на мишката (EVT_LEFT_DOWN) с метода MyButton::OnMouseDown
EVT_LEFT_UP(MyButton::OnMouseUp)         // Свързва събитието за отпускане на левия бутон на мишката (EVT_LEFT_UP) с метода MyButton::OnMouseUp
EVT_ENTER_WINDOW(MyButton::OnMouseEnter) // Свързва събитието за влизане на мишката в областта на прозореца (EVT_ENTER_WINDOW) с метода MyButton::OnMouseEnter
EVT_LEAVE_WINDOW(MyButton::OnMouseLeave) // Свързва събитието за излизане на мишката от областта на прозореца (EVT_LEAVE_WINDOW) с метода MyButton::OnMouseLeave
END_EVENT_TABLE()                        // Край на дефиницията на таблицата със събития

// --- Имплементация на конструктора на MyButton ---
MyButton::MyButton(wxWindow *parent, wxWindowID id, const wxString &imagePath,       // Конструктор на класа MyButton
                   const wxPoint &pos, const wxSize &size,                           // Параметри: родителски прозорец, ID, път до изображение, позиция, размер
                   long style, const wxString &name)                                 // Параметри: стил, име на прозореца
    : wxWindow(),                                                                    // Инициализира базовия клас wxWindow (празна инициализация, Create ще бъде извикан по-късно)
      m_alwaysShow(false), m_isHovered(false), m_isPressed(false)                    // Инициализира състоянието // Инициализира член-променливите за състояние на бутона
{                                                                                    // Начало на тялото на конструктора
    SetBackgroundStyle(wxBG_STYLE_PAINT);                                            // Използва прозрачен фон // Задава стил на фона, за да се позволи персонализирано рисуване и прозрачност
    wxWindow::Create(parent, id, pos, size, style | wxFULL_REPAINT_ON_RESIZE, name); // Създава действителния прозорец с подадените параметри и флаг за пълно прерисуване при оразмеряване

    if (!imagePath.IsEmpty())                                          // Проверява дали пътят до изображението не е празен
    {                                                                  // Начало на if блока
        wxString exePath = wxStandardPaths::Get().GetExecutablePath(); // Взема пълния път до изпълнимия файл на приложението
        wxFileName fName(exePath);                                     // Създава обект wxFileName от пътя до изпълнимия файл
        fName.SetFullName("");                                         // Премахва името на изпълнимия файл, оставяйки само пътя до директорията
        fName.AppendDir("resources");                                  // Добавя директория "resources" към пътя
        fName.SetFullName(imagePath);                                  // Задава името на файла на изображението

        wxLogDebug("MyButton: Attempting to load image from: %s", fName.GetFullPath()); // Записва в дебъг лога пътя, от който се опитва да зареди изображението

        if (fName.FileExists())                                       // Проверява дали файлът с изображението съществува
        {                                                             // Начало на if блока
            m_image.LoadFile(fName.GetFullPath(), wxBITMAP_TYPE_PNG); // Зарежда изображението от файла, указвайки, че е PNG
            if (m_image.IsOk())                                       // Проверява дали изображението е заредено успешно
            {                                                         // Начало на if блока
                if (size == wxDefaultSize)                            // Ако размерът на бутона не е указан изрично (е wxDefaultSize)
                {                                                     // Начало на if блока
                    SetSize(m_image.GetWidth(), m_image.GetHeight()); // Задава размера на бутона според размера на изображението
                } // Край на if блока
                m_originalImage = m_image.Copy(); // Създава копие на оригиналното изображение
            } // Край на if блока
            else                                                                                                               // Ако изображението не е заредено успешно
            {                                                                                                                  // Начало на else блока
                wxLogWarning("MyButton: Failed to load image file '%s' (invalid format or read error).", fName.GetFullPath()); // Записва предупреждение в лога за неуспешно зареждане
            } // Край на else блока
        } // Край на if блока
        else                                                                              // Ако файлът с изображението не е намерен
        {                                                                                 // Начало на else блока
            wxLogWarning("MyButton: Image file not found at '%s'.", fName.GetFullPath()); // Записва предупреждение в лога, че файлът не е намерен
        } // Край на else блока
    } // Край на if блока
    else                                                     // Ако пътят до изображението е празен
    {                                                        // Начало на else блока
        wxLogWarning("MyButton: Empty imagePath provided."); // Записва предупреждение в лога за празен път
    } // Край на else блока
} // Край на тялото на конструктора

// --- Имплементация на MyButton ---
void MyButton::OnPaint(wxPaintEvent &event)                // Метод за обработка на събитието за рисуване
{                                                          // Начало на тялото на метода OnPaint
    wxAutoBufferedPaintDC dc(this);                        // Създава буфериран контекст за рисуване (DC) за текущия прозорец
    wxGraphicsContext *gc = wxGraphicsContext::Create(dc); // Създава графичен контекст (wxGraphicsContext) от DC-то за по-разширени възможности за рисуване

    if (gc && m_originalImage.IsOk())                                                                        // Проверява дали графичният контекст е създаден успешно и дали оригиналното изображение е валидно
    {                                                                                                        // Начало на if блока
        const ThemeColors &colors = ThemeConfig::Get().GetThemeColors(ThemeConfig::Get().GetCurrentTheme()); // Взема цветовете на текущата тема
        wxRect rect = GetClientRect();                                                                       // Взема клиентската област на бутона (правоъгълника, в който се рисува)
        wxColour buttonBg = GetParent()->GetBackgroundColour();                                              // По подразбиране цветът на фона на бутона е цветът на фона на родителя
        if (m_isPressed)                                                                                     // Ако бутонът е натиснат
            buttonBg = colors.button.pressed;                                                                // Задава цвета на фона за натиснат бутон от темата
        else if (m_isHovered)                                                                                // Ако мишката е върху бутона
            buttonBg = colors.button.hover;                                                                  // Задава цвета на фона за бутон под мишката от темата

        gc->SetBrush(wxBrush(buttonBg));                            // Задава четката (за запълване) на графичния контекст с изчисления цвят на фона
        gc->SetPen(*wxTRANSPARENT_PEN);                             // Задава прозрачно перо (за контури), така че да няма видим контур
        gc->DrawRectangle(rect.x, rect.y, rect.width, rect.height); // Рисува правоъгълник, запълващ цялата клиентска област на бутона с избрания фон

        wxBitmap bmp(m_originalImage);                     // Създава wxBitmap от оригиналното изображение (wxGraphicsContext работи по-добре с wxBitmap)
        gc->SetInterpolationQuality(wxINTERPOLATION_BEST); // Задава най-добро качество на интерполация при мащабиране на изображението

        wxSize bmpSize = bmp.GetSize();                                    // Взема размера на битмапа
        wxSize btnSize = GetClientSize();                                  // Взема размера на клиентската област на бутона
        double scaleX = (double)btnSize.GetWidth() / bmpSize.GetWidth();   // Изчислява коефициента на мащабиране по X
        double scaleY = (double)btnSize.GetHeight() / bmpSize.GetHeight(); // Изчислява коефициента на мащабиране по Y
        double scale = std::min({scaleX, scaleY, 1.0}) * 0.6;              // Избира най-малкия от коефициентите на мащабиране и 1.0 (за да не се увеличава изображението), и го умножава по 0.6, за да направи иконата по-малка

        double drawWidth = bmpSize.GetWidth() * scale;           // Изчислява ширината на изображението за рисуване
        double drawHeight = bmpSize.GetHeight() * scale;         // Изчислява височината на изображението за рисуване
        double drawX = (btnSize.GetWidth() - drawWidth) / 2.0;   // Изчислява X координатата за центрирано рисуване
        double drawY = (btnSize.GetHeight() - drawHeight) / 2.0; // Изчислява Y координатата за центрирано рисуване

        gc->DrawBitmap(bmp, drawX, drawY, drawWidth, drawHeight); // Рисува битмапа на изчислените координати и с изчислените размери

        delete gc; // Изтрива графичния контекст, за да освободи ресурсите
    } // Край на if блока
    else                                        // Ако графичният контекст не е създаден или изображението не е валидно
    {                                           // Начало на else блока
        wxPaintDC dcFallback(this);             // Създава стандартен DC като резервен вариант
        dcFallback.SetBackground(*wxRED_BRUSH); // Задава червена четка за фон (за индикация на грешка)
        dcFallback.Clear();                     // Изчиства областта с червения фон
    } // Край на else блока
} // Край на тялото на метода OnPaint

void MyButton::OnMouseDown(wxMouseEvent &event) // Метод за обработка на събитието за натискане на левия бутон на мишката
{                                               // Начало на тялото на метода OnMouseDown
    m_isPressed = true;                         // Задава флага за натиснато състояние на true
    CaptureMouse();                             // "Хваща" мишката, така че всички последващи събития от мишката да се изпращат към този прозорец, дори ако курсорът излезе извън него
    Refresh();                                  // Поисква прерисуване на бутона, за да се отрази натиснатото състояние
    event.Skip();                               // Позволява на събитието да бъде обработено и от други обработчици (ако има такива)
} // Край на тялото на метода OnMouseDown

void MyButton::OnMouseUp(wxMouseEvent &event) // Метод за обработка на събитието за отпускане на левия бутон на мишката
{                                             // Начало на тялото на метода OnMouseUp
    bool wasPressed = m_isPressed;            // Запазва предишното състояние на натискане
    m_isPressed = false;                      // Задава флага за натиснато състояние на false
    if (HasCapture())                         // Проверява дали прозорецът е "хванал" мишката
    {                                         // Начало на if блока
        ReleaseMouse();                       // "Освобождава" мишката
    } // Край на if блока
    Refresh(); // Поисква прерисуване на бутона, за да се отрази отпуснатото състояние

    wxPoint mousePos = event.GetPosition();               // Взема позицията на мишката спрямо бутона
    wxRect clientRect = GetClientRect();                  // Взема клиентската област на бутона
    if (wasPressed && clientRect.Contains(mousePos))      // Проверява дали бутонът е бил натиснат и дали мишката е все още върху него при отпускане (това се счита за клик)
    {                                                     // Начало на if блока
        wxCommandEvent clickEvent(wxEVT_BUTTON, GetId()); // Създава ново събитие от тип wxEVT_BUTTON (стандартно събитие за клик върху бутон)
        clickEvent.SetEventObject(this);                  // Задава текущия бутон като обект, генерирал събитието
        ProcessWindowEvent(clickEvent);                   // Обработва (изпраща) събитието за клик
    } // Край на if блока
    else              // Ако не е бил валиден клик (напр. мишката е била отпусната извън бутона)
    {                 // Начало на else блока
        event.Skip(); // Позволява на събитието да бъде обработено от други
    } // Край на else блока
} // Край на тялото на метода OnMouseUp

void MyButton::OnMouseEnter(wxMouseEvent &event) // Метод за обработка на събитието за влизане на мишката в областта на бутона
{                                                // Начало на тялото на метода OnMouseEnter
    m_isHovered = true;                          // Задава флага за състояние "мишката е върху" на true
    Refresh();                                   // Поисква прерисуване, за да се отрази промяната във визията (hover ефект)
    event.Skip();                                // Позволява на събитието да бъде обработено от други
} // Край на тялото на метода OnMouseEnter

void MyButton::OnMouseLeave(wxMouseEvent &event) // Метод за обработка на събитието за излизане на мишката от областта на бутона
{                                                // Начало на тялото на метода OnMouseLeave
    m_isHovered = false;                         // Задава флага за състояние "мишката е върху" на false
    m_isPressed = false;                         // Нулира и флага за натиснато състояние (за всеки случай, ако мишката е напуснала, докато бутонът е бил натиснат)
    Refresh();                                   // Поисква прерисуване, за да се премахне hover ефектът
    event.Skip();                                // Позволява на събитието да бъде обработено от други
} // Край на тялото на метода OnMouseLeave

void MyButton::DrawOnContext(wxGraphicsContext &gc) // Метод за рисуване върху предоставен графичен контекст (не се използва активно в текущата имплементация на OnPaint)
{                                                   // Начало на тялото на метода DrawOnContext
  // Имплементацията не е строго необходима въз основа на текущия OnPaint
} // Край на тялото на метода DrawOnContext

// --- Таблица със събития и имплементация на CustomTitleBar ---
BEGIN_EVENT_TABLE(CustomTitleBar, wxPanel)                 // Начало на дефиницията на таблицата със събития за класа CustomTitleBar, наследяващ wxPanel
EVT_PAINT(CustomTitleBar::OnPaint)                         // Свързва събитието за рисуване с метода CustomTitleBar::OnPaint
EVT_LEFT_DOWN(CustomTitleBar::OnMouseLeftDown)             // Свързва натискането на левия бутон на мишката с CustomTitleBar::OnMouseLeftDown
EVT_LEFT_UP(CustomTitleBar::OnMouseLeftUp)                 // Свързва отпускането на левия бутон на мишката с CustomTitleBar::OnMouseLeftUp
EVT_MOTION(CustomTitleBar::OnMouseMove)                    // Свързва движението на мишката с CustomTitleBar::OnMouseMove
EVT_MOUSE_CAPTURE_LOST(CustomTitleBar::OnMouseCaptureLost) // Свързва събитието за загуба на "хващането" на мишката с CustomTitleBar::OnMouseCaptureLost
EVT_LEAVE_WINDOW(CustomTitleBar::OnMouseLeave)             // Свързва излизането на мишката от прозореца с CustomTitleBar::OnMouseLeave
EVT_BUTTON(ID_MINIMIZE, CustomTitleBar::OnMinimize)        // Свързва събитие от бутон с ID_MINIMIZE (клик) с CustomTitleBar::OnMinimize
EVT_BUTTON(ID_CLOSE_WINDOW, CustomTitleBar::OnClose)       // Свързва събитие от бутон с ID_CLOSE_WINDOW (клик) с CustomTitleBar::OnClose
END_EVENT_TABLE()                                          // Край на дефиницията на таблицата със събития

CustomTitleBar::CustomTitleBar(wxWindow *parent)                    // Конструктор на класа CustomTitleBar
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, 30)), // Инициализира базовия клас wxPanel с родител, ID по подразбиране, позиция по подразбиране и размер (-1 ширина - автоматична, 30 височина)
      m_titleText(nullptr),                                         // Инициализира указателя към текста на заглавието на nullptr
      m_isDragging(false)                                           // Инициализира флага за влачене на false
{                                                                   // Начало на тялото на конструктора
    // Задава стила на фона преди създаването на каквито и да било контроли
    SetBackgroundStyle(wxBG_STYLE_PAINT); // Задава стил на фона, за да се позволи персонализирано рисуване

    // Взема цветовете на темата
    const ThemeColors &colors = ThemeConfig::Get().GetThemeColors(ThemeConfig::Get().GetCurrentTheme()); // Взема цветовете от конфигурацията на темата

    // Задава изрично цвета на фона
    SetBackgroundColour(colors.titleBar); // Задава цвета на фона на заглавната лента според темата

    // Създава контролите
    CreateControls(); // Извиква метод за създаване на вътрешните контроли (бутони, текст)

    // Прилага темата
    ThemeConfig::Get().ApplyTheme(this, ThemeConfig::Get().GetCurrentTheme()); // Прилага настройките на темата към текущата заглавна лента
} // Край на тялото на конструктора
// В CustomTitleBar.cpp, променете функцията CreateControls:

void CustomTitleBar::CreateControls()                 // Метод за създаване на контролите в заглавната лента
{                                                     // Начало на тялото на метода CreateControls
    wxBoxSizer *sizer = new wxBoxSizer(wxHORIZONTAL); // Създава хоризонтален sizer за подреждане на елементите
    SetSizer(sizer);                                  // Задава този sizer като основен за CustomTitleBar панела

    // Първо създава текста на заглавието с правилна обработка на прозрачността
    m_titleText = new wxStaticText(this, wxID_ANY, "LinuxISOPro",                   // Създава статичен текст за заглавието
                                   wxDefaultPosition, wxDefaultSize,                // Позиция и размер по подразбиране
                                   wxALIGN_CENTER_VERTICAL | wxTRANSPARENT_WINDOW); // Стил: вертикално центриран, прозрачен прозорец (за да не прерисува излишно фона си)

    // Забележка: Не използвайте SetBackgroundStyle тук, тъй като е твърде късно
    // Вместо това ще използваме цвета на фона на родителя
    const ThemeColors &colors = ThemeConfig::Get().GetThemeColors(ThemeConfig::Get().GetCurrentTheme()); // Взема цветовете на темата
    m_titleText->SetBackgroundColour(colors.titleBar);                                                   // Задава цвета на фона на текста на заглавието да съответства на заглавната лента
    m_titleText->SetForegroundColour(colors.text);                                                       // Задава цвета на самия текст според темата

    wxFont titleFont = m_titleText->GetFont(); // Взема текущия шрифт на текста
    titleFont.SetPointSize(10);                // Задава размер на шрифта 10 пункта
    titleFont.SetWeight(wxFONTWEIGHT_NORMAL);  // Задава нормална дебелина на шрифта
    m_titleText->SetFont(titleFont);           // Прилага новия шрифт

    m_titleText->Bind(wxEVT_LEFT_DOWN, &CustomTitleBar::OnMouseLeftDown, this); // Свързва събитието за натискане на левия бутон върху текста с метода OnMouseLeftDown на CustomTitleBar
    m_titleText->Bind(wxEVT_LEFT_UP, &CustomTitleBar::OnMouseLeftUp, this);     // Свързва събитието за отпускане на левия бутон върху текста с OnMouseLeftUp
    m_titleText->Bind(wxEVT_MOTION, &CustomTitleBar::OnMouseMove, this);        // Свързва събитието за движение на мишката върху текста с OnMouseMove
    this->Bind(wxEVT_LEFT_DOWN, &CustomTitleBar::OnMouseLeftDown, this);        // Свързва събитието за натискане на левия бутон върху самата заглавна лента (панела) с OnMouseLeftDown
    this->Bind(wxEVT_LEFT_UP, &CustomTitleBar::OnMouseLeftUp, this);            // Свързва отпускането на левия бутон върху панела с OnMouseLeftUp
    this->Bind(wxEVT_MOTION, &CustomTitleBar::OnMouseMove, this);               // Свързва движението на мишката върху панела с OnMouseMove

    m_minimizeButton = new MyButton(this, ID_MINIMIZE, "minimize.png",  // Създава бутон за минимизиране (MyButton)
                                    wxDefaultPosition, wxSize(45, 30)); // С изображение "minimize.png" и размер 45x30
    m_closeButton = new MyButton(this, ID_CLOSE_WINDOW, "close.png",    // Създава бутон за затваряне (MyButton)
                                 wxDefaultPosition, wxSize(45, 30));    // С изображение "close.png" и размер 45x30

    sizer->AddSpacer(5);                                             // Добавя малко празно пространство отляво
    sizer->Add(m_titleText, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 5); // Добавя текста на заглавието към sizer-а, разширяващ се, вертикално центриран, с отстъп отляво
    sizer->Add(m_minimizeButton, 0, wxALIGN_CENTER_VERTICAL, 0);     // Добавя бутона за минимизиране, вертикално центриран, без допълнителен отстъп
    sizer->Add(m_closeButton, 0, wxALIGN_CENTER_VERTICAL, 0);        // Добавя бутона за затваряне, вертикално центриран, без допълнителен отстъп
} // Край на тялото на метода CreateControls

void CustomTitleBar::OnPaint(wxPaintEvent &event)                                                        // Метод за обработка на събитието за рисуване на заглавната лента
{                                                                                                        // Начало на тялото на метода OnPaint
    wxAutoBufferedPaintDC dc(this);                                                                      // Създава буфериран DC
    wxRect rect = GetClientRect();                                                                       // Взема клиентската област
    const ThemeColors &colors = ThemeConfig::Get().GetThemeColors(ThemeConfig::Get().GetCurrentTheme()); // Взема цветовете на темата

    // Задава цвета на фона на заглавната лента
    dc.SetBrush(wxBrush(colors.titleBar)); // Задава четка с цвета на фона на заглавната лента от темата
    dc.SetPen(*wxTRANSPARENT_PEN);         // Задава прозрачно перо
    dc.DrawRectangle(rect);                // Рисува правоъгълник, запълващ цялата област

    // Уверява се, че цветът на фона е правилно зададен
    SetBackgroundColour(colors.titleBar); // Повторно задава цвета на фона на контролата (за всеки случай, за консистентност)
} // Край на тялото на метода OnPaint

void CustomTitleBar::OnMouseLeftDown(wxMouseEvent &event)                       // Метод за обработка на натискане на левия бутон на мишката върху заглавната лента
{                                                                               // Начало на тялото на метода OnMouseLeftDown
    wxWindow *clickedWindow = dynamic_cast<wxWindow *>(event.GetEventObject()); // Взема прозореца, върху който е станало събитието
    // Проверява дали кликът е станал върху бутон или върху самия текст на заглавието.
    // Ако е върху текста на заглавието, *искаме* да влачим.
    // Ако е върху бутон, *не искаме* да влачим.
    if (dynamic_cast<MyButton *>(clickedWindow)) // Проверява дали кликнатият прозорец е от тип MyButton
    {                                            // Начало на if блока
        event.Skip();                            // Позволява на бутона да обработи собствения си клик
        return;                                  // Прекратява изпълнението на този метод
    } // Край на if блока

    // Ако кликът е върху текста на заглавието или фона на панела, започва влачене
    m_dragStart = ClientToScreen(event.GetPosition()); // Преобразува позицията на мишката от координати спрямо клиента към екранни координати и я запазва като начална точка на влачене
    wxPoint framePos = GetParent()->GetPosition();     // Взема текущата позиция на родителския прозорец (рамката)
    m_dragStart = m_dragStart - framePos;              // Изчислява отместването на мишката спрямо горния ляв ъгъл на рамката
    m_isDragging = true;                               // Задава флага за влачене на true
    CaptureMouse();                                    // "Хваща" мишката
    // Не пропускайте тук, обработили сме началото на влаченето
} // Край на тялото на метода OnMouseLeftDown

void CustomTitleBar::OnMouseLeftUp(wxMouseEvent &event) // Метод за обработка на отпускане на левия бутон на мишката
{                                                       // Начало на тялото на метода OnMouseLeftUp
    if (m_isDragging)                                   // Проверява дали е било в режим на влачене
    {                                                   // Начало на if блока
        if (HasCapture())                               // Проверява дали мишката е била "хваната"
        {                                               // Начало на if блока
            ReleaseMouse();                             // "Освобождава" мишката
        } // Край на if блока
        m_isDragging = false; // Задава флага за влачене на false
        // Не пропускайте тук, ако сме обработили освобождаването на влаченето
    } // Край на if блока
    else              // Ако не е било в режим на влачене
    {                 // Начало на else блока
        event.Skip(); // Позволява на събития от бутони и т.н. да се обработят, ако не се влачи
    } // Край на else блока
} // Край на тялото на метода OnMouseLeftUp

void CustomTitleBar::OnMouseMove(wxMouseEvent &event)             // Метод за обработка на движение на мишката
{                                                                 // Начало на тялото на метода OnMouseMove
    if (m_isDragging && event.Dragging() && event.LeftIsDown())   // Проверява дали е в режим на влачене, дали събитието е за влачене и дали левият бутон е натиснат
    {                                                             // Начало на if блока
        wxPoint currentPos = ClientToScreen(event.GetPosition()); // Взема текущата позиция на мишката в екранни координати
        wxPoint newFramePos = currentPos - m_dragStart;           // Изчислява новата позиция на рамката, като извади първоначалното отместване
        GetParent()->Move(newFramePos);                           // Премества родителския прозорец (рамката) на новата позиция
        // Не пропускайте събитието след преместване на прозореца
        // event.Skip(); // <<< ПРЕМАХНЕТЕ или КОМЕНТИРАЙТЕ този ред
    } // Край на if блока
    else              // Ако не се влачи
    {                 // Начало на else блока
        event.Skip(); // Позволява на hover събития и т.н. да се обработят, ако не се влачи
    } // Край на else блока
} // Край на тялото на метода OnMouseMove

void CustomTitleBar::OnMouseCaptureLost(wxMouseCaptureLostEvent &event) // Метод за обработка на събитието за загуба на "хващането" на мишката
{                                                                       // Начало на тялото на метода OnMouseCaptureLost
    m_isDragging = false;                                               // Нулира флага за влачене, тъй като сме загубили контрол над мишката
    // Няма нужда да се пропуска тук
} // Край на тялото на метода OnMouseCaptureLost

void CustomTitleBar::OnMouseLeave(wxMouseEvent &event) // Метод за обработка на излизане на мишката от областта на заглавната лента
{                                                      // Начало на тялото на метода OnMouseLeave
    // Нулира състоянието на hover на бутоните, ако мишката напусне изцяло заглавната лента
    if (m_minimizeButton)            // Проверява дали бутонът за минимизиране съществува
        m_minimizeButton->Refresh(); // Поисква прерисуване на бутона за минимизиране (за да се премахне евентуален hover ефект)
    if (m_closeButton)               // Проверява дали бутонът за затваряне съществува
        m_closeButton->Refresh();    // Поисква прерисуване на бутона за затваряне
    event.Skip();                    // Позволява на събитието да бъде обработено от други
} // Край на тялото на метода OnMouseLeave

void CustomTitleBar::OnMinimize(wxCommandEvent &event)    // Метод за обработка на командата за минимизиране
{                                                         // Начало на тялото на метода OnMinimize
    wxFrame *frame = wxDynamicCast(GetParent(), wxFrame); // Опитва се да преобразува родителския прозорец към wxFrame
    if (frame)                                            // Ако преобразуването е успешно
    {                                                     // Начало на if блока
        frame->Iconize(true);                             // Минимизира рамката (прозореца)
    } // Край на if блока
} // Край на тялото на метода OnMinimize

void CustomTitleBar::OnClose(wxCommandEvent &event)       // Метод за обработка на командата за затваряне
{                                                         // Начало на тялото на метода OnClose
    wxFrame *frame = wxDynamicCast(GetParent(), wxFrame); // Опитва се да преобразува родителския прозорец към wxFrame
    if (frame)                                            // Ако преобразуването е успешно
    {                                                     // Начало на if блока
        frame->Close(true);                               // Затваря рамката (прозореца), като true указва, че може да бъде отменено (напр. от EVT_CLOSE обработчик)
    } // Край на if блока
} // Край на тялото на метода OnClose

// Заменете функцията RefreshButtons в CustomTitleBar.cpp с тази:

void CustomTitleBar::RefreshControls() // Метод за опресняване на контролите в заглавната лента (обикновено при смяна на тема)
{                                      // Начало на тялото на метода RefreshControls
    // Взема цветовете на текущата тема
    const ThemeColors &colors = ThemeConfig::Get().GetThemeColors(ThemeConfig::Get().GetCurrentTheme()); // Взема цветовете от конфигурацията на темата

    // Актуализира цвета на фона на заглавната лента
    SetBackgroundColour(colors.titleBar); // Задава новия цвят на фона

    // Актуализира цветовете на текста на заглавието, за да съответстват на темата
    if (m_titleText)                                       // Проверява дали текстът на заглавието съществува
    {                                                      // Начало на if блока
        m_titleText->SetBackgroundColour(colors.titleBar); // Задава цвета на фона на текста
        m_titleText->SetForegroundColour(colors.text);     // Задава цвета на самия текст
        m_titleText->Refresh();                            // Поисква прерисуване на текста
    } // Край на if блока

    // Опреснява бутоните
    if (m_minimizeButton)            // Проверява дали бутонът за минимизиране съществува
        m_minimizeButton->Refresh(); // Поисква прерисуване на бутона
    if (m_closeButton)               // Проверява дали бутонът за затваряне съществува
        m_closeButton->Refresh();    // Поисква прерисуване на бутона

    // Опреснява цялата заглавна лента, за да се гарантира правилно рисуване
    Refresh(); // Поисква прерисуване на самата заглавна лента
} // Край на тялото на метода RefreshControls