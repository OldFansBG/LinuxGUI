#include "FilesystemSelectionDialog.h"                                                                                          // Включва заглавния файл за FilesystemSelectionDialog.
#include <wx/sizer.h>                                                                                                           // Включва заглавния файл за wxSizer, за управление на оформлението.
#include <wx/button.h>                                                                                                          // Включва заглавния файл за wxButton (бутони).
#include <wx/stattext.h>                                                                                                        // Включва заглавния файл за wxStaticText (статичен текст).
#include <wx/dcbuffer.h>                                                                                                        // Включва заглавния файл за wxDCBuffer, за двойно буферирано рисуване.
#include <wx/graphics.h>                                                                                                        // Включва заглавния файл за wxGraphicsContext, за разширени графични операции.
#include <wx/checkbox.h>                                                                                                        // Включва заглавния файл за wxCheckBox (квадратчета за отметка).
                                                                                                                                // Празна линия.
wxBEGIN_EVENT_TABLE(FilesystemSelectionDialog, wxDialog)                                                                        // Начало на дефиницията на таблицата със събития за класа FilesystemSelectionDialog, който наследява wxDialog.
    EVT_BUTTON(wxID_OK, FilesystemSelectionDialog::OnOK)                                                                        // Свързва събитието за натискане на бутон с ID wxID_OK с метода OnOK.
    EVT_BUTTON(wxID_CANCEL, FilesystemSelectionDialog::OnCancel)                                                                // Свързва събитието за натискане на бутон с ID wxID_CANCEL с метода OnCancel.
    EVT_PAINT(FilesystemSelectionDialog::OnPaint)                                                                               // Свързва събитието за прерисуване (EVT_PAINT) с метода OnPaint.
    EVT_CHECKBOX(wxID_ANY, FilesystemSelectionDialog::OnItemCheckbox)                                                           // Event for checkboxes // Свързва събитието за промяна на състоянието на квадратче за отметка (с произволно ID) с метода OnItemCheckbox.
    wxEND_EVENT_TABLE()                                                                                                         // Край на дефиницията на таблицата със събития.
                                                                                                                                // Празна линия.
    FilesystemSelectionDialog::FilesystemSelectionDialog(wxWindow *parent, const wxString &title, const wxArrayString &choices) // Конструктор на класа FilesystemSelectionDialog.
    : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxSize(450, 500), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)          // Извиква конструктора на базовия клас wxDialog с родител, ID, заглавие, позиция, размер и стилове (включително възможност за преоразмеряване).
{                                                                                                                               // Начало на тялото на конструктора.
    SetBackgroundStyle(wxBG_STYLE_PAINT);                                                                                       // Задава стил на фона, който изисква обработка на събитието OnPaint за рисуване на фона.
                                                                                                                                // Празна линия.
    wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);                                                                         // Създава главен wxBoxSizer с вертикална ориентация.
                                                                                                                                // Празна линия.
    // Title Text with improved styling // Заглавен текст с подобрено стилизиране.
    wxStaticText *titleText = new wxStaticText(this, wxID_ANY, title); // Създава статичен текст за заглавието на диалога.
    wxFont titleFont = titleText->GetFont();                           // Взема текущия шрифт на заглавния текст.
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);                            // Задава удебелен шрифт.
    titleFont.SetPointSize(titleFont.GetPointSize() + 3);              // Увеличава размера на шрифта с 3 пункта.
    titleText->SetFont(titleFont);                                     // Прилага новия шрифт към заглавния текст.
    mainSizer->Add(titleText, 0, wxEXPAND | wxALL, 25);                // Добавя заглавния текст към главния sizer, с разширяване и отстъп от всички страни 25 пиксела.
                                                                       // Празна линия.
    // Add description text // Добавяне на описателен текст.
    wxStaticText *descText = new wxStaticText(this, wxID_ANY, "Please select a filesystem to mount:"); // Създава статичен текст с описание.
    wxFont descFont = descText->GetFont();                                                             // Взема текущия шрифт на описателния текст.
    descFont.SetPointSize(descFont.GetPointSize() + 1);                                                // Увеличава размера на шрифта с 1 пункт.
    descText->SetFont(descFont);                                                                       // Прилага новия шрифт към описателния текст.
    mainSizer->Add(descText, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 25);                           // Добавя описателния текст към главния sizer, с разширяване и отстъпи отляво, отдясно и отдолу по 25 пиксела.
                                                                                                       // Празна линия.
    // Panel to hold the list of items with rounded corners // Панел за съхранение на списъка с елементи със заоблени ъгли.
    m_itemsPanel = new wxScrolledWindow(this, wxID_ANY);                         // Създава превъртащ се прозорец (панел) за елементите.
    m_itemsPanel->SetBackgroundColour(wxColour(55, 65, 81));                     // Darker background for contrast // Задава по-тъмен цвят на фона за контраст.
    m_itemsPanel->SetScrollRate(0, 10);                                          // Set vertical scroll rate // Задава скоростта на вертикално превъртане (10 пиксела на стъпка).
    m_itemsPanel->SetMinSize(wxSize(-1, 300));                                   // Set a reasonable default size to ensure scrollbars appear when needed // Задава минимален размер, за да се гарантира появата на плъзгачи при нужда.
    wxBoxSizer *itemsSizer = new wxBoxSizer(wxVERTICAL);                         // Създава sizer с вертикална ориентация за елементите в превъртащия се панел.
    m_itemsPanel->SetSizer(itemsSizer);                                          // Задава sizer-а на превъртащия се панел.
    mainSizer->Add(m_itemsPanel, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 25); // Добавя панела с елементи към главния sizer, позволявайки му да се разширява и с отстъпи.
                                                                                 // Празна линия.
    CreateItemList(choices);                                                     // Извиква метод за създаване на списъка с елементи (файлови системи).
                                                                                 // Празна линия.
    // OK and Cancel buttons with improved styling // Бутони "OK" и "Cancel" с подобрено стилизиране.
    wxBoxSizer *buttonSizer = new wxBoxSizer(wxHORIZONTAL);                                                                // Създава sizer с хоризонтална ориентация за бутоните.
    wxButton *okButton = new wxButton(this, wxID_OK, "Select", wxDefaultPosition, wxSize(100, 40), wxBORDER_NONE);         // Създава бутон "Select" (OK) без рамка.
    wxButton *cancelButton = new wxButton(this, wxID_CANCEL, "Cancel", wxDefaultPosition, wxSize(100, 40), wxBORDER_NONE); // Създава бутон "Cancel" без рамка.
                                                                                                                           // Празна линия.
    okButton->SetBackgroundColour(wxColour(79, 70, 229));                                                                  // Indigo color // Задава индигов цвят на фона на бутона "OK".
    okButton->SetForegroundColour(*wxWHITE);                                                                               // Задава бял цвят на текста на бутона "OK".
    cancelButton->SetBackgroundColour(wxColour(75, 85, 99));                                                               // Slate gray // Задава тъмносив цвят на фона на бутона "Cancel".
    cancelButton->SetForegroundColour(*wxWHITE);                                                                           // Задава бял цвят на текста на бутона "Cancel".
                                                                                                                           // Празна линия.
    wxFont buttonFont = okButton->GetFont();                                                                               // Взема шрифта на бутона "OK".
    buttonFont.SetWeight(wxFONTWEIGHT_BOLD);                                                                               // Задава удебелен шрифт.
    buttonFont.SetPointSize(buttonFont.GetPointSize() + 1);                                                                // Увеличава размера на шрифта с 1 пункт.
    okButton->SetFont(buttonFont);                                                                                         // Прилага новия шрифт към бутона "OK".
    cancelButton->SetFont(buttonFont);                                                                                     // Прилага новия шрифт към бутона "Cancel".
                                                                                                                           // Празна линия.
    // Add spacer to push buttons to the right // Добавя празно пространство, за да избута бутоните надясно.
    buttonSizer->Add(0, 0, 1, wxEXPAND);                                        // Добавя разтегливо празно пространство.
    buttonSizer->Add(cancelButton, 0, wxRIGHT, 15);                             // Добавя бутона "Cancel" с десен отстъп от 15 пиксела.
    buttonSizer->Add(okButton, 0);                                              // Добавя бутона "OK".
    mainSizer->Add(buttonSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 25); // Добавя sizer-а с бутоните към главния sizer, с разширяване и отстъпи.
                                                                                // Празна линия.
    SetSizerAndFit(mainSizer);                                                  // Задава главния sizer на диалога и го оразмерява според съдържанието му.
    ApplyTheme();                                                               // Прилага текущата тема.
    SetupColors();                                                              // Add this line // Извиква метод за настройка на цветовете.
    CentreOnParent();                                                           // Центрира диалога спрямо родителския му прозорец.
} // Край на тялото на конструктора.
  // Празна линия.
wxString FilesystemSelectionDialog::GetSelectedFilesystem() const // Метод за получаване на избраната файлова система.
{                                                                 // Начало на тялото на метода.
    return m_selectedFilesystem;                                  // Връща името на избраната файлова система.
} // Край на тялото на метода.
  // Празна линия.
void FilesystemSelectionDialog::OnOK(wxCommandEvent &event)                                             // Обработчик на събитието за натискане на бутона "OK".
{                                                                                                       // Начало на тялото на метода.
    if (!m_selectedFilesystem.IsEmpty())                                                                // Проверява дали е избрана файлова система.
        EndModal(wxID_OK);                                                                              // Ако е избрана, затваря модалния диалог и връща wxID_OK.
    else                                                                                                // Ако не е избрана файлова система.
        wxMessageBox("Please select a filesystem.", "Selection Required", wxOK | wxICON_WARNING, this); // Показва съобщение за грешка, изискващо избор.
} // Край на тялото на метода.
  // Празна линия.
void FilesystemSelectionDialog::OnCancel(wxCommandEvent &event) // Обработчик на събитието за натискане на бутона "Cancel".
{                                                               // Начало на тялото на метода.
    EndModal(wxID_CANCEL);                                      // Затваря модалния диалог и връща wxID_CANCEL.
} // Край на тялото на метода.
  // Празна линия.
void FilesystemSelectionDialog::ApplyTheme()                                               // Метод за прилагане на визуалната тема.
{                                                                                          // Начало на тялото на метода.
    ThemeConfig &themeConfig = ThemeConfig::Get();                                         // Взема инстанция на конфигурацията на темата (singleton).
    const ThemeColors &colors = themeConfig.GetThemeColors(themeConfig.GetCurrentTheme()); // Взема цветовете за текущата тема.
                                                                                           // Празна линия.
    SetBackgroundColour(colors.panel.background);                                          // Задава цвета на фона на диалога според темата.
                                                                                           // Празна линия.
    // Update our color system // Актуализира нашата цветова система.
    SetupColors(); // Извиква метод за настройка на специфични цветове за елементите.
                   // Празна линия.
    // Apply theme to all panels // Прилагане на темата към всички панели.
    wxWindowList itemChildren = m_itemsPanel->GetChildren(); // Взема списък с дъщерните прозорци (панелите с елементи) на m_itemsPanel.
    for (wxWindow *child : itemChildren)                     // Итерира през всеки дъщерен прозорец.
    {                                                        // Начало на тялото на цикъла.
        if (wxPanel *panel = wxDynamicCast(child, wxPanel))  // Ако дъщерният прозорец е wxPanel.
        {                                                    // Начало на тялото на if.
            panel->SetBackgroundColour(m_normalItemColor);   // Задава нормалния цвят на фона на панела на елемента.
                                                             // Празна линия.
            // Apply to all children of the panel // Прилагане към всички дъщерни елементи на панела.
            for (wxWindow *panelChild : panel->GetChildren())               // Итерира през дъщерните елементи на текущия панел (напр. checkbox, statictext).
            {                                                               // Начало на вътрешния цикъл.
                if (wxCheckBox *cb = wxDynamicCast(panelChild, wxCheckBox)) // Ако дъщерният елемент е wxCheckBox.
                {                                                           // Начало на тялото на if.
                    cb->SetForegroundColour(colors.text);                   // Задава цвета на текста на квадратчето за отметка.
                    cb->SetBackgroundColour(m_normalItemColor);             // Задава цвета на фона на квадратчето за отметка.
                } // Край на тялото на if.
                else if (wxStaticText *text = wxDynamicCast(panelChild, wxStaticText)) // Ако дъщерният елемент е wxStaticText.
                {                                                                      // Начало на тялото на else if.
                    text->SetForegroundColour(colors.text);                            // Задава цвета на текста на статичния текст.
                } // Край на тялото на else if.
            } // Край на вътрешния цикъл.
        } // Край на тялото на if.
    } // Край на тялото на цикъла.
    // Празна линия.
    // Apply theme to buttons and other controls // Прилагане на темата към бутони и други контроли.
    wxWindowList &children = GetChildren();                         // Взема списък с всички преки дъщерни прозорци на диалога.
    for (wxWindow *child : children)                                // Итерира през всеки дъщерен прозорец.
    {                                                               // Начало на тялото на цикъла.
        if (wxButton *button = wxDynamicCast(child, wxButton))      // Ако дъщерният прозорец е wxButton.
        {                                                           // Начало на тялото на if.
            if (button->GetId() == wxID_OK)                         // Ако бутонът е "OK".
            {                                                       // Начало на тялото на if.
                button->SetBackgroundColour(wxColour(79, 70, 229)); // Keep primary button special // Запазва специалния цвят на основния бутон (индиго).
            } // Край на тялото на if.
            else                                                       // За всички други бутони (напр. "Cancel").
            {                                                          // Начало на тялото на else.
                button->SetBackgroundColour(colors.button.background); // Задава цвета на фона на бутона според темата.
            } // Край на тялото на else.
            button->SetForegroundColour(colors.button.text); // Задава цвета на текста на бутона според темата.
        } // Край на тялото на if.
        else if (wxStaticText *staticText = wxDynamicCast(child, wxStaticText)) // Ако дъщерният прозорец е wxStaticText.
        {                                                                       // Начало на тялото на else if.
            staticText->SetForegroundColour(colors.text);                       // Задава цвета на текста на статичния текст според темата.
        } // Край на тялото на else if.
    } // Край на тялото на цикъла.
    // Празна линия.
    Refresh(); // Прерисува диалога, за да отрази промените в темата.
} // Край на тялото на метода.
  // Празна линия.
void FilesystemSelectionDialog::SetupColors() // Метод за настройка на специфични цветове, използвани в диалога.
{                                             // Начало на тялото на метода.
    // Get theme colors // Вземане на цветовете на темата.
    ThemeConfig &themeConfig = ThemeConfig::Get();                                         // Взема инстанция на конфигурацията на темата.
    const ThemeColors &colors = themeConfig.GetThemeColors(themeConfig.GetCurrentTheme()); // Взема цветовете за текущата тема.
                                                                                           // Празна линия.
    // Set up base and hover colors // Настройка на основни цветове и цветове при посочване с мишката (hover).
    m_normalItemColor = colors.input.background; // Задава нормалния цвят на фона за елементите (взема се от цвета на фона на полетата за въвеждане в темата).
                                                 // Празна линия.
    // Create a slightly lighter version of the background for hover // Създаване на леко по-светла версия на фона за ефект при посочване.
    wxColour baseColor = m_normalItemColor; // Взема основния цвят.
    // Make hover color 20% lighter (but not exceeding 255) // Прави цвета за hover с 20 единици по-светъл (но не надвишаващ 255).
    int r = std::min(255, baseColor.Red() + 20);   // Изчислява червения компонент.
    int g = std::min(255, baseColor.Green() + 20); // Изчислява зеления компонент.
    int b = std::min(255, baseColor.Blue() + 20);  // Изчислява синия компонент.
    m_hoverItemColor = wxColour(r, g, b);          // Създава цвета за hover.
                                                   // Празна линия.
    // Apply to the scroll panel // Прилагане към превъртащия се панел.
    m_itemsPanel->SetBackgroundColour(m_normalItemColor); // Задава цвета на фона на панела със списъка с елементи.
} // Край на тялото на метода.
  // Празна линия.
void FilesystemSelectionDialog::OnPaint(wxPaintEvent &event)                   // Обработчик на събитието за прерисуване.
{                                                                              // Начало на тялото на метода.
    wxAutoBufferedPaintDC dc(this);                                            // Създава автоматично буфериран контекст за рисуване (за избягване на трептене).
    dc.SetBackground(wxBrush(GetBackgroundColour()));                          // Задава четка за фона с цвета на фона на диалога.
    dc.Clear();                                                                // Изчиства контекста за рисуване с цвета на фона.
                                                                               // Празна линия.
    wxGraphicsContext *gc = wxGraphicsContext::Create(dc);                     // Създава графичен контекст от DC за разширени възможности за рисуване.
    if (gc)                                                                    // Ако графичният контекст е създаден успешно.
    {                                                                          // Начало на тялото на if.
        wxRect rect = GetClientRect();                                         // Взема клиентската област на диалога.
        gc->SetBrush(wxBrush(GetBackgroundColour()));                          // Задава четка за запълване с цвета на фона на диалога.
        gc->SetPen(*wxTRANSPARENT_PEN);                                        // Задава прозрачна писалка (без контур).
        gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);                             // Включва анти-алиасинг за по-гладки ръбове.
        gc->DrawRoundedRectangle(rect.x, rect.y, rect.width, rect.height, 15); // Рисува заоблен правоъгълник за целия диалог с радиус на заобляне 15.
                                                                               // Празна линия.
        // Draw a subtle rounded rectangle around the items panel // Рисуване на фин заоблен правоъгълник около панела с елементи.
        wxRect itemsRect = m_itemsPanel->GetRect();                                                // Взема правоъгълника, обхващащ панела с елементи.
        gc->SetBrush(wxBrush(m_itemsPanel->GetBackgroundColour()));                                // Задава четка с цвета на фона на панела с елементи.
        gc->SetPen(wxPen(wxColour(45, 55, 71), 1));                                                // Задава писалка с тъмносив цвят и дебелина 1 пиксел за контура.
        gc->DrawRoundedRectangle(itemsRect.x, itemsRect.y, itemsRect.width, itemsRect.height, 10); // Рисува заоблен правоъгълник около панела с елементи с радиус 10.
                                                                                                   // Празна линия.
        delete gc;                                                                                 // Освобождава графичния контекст.
    } // Край на тялото на if.
} // Край на тялото на метода.
  // Празна линия.
void FilesystemSelectionDialog::CreateItemList(const wxArrayString &choices) // Метод за създаване на списъка с избираеми елементи (файлови системи).
{                                                                            // Начало на тялото на метода.
    wxBoxSizer *itemsSizer = new wxBoxSizer(wxVERTICAL);                     // Създава sizer с вертикална ориентация за елементите.
    m_itemsPanel->SetSizer(itemsSizer);                                      // Задава този sizer на панела за елементи (m_itemsPanel).
    m_checkboxes.clear();                                                    // Изчиства списъка с квадратчета за отметка.
                                                                             // Празна линия.
    // Add padding at the top // Добавяне на отстъп отгоре.
    itemsSizer->AddSpacer(10); // Добавя празно пространство от 10 пиксела.
                               // Празна линия.
    // Create panels for each item // Създаване на панели за всеки елемент.
    for (const wxString &choiceText : choices)                          // Итерира през всеки избор (текст на файлова система).
    {                                                                   // Начало на тялото на цикъла.
        wxPanel *itemPanel = CreateItemPanel(m_itemsPanel, choiceText); // Създава панел за текущия елемент.
        itemsSizer->Add(itemPanel, 0, wxEXPAND | wxLEFT | wxRIGHT, 10); // Добавя панела на елемента към sizer-а, с разширяване и хоризонтални отстъпи от 10 пиксела.
        itemsSizer->AddSpacer(8);                                       // Space between items // Добавя празно пространство от 8 пиксела между елементите.
    } // Край на тялото на цикъла.
    // Празна линия.
    // Add padding at the bottom // Добавяне на отстъп отдолу.
    itemsSizer->AddSpacer(10); // Добавя празно пространство от 10 пиксела.
                               // Празна линия.
    // Configure scrolling // Конфигуриране на превъртането.
    m_itemsPanel->FitInside();          // Оразмерява превъртащия се панел, така че да пасне на съдържанието си (за правилна работа на плъзгачите).
    m_itemsPanel->SetScrollRate(0, 10); // Задава скоростта на вертикално превъртане (10 пиксела на стъпка).
} // Край на тялото на метода.
  // Празна линия.
wxPanel *FilesystemSelectionDialog::CreateItemPanel(wxWindow *parent, const wxString &choiceText) // Метод за създаване на панел за отделен елемент от списъка.
{                                                                                                 // Начало на тялото на метода.
    wxPanel *panel = new wxPanel(parent, wxID_ANY);                                               // Създава нов панел като дъщерен на подадения родител.
    panel->SetBackgroundColour(m_normalItemColor);                                                // Задава нормалния цвят на фона на панела.
    panel->SetMinSize(wxSize(-1, 50));                                                            // Задава минимална височина на панела от 50 пиксела.
                                                                                                  // Празна линия.
    wxBoxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);                                             // Създава sizer с хоризонтална ориентация за съдържанието на панела.
    sizer->AddSpacer(15);                                                                         // Добавя ляв отстъп от 15 пиксела.
                                                                                                  // Празна линия.
    // Create checkbox // Създаване на квадратче за отметка.
    wxCheckBox *checkBox = new wxCheckBox(panel, wxID_ANY, "");     // Създава квадратче за отметка без текст до него.
    checkBox->SetForegroundColour(*wxWHITE);                        // Задава бял цвят на отметката (ако е видима).
    checkBox->SetBackgroundColour(panel->GetBackgroundColour());    // Задава цвета на фона на квадратчето да е същият като на панела.
    checkBox->SetClientData(new wxString(choiceText));              // Correctly set client data // Задава клиентски данни (текста на избора) към квадратчето.
    sizer->Add(checkBox, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 15); // Добавя квадратчето към sizer-а, с вертикално центриране и десен отстъп от 15 пиксела.
    m_checkboxes.push_back(checkBox);                               // Добавя квадратчето към списъка с квадратчета за отметка.
                                                                    // Празна линия.
    // Create text label // Създаване на текстов етикет.
    wxStaticText *text = new wxStaticText(panel, wxID_ANY, choiceText); // Създава статичен текст с името на файловата система.
    text->SetForegroundColour(*wxWHITE);                                // Задава бял цвят на текста.
    wxFont textFont = text->GetFont();                                  // Взема текущия шрифт на текста.
    textFont.SetPointSize(textFont.GetPointSize() + 1);                 // Увеличава размера на шрифта с 1 пункт.
    text->SetFont(textFont);                                            // Прилага новия шрифт.
    sizer->Add(text, 1, wxALIGN_CENTER_VERTICAL);                       // Добавя текста към sizer-а, позволявайки му да се разширява и с вертикално центриране.
                                                                        // Празна линия.
    // Add right padding // Добавяне на десен отстъп.
    sizer->AddSpacer(15);   // Добавя десен отстъп от 15 пиксела.
    panel->SetSizer(sizer); // Задава sizer-а на панела.
                            // Празна линия.
    // Bind checkbox event // Свързване на събитието на квадратчето за отметка.
    checkBox->Bind(wxEVT_CHECKBOX, &FilesystemSelectionDialog::OnItemCheckbox, this); // Свързва събитието за промяна на състоянието на квадратчето с метода OnItemCheckbox.
                                                                                      // Празна линия.
    // Enhanced hover effect with persistent tracking // Подобрен ефект при посочване с мишката (hover) с постоянно проследяване.
    panel->Bind(wxEVT_ENTER_WINDOW, [this, panel](wxMouseEvent &event) // Свързва събитието за влизане на мишката в областта на панела.
                {                                                      // Начало на ламбда функцията.
                    panel->SetBackgroundColour(m_hoverItemColor);      // Задава цвета на фона на панела на цвета за hover.
                    panel->Refresh(true);                              // Прерисува панела незабавно.
                    event.Skip();
                });                                                    // Позволява на събитието да бъде обработено и от други.
                                                                       // Празна линия.
    panel->Bind(wxEVT_LEAVE_WINDOW, [this, panel](wxMouseEvent &event) // Свързва събитието за излизане на мишката от областта на панела.
                {                                                      // Начало на ламбда функцията.
                    panel->SetBackgroundColour(m_normalItemColor);     // Връща нормалния цвят на фона на панела.
                    panel->Refresh(true);                              // Прерисува панела незабавно.
                    event.Skip();
                }); // Позволява на събитието да бъде обработено и от други.
                    // Празна линия.
    // Make checkbox children inherit hover events // Прави дъщерните елементи на квадратчето за отметка да наследяват събитията за hover.
    for (wxWindow *child : panel->GetChildren())                           // Итерира през дъщерните елементи на панела.
    {                                                                      // Начало на тялото на цикъла.
        child->Bind(wxEVT_ENTER_WINDOW, [this, panel](wxMouseEvent &event) // Свързва събитието за влизане на мишката за всеки дъщерен елемент.
                    {                                                      // Начало на ламбда функцията.
                        panel->SetBackgroundColour(m_hoverItemColor);      // Променя фона на родителския панел.
                        panel->Refresh(true);                              // Прерисува родителския панел.
                        event.Skip();
                    });                                                    // Позволява на събитието да бъде обработено.
                                                                           // Празна линия.
        child->Bind(wxEVT_LEAVE_WINDOW, [this, panel](wxMouseEvent &event) // Свързва събитието за излизане на мишката за всеки дъщерен елемент.
                    {                                                      // Начало на ламбда функцията.
                        // Don't change color if mouse is actually moving between child and parent // Не променяй цвета, ако мишката всъщност се движи между дъщерен елемент и родителския панел.
                        wxPoint mousePos = wxGetMousePosition();   // Взема текущата позиция на мишката на екрана.
                        wxRect panelRect = panel->GetScreenRect(); // Взема правоъгълника на панела на екрана.
                        if (!panelRect.Contains(mousePos))
                        {                                                  // Ако мишката не е върху панела.
                            panel->SetBackgroundColour(m_normalItemColor); // Връща нормалния цвят на фона.
                            panel->Refresh(true);                          // Прерисува панела.
                        } // Край на if.
                        event.Skip();
                    }); // Позволява на събитието да бъде обработено.
    } // Край на тялото на цикъла.
    // Празна линия.
    // Make the entire panel clickable // Прави целия панел кликаем.
    panel->Bind(wxEVT_LEFT_DOWN, [this, checkBox](wxMouseEvent &event) // Свързва събитието за натискане на левия бутон на мишката върху панела.
                {                                                      // Начало на ламбда функцията.
                    checkBox->SetValue(!checkBox->GetValue());         // Обръща състоянието на свързаното квадратче за отметка.
                                                                       // Празна линия.
                    // Create and process a checkbox event manually // Ръчно създаване и обработка на събитие за квадратчето за отметка.
                    wxCommandEvent checkEvent(wxEVT_CHECKBOX, checkBox->GetId()); // Създава ново събитие от тип wxEVT_CHECKBOX.
                    checkEvent.SetEventObject(checkBox);                          // Задава обекта, изпратил събитието.
                    checkEvent.SetInt(checkBox->GetValue());                      // Задава новото състояние на квадратчето (0 или 1).
                    ProcessEvent(checkEvent);                                     // Обработва събитието (това ще извика OnItemCheckbox).
                                                                                  // Празна линия.
                    event.Skip();
                }); // Позволява на събитието да бъде обработено.
                    // Празна линия.
    return panel;   // Връща създадения панел.
} // Край на тялото на метода.
  // Празна линия.
void FilesystemSelectionDialog::OnItemCheckbox(wxCommandEvent &event)                      // Обработчик на събитието за промяна на състоянието на квадратче за отметка.
{                                                                                          // Начало на тялото на метода.
    wxCheckBox *clickedCheckBox = dynamic_cast<wxCheckBox *>(event.GetEventObject());      // Взема квадратчето, което е предизвикало събитието.
    if (!clickedCheckBox)                                                                  // Ако не е валидно квадратче за отметка.
        return;                                                                            // Излиза от метода.
                                                                                           // Празна линия.
    if (clickedCheckBox->GetValue())                                                       // If the clicked checkbox is now checked // Ако кликнатото квадратче сега е маркирано.
    {                                                                                      // Начало на тялото на if.
        m_selectedFilesystem = *static_cast<wxString *>(clickedCheckBox->GetClientData()); // Update selected filesystem // Актуализира избраната файлова система с данните, асоциирани с квадратчето.
                                                                                           // Празна линия.
        for (wxCheckBox *cb : m_checkboxes)                                                // Iterate through all checkboxes // Итерира през всички квадратчета за отметка.
        {                                                                                  // Начало на тялото на цикъла.
            if (cb != clickedCheckBox)                                                     // Uncheck other checkboxes // Ако текущото квадратче не е това, което е било кликнато.
            {                                                                              // Начало на тялото на if.
                cb->SetValue(false);                                                       // Демаркира другите квадратчета (за да се осигури само един избор).
            } // Край на тялото на if.
        } // Край на тялото на цикъла.
    } // Край на тялото на if.
    else                                      // Ако кликнатото квадратче сега е демаркирано.
    {                                         // Начало на тялото на else.
        m_selectedFilesystem = wxEmptyString; // No checkbox is selected // Нулира избраната файлова система (няма избран елемент).
    } // Край на тялото на else.
    event.Skip(); // Позволява на събитието да бъде обработено и от други обработчици.
} // Край на тялото на метода.