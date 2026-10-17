#include "DesktopTab.h"   // Включва заглавния файл за класа DesktopTab
#include <wx/filename.h>  // Включва wxFileName за работа с имена на файлове и пътища
#include <array>          // Включва стандартната C++ библиотека <array> за използване на std::array
#include <wx/textfile.h>  // Включва wxTextFile за работа с текстови файлове
#include "SecondWindow.h" // Включва заглавния файл за класа SecondWindow
#include <wx/file.h>      // Включва wxFile за основни файлови операции
#include <wx/wfstream.h>  // Включва wxFFileInputStream и wxFFileOutputStream за потоци към файлове
#include <wx/txtstrm.h>   // Включва wxTextInputStream и wxTextOutputStream за текстови потоци
#include <wx/process.h>   // Включва wxProcess за управление на външни процеси
#include <wx/weakref.h>   // wxWeakRef за безопасен достъп до таба от асинхронния callback
#include "ContainerCommand.h" // Команди в chroot средата на Docker контейнера

// Декларация на персонализирано събитие
wxDEFINE_EVENT(FILE_COPY_COMPLETE_EVENT, wxCommandEvent); // Дефинира нов тип събитие FILE_COPY_COMPLETE_EVENT, базирано на wxCommandEvent

DesktopTab::DesktopTab(wxWindow *parent)                                   // Конструктор на класа DesktopTab
    : wxPanel(parent), m_gridSizer(nullptr)                                // Инициализира базовия клас wxPanel и член-променливата m_gridSizer на nullptr
{                                                                          // Начало на тялото на конструктора
    CreateDesktopTab();                                                    // Извиква метод за създаване на съдържанието на таба
    Bind(wxEVT_SIZE, &DesktopTab::OnSize, this);                           // Свързва събитието за промяна на размера (wxEVT_SIZE) с метода OnSize
    Bind(FILE_COPY_COMPLETE_EVENT, &DesktopTab::OnFileCopyComplete, this); // Свързва персонализираното събитие FILE_COPY_COMPLETE_EVENT с метода OnFileCopyComplete
} // Край на тялото на конструктора

wxBitmap DesktopTab::LoadImage(const wxString &imageName)                             // Метод за зареждане на изображение
{                                                                                     // Начало на тялото на LoadImage
    wxFileName path(wxFileName::GetCwd(), imageName);                                 // Създава обект wxFileName, комбинирайки текущата работна директория и името на изображението
    path.AppendDir("desktopenvs");                                                    // Добавя поддиректория "desktopenvs" към пътя
    wxLogDebug("DesktopTab::LoadImage - Loading image from: %s", path.GetFullPath()); // Записва в лога пълния път, от който се зарежда изображението
    if (path.FileExists())                                                            // Проверява дали файлът съществува
    {                                                                                 // Начало на if (path.FileExists)
        return wxBitmap(path.GetFullPath(), wxBITMAP_TYPE_PNG);                       // Връща wxBitmap, зареден от файла (указва се тип PNG)
    } // Край на if (path.FileExists)
    wxLogDebug("DesktopTab::LoadImage - Image not found: %s", path.GetFullPath()); // Записва в лога, че изображението не е намерено
    return wxNullBitmap;                                                           // Връща празен битмап (wxNullBitmap), ако изображението не е намерено
} // Край на тялото на LoadImage

wxBitmap DesktopTab::ConvertToGrayscale(const wxBitmap &original)                         // Метод за конвертиране на битмап към сива гама
{                                                                                         // Начало на тялото на ConvertToGrayscale
    wxImage image = original.ConvertToImage();                                            // Конвертира подадения wxBitmap към wxImage
    if (!image.IsOk())                                                                    // Проверява дали конвертирането е успешно
    {                                                                                     // Начало на if (!image.IsOk)
        wxLogDebug("DesktopTab::ConvertToGrayscale - Failed to convert bitmap to image"); // Записва в лога, че конвертирането е неуспешно
        return original;                                                                  // Връща оригиналния битмап
    } // Край на if (!image.IsOk)
    image = image.ConvertToGreyscale(); // Конвертира wxImage към сива гама
    return wxBitmap(image);             // Конвертира обратно към wxBitmap и го връща
} // Край на тялото на ConvertToGrayscale

void DesktopTab::ApplyGrayscaleToCards(const wxString &detectedEnv)                                         // Метод за прилагане на сива гама към картите, които не съответстват на засечената среда
{                                                                                                           // Начало на тялото на ApplyGrayscaleToCards
    wxLogDebug("DesktopTab::ApplyGrayscaleToCards - Applying grayscale for detected env: %s", detectedEnv); // Записва в лога коя е засечената среда

    if (!m_scrolledWindow || IsBeingDeleted())
    {
        wxLogDebug("DesktopTab::ApplyGrayscaleToCards - m_scrolledWindow is invalid or widget is being deleted");
        return;
    }

    wxWindowList &children = m_scrolledWindow->GetChildren();                      // Взема списък с дъщерните елементи на скролируемия прозорец
    for (wxWindowList::iterator it = children.begin(); it != children.end(); ++it) // Итерира през дъщерните елементи
    {                                                                              // Начало на for (children)
        wxPanel *card = wxDynamicCast(*it, wxPanel);                               // Опитва да преобразува текущия елемент към wxPanel (карта)
        if (!card || !card->GetSizer())                                            // Проверява дали преобразуването е успешно и дали картата има sizer
            continue;                                                              // Ако не, преминава към следващия елемент

        wxString cardName = card->GetName();                                                                  // Взема името на картата (което съответства на името на десктоп средата)
        bool isDetectedEnv = (cardName == detectedEnv);                                                       // Проверява дали името на картата съвпада със засечената среда
        wxLogDebug("DesktopTab::ApplyGrayscaleToCards - Card: %s, is detected: %d", cardName, isDetectedEnv); // Записва в лога информация за текущата карта

        wxWindowList &cardChildren = card->GetChildren();                                                     // Взема дъщерните елементи на текущата карта
        for (wxWindowList::iterator childIt = cardChildren.begin(); childIt != cardChildren.end(); ++childIt) // Итерира през тях
        {                                                                                                     // Начало на for (cardChildren)
            wxPanel *imageContainer = wxDynamicCast(*childIt, wxPanel);                                       // Опитва да преобразува към wxPanel (контейнер на изображението)
            if (!imageContainer || !imageContainer->GetClientData())                                          // Проверява дали е валиден панел и има клиентски данни
                continue;                                                                                     // Ако не, преминава нататък

            wxBitmap *storedBitmap = static_cast<wxBitmap *>(imageContainer->GetClientData());           // Преобразува клиентските данни към указател към wxBitmap
            if (storedBitmap && storedBitmap->IsOk())                                                    // Проверява дали битмапът е валиден
            {                                                                                            // Начало на if (storedBitmap)
                if (!isDetectedEnv)                                                                      // Ако текущата карта не е засечената среда
                {                                                                                        // Начало на if (!isDetectedEnv)
                    *storedBitmap = ConvertToGrayscale(*storedBitmap);                                   // Конвертира съхранения битмап към сива гама
                    wxLogDebug("DesktopTab::ApplyGrayscaleToCards - Applied grayscale to %s", cardName); // Записва в лога, че е приложена сива гама
                } // Край на if (!isDetectedEnv)
                imageContainer->Refresh(); // Опреснява контейнера на изображението, за да се покажат промените
            } // Край на if (storedBitmap)
        } // Край на for (cardChildren)

        wxWindowList &grandChildren = card->GetChildren();                                             // Отново взема дъщерните елементи на картата (за да намери бутона)
        for (wxWindowList::iterator gcIt = grandChildren.begin(); gcIt != grandChildren.end(); ++gcIt) // Итерира през тях
        {                                                                                              // Начало на for (grandChildren)
            wxButton *button = wxDynamicCast(*gcIt, wxButton);                                         // Опитва да преобразува към wxButton
            if (button)                                                                                // Ако е бутон
            {                                                                                          // Начало на if (button)
                if (isDetectedEnv)                                                                     // Ако това е картата на засечената среда
                {                                                                                      // Начало на if (isDetectedEnv)
                    button->SetBackgroundColour(wxColour(0, 120, 215));                                // Задава син цвят на фона на бутона
                } // Край на if (isDetectedEnv)
                else                                                   // Ако не е
                {                                                      // Начало на else
                    button->SetBackgroundColour(wxColour(80, 80, 80)); // Задава сив цвят на фона на бутона
                } // Край на else
                button->Refresh(); // Опреснява бутона
            } // Край на if (button)
        } // Край на for (grandChildren)
    } // Край на for (children)

    if (m_scrolledWindow)
    {
        m_scrolledWindow->Refresh(); // Опреснява целия скролируем прозорец
    }
} // Край на тялото на ApplyGrayscaleToCards

void DesktopTab::OnSize(wxSizeEvent &event) // Метод, извиква се при промяна на размера на таба
{                                           // Начало на тялото на OnSize
    if (IsBeingDeleted())
    {
        event.Skip();
        return;
    }

    wxSize size = event.GetSize();                                                                      // Взема новия размер от събитието
    wxLogDebug("DesktopTab::OnSize - Window size changed: %d x %d", size.GetWidth(), size.GetHeight()); // Записва в лога новия размер

    // Извикваме RecalculateLayout само ако размерът е валиден и не е 0
    if (size.GetWidth() > 0 && size.GetHeight() > 0 && m_gridSizer != nullptr)
    {
        RecalculateLayout(size.GetWidth()); // Извиква метод за преизчисляване на разположението, подавайки новата ширина
    }

    event.Skip(); // Позволява на събитието да бъде обработено и от други обработчици
} // Край на тялото на OnSize

void DesktopTab::RecalculateLayout(int windowWidth) // Метод за преизчисляване на разположението на картите в мрежа
{                                                   // Начало на тялото на RecalculateLayout
    if (!m_gridSizer || IsBeingDeleted())           // Проверява дали указателят към sizer-а на мрежата е валиден или ако компонентът се изтрива
        return;                                     // Ако не, прекратява изпълнението

    int availableWidth = windowWidth - 40;                                                             // Изчислява наличната ширина (приблизително, като се вземат предвид отстъпи)
    int cardWithSpacing = 220;                                                                         // Приблизителна ширина на една карта с отстояние
    int columns = std::max(1, availableWidth / cardWithSpacing);                                       // Изчислява броя на колоните, като гарантира поне една
    wxLogDebug("DesktopTab::RecalculateLayout - Window width: %d, Columns: %d", windowWidth, columns); // Записва в лога ширината и броя на колоните

    if (m_gridSizer->GetCols() != columns) // Проверява дали изчисленият брой колони е различен от текущия
    {                                      // Начало на if (m_gridSizer->GetCols() != columns)
        m_gridSizer->SetCols(columns);     // Задава новия брой колони на sizer-а

        // Безопасно премахване на разтегливи колони
        for (int i = 0; i < m_gridSizer->GetCols() && i < m_gridSizer->GetItemCount(); i++) // Итерира през текущите колони с граница
        {                                                                                   // Начало на for
            if (m_gridSizer->IsColGrowable(i))                                              // Проверява дали колоната е разтеглива
            {                                                                               // Начало на if (IsColGrowable)
                m_gridSizer->RemoveGrowableCol(i);                                          // Премахва я от списъка с разтегливи колони
            } // Край на if (IsColGrowable)
        } // Край на for

        // Безопасно добавяне на разтегливи колони
        for (int i = 0; i < columns && i < m_gridSizer->GetItemCount(); i++) // Итерира през новия брой колони с граница
        {                                                                    // Начало на for
            m_gridSizer->AddGrowableCol(i, 1);                               // Добавя всяка колона като разтеглива с пропорция 1
        } // Край на for

        Layout();  // Преизчислява разположението на елементите в таба
        Refresh(); // Опреснява таба, за да се покажат промените
    } // Край на if (m_gridSizer->GetCols() != columns)
} // Край на тялото на RecalculateLayout

wxString DesktopTab::GetGuiFilePath()                                                                  // Метод за получаване на пътя до файла, съдържащ информация за GUI
{                                                                                                      // Начало на тялото на GetGuiFilePath
    wxString guiFilePath;                                                                              // Низ за съхранение на пътя
    wxWindow *parent = GetParent();                                                                    // Взема родителския прозорец
    while (parent)                                                                                     // Итерира нагоре по йерархията на родителите
    {                                                                                                  // Начало на while (parent)
        SecondWindow *secondWindow = wxDynamicCast(parent, SecondWindow);                              // Опитва да преобразува към SecondWindow
        if (secondWindow)                                                                              // Ако преобразуването е успешно
        {                                                                                              // Начало на if (secondWindow)
            guiFilePath = wxFileName(secondWindow->GetProjectDir(), "detected_gui.txt").GetFullPath(); // Формира пътя, използвайки проектната директория от SecondWindow
            wxLogDebug("DesktopTab::GetGuiFilePath - Found SecondWindow, gui path: %s", guiFilePath);  // Записва в лога намерения път
            break;                                                                                     // Прекъсва цикъла
        } // Край на if (secondWindow)
        parent = parent->GetParent(); // Преминава към следващия родител
    } // Край на while (parent)
    if (guiFilePath.IsEmpty())                                                                  // Ако пътят не е намерен чрез SecondWindow
    {                                                                                           // Начало на if (guiFilePath.IsEmpty())
        guiFilePath = wxFileName(wxGetCwd(), "detected_gui.txt").GetFullPath();                 // Използва текущата работна директория
        wxLogDebug("DesktopTab::GetGuiFilePath - No SecondWindow, using cwd: %s", guiFilePath); // Записва в лога, че се използва текущата работна директория
    } // Край на if (guiFilePath.IsEmpty())
    return guiFilePath; // Връща намерения или формирания път
} // Край на тялото на GetGuiFilePath

void DesktopTab::CreateDesktopTab()                              // Метод за създаване на съдържанието на таба
{                                                                // Начало на тялото на CreateDesktopTab
    wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);          // Създава главен вертикален sizer
    m_scrolledWindow = new wxScrolledWindow(this, wxID_ANY);     // Създава скролируем прозорец
    m_scrolledWindow->SetScrollRate(0, 10);                      // Задава скорост на скролиране (0 за хоризонтално, 10 за вертикално)
    m_scrolledWindow->SetBackgroundColour(wxColour(18, 18, 18)); // Задава цвят на фона на скролируемия прозорец

    wxBoxSizer *scrolledSizer = new wxBoxSizer(wxVERTICAL); // Създава sizer за съдържанието на скролируемия прозорец
    m_scrolledWindow->SetSizer(scrolledSizer);              // Задава този sizer на скролируемия прозорец

    wxBoxSizer *headerSizer = new wxBoxSizer(wxHORIZONTAL);                                        // Създава хоризонтален sizer за хедъра
    m_textDisplay = new wxStaticText(m_scrolledWindow, wxID_ANY, "GUI Environment: Not detected"); // Създава статичен текст за показване на GUI среда
    m_textDisplay->SetForegroundColour(*wxWHITE);                                                  // Задава бял цвят на текста
    m_textDisplay->SetFont(wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD)); // Задава шрифт на текста
    headerSizer->Add(m_textDisplay, 1, wxALIGN_LEFT | wxALL, 10);                                  // Добавя текста към хедър sizer-а, разширяващ се, с отстъп

    m_reloadButton = new wxButton(m_scrolledWindow, wxID_ANY, "Reload UI",          // Създава бутон "Reload UI"
                                  wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT); // Стил wxBU_EXACTFIT, за да пасне точно на текста
    m_reloadButton->SetForegroundColour(*wxWHITE);                                  // Задава бял цвят на текста на бутона
    m_reloadButton->SetBackgroundColour(wxColour(60, 60, 60));                      // Задава сив цвят на фона на бутона
    headerSizer->Add(m_reloadButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);     // Добавя бутона към хедър sizer-а, вертикално центриран, с отстъп отдясно

    m_reloadButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent &) // Свързва натискането на бутона с ламбда функция
                         { UpdateTextFromFile(); });            // Ламбда функцията извиква UpdateTextFromFile()

    scrolledSizer->Add(headerSizer, 0, wxEXPAND); // Добавя хедър sizer-а към sizer-а на скролируемия прозорец

    const std::array<wxString, 9> environments = {                                 // Дефинира масив с имената на десктоп средите
                                                  "GNOME", "KDE Plasma", "XFCE",   // Първи ред среди
                                                  "Cinnamon", "MATE", "LXQt",      // Втори ред среди
                                                  "Budgie", "Deepin", "Pantheon"}; // Трети ред среди

    const std::array<wxString, 9> imageFiles = {                                             // Дефинира масив с имената на файловете с изображения за всяка среда
                                                "GNOME.png", "KDE.png", "Xfce.png",          // Изображения за първия ред
                                                "Cinnamon.png", "MATE.png", "LXQt.png",      // Изображения за втория ред
                                                "Budgie.png", "Deepin.png", "Pantheon.png"}; // Изображения за третия ред

    const std::array<wxString, 9> packageNames = {                                                                     // Дефинира масив с имената на пакетите за инсталация на всяка среда
                                                  "gnome-session", "plasma-desktop", "xfce4",                          // Пакети за първия ред
                                                  "cinnamon", "mate-desktop-environment-core", "lxqt",                 // Пакети за втория ред
                                                  "budgie-desktop", "deepin-desktop-environment", "pantheon-desktop"}; // Пакети за третия ред

    const std::array<wxString, 9> sessionNames = {                                  // Имена на сесиите (/usr/share/xsessions/*.desktop) за LightDM
                                                  "gnome", "plasma", "xfce",
                                                  "cinnamon", "mate", "lxqt",
                                                  "budgie-desktop", "deepin", "pantheon"};

    m_gridSizer = new wxFlexGridSizer(3, 20, 20); // Създава wxFlexGridSizer с 3 колони и отстояние 20x20 пиксела
    m_gridSizer->SetFlexibleDirection(wxBOTH);    // Позволява на мрежата да бъде гъвкава и в двете посоки

    for (size_t i = 0; i < environments.size(); ++i)     // Итерира през десктоп средите
    {                                                    // Начало на for (environments)
        wxPanel *card = new wxPanel(m_scrolledWindow);   // Създава панел (карта) за всяка среда
        card->SetBackgroundColour(wxColour(32, 32, 32)); // Задава цвят на фона на картата
        card->SetMinSize(wxSize(200, 300));              // Задава минимален размер на картата
        card->SetName(environments[i]);                  // Задава име на картата (съвпада с името на средата)

        wxBoxSizer *cardSizer = new wxBoxSizer(wxVERTICAL); // Създава вертикален sizer за съдържанието на картата

        wxPanel *imageContainer = new wxPanel(card, wxID_ANY);     // Създава панел-контейнер за изображението
        imageContainer->SetMinSize(wxSize(180, 240));              // Задава минимален размер на контейнера на изображението
        imageContainer->SetBackgroundColour(wxColour(32, 32, 32)); // Задава цвят на фона на контейнера на изображението

        wxBitmap bitmap = LoadImage(imageFiles[i]);              // Зарежда изображението за текущата среда
        if (bitmap.IsOk())                                       // Проверява дали изображението е заредено успешно
        {                                                        // Начало на if (bitmap.IsOk())
            imageContainer->SetClientData(new wxBitmap(bitmap)); // Запазва копие на битмапа като клиентски данни на контейнера

            imageContainer->Bind(wxEVT_PAINT, [this](wxPaintEvent &evt)                              // Свързва събитието за рисуване на контейнера с ламбда функция
                                 {                                                                   // Начало на ламбда функцията за рисуване
                                     wxPanel *panel = dynamic_cast<wxPanel *>(evt.GetEventObject()); // Взема панела, който е извикал събитието
                                     if (!panel)
                                         return; // Ако не е панел, прекратява

                                     wxPaintDC dc(panel);                 // Създава DC за рисуване върху панела
                                     wxSize panelSize = panel->GetSize(); // Взема размера на панела

                                     wxBitmap *bmpPtr = static_cast<wxBitmap *>(panel->GetClientData()); // Взема указателя към битмапа от клиентските данни
                                     if (!bmpPtr || !bmpPtr->IsOk())
                                         return; // Ако няма битмап или не е валиден, прекратява

                                     wxBitmap bmpToDraw = *bmpPtr;         // Създава копие на битмапа за рисуване
                                     wxSize bmpSize = bmpToDraw.GetSize(); // Взема размера на битмапа

                                     double scale = std::min(                                             // Изчислява коефициента на мащабиране, за да пасне изображението в панела, запазвайки пропорциите
                                         static_cast<double>(panelSize.GetWidth()) / bmpSize.GetWidth(),  // Мащаб по ширина
                                         static_cast<double>(panelSize.GetHeight()) / bmpSize.GetHeight() // Мащаб по височина
                                     );                                                                   // Край на std::min

                                     int newWidth = bmpSize.GetWidth() * scale;   // Изчислява новата ширина на изображението
                                     int newHeight = bmpSize.GetHeight() * scale; // Изчислява новата височина на изображението

                                     int x = (panelSize.GetWidth() - newWidth) / 2;   // Изчислява X координата за центриране
                                     int y = (panelSize.GetHeight() - newHeight) / 2; // Изчислява Y координата за центриране

                                     wxImage img = bmpToDraw.ConvertToImage();                           // Конвертира битмапа към wxImage за мащабиране
                                     dc.DrawBitmap(                                                      // Рисува мащабирания битмап
                                         wxBitmap(img.Scale(newWidth, newHeight, wxIMAGE_QUALITY_HIGH)), // Мащабира изображението с високо качество
                                         x, y                                                            // Позиция за рисуване
                                     );
                                 }); // Край на ламбда функцията за рисуване
        } // Край на if (bitmap.IsOk())
        else // Няма изображение в desktopenvs/ - рисува се плочка с името на средата
        {
            wxString envName = environments[i];
            imageContainer->Bind(wxEVT_PAINT, [imageContainer, envName](wxPaintEvent &)
                                 {
                                     wxPaintDC dc(imageContainer);
                                     wxRect rect(imageContainer->GetClientSize());
                                     dc.SetBrush(wxBrush(wxColour(48, 52, 64)));
                                     dc.SetPen(*wxTRANSPARENT_PEN);
                                     dc.DrawRoundedRectangle(rect, 8);
                                     dc.SetFont(wxFont(16, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
                                     dc.SetTextForeground(wxColour(200, 200, 200));
                                     dc.DrawLabel(envName, rect, wxALIGN_CENTER);
                                 });
        }

        cardSizer->Add(imageContainer, 1, wxEXPAND | wxALL, 10); // Добавя контейнера на изображението към sizer-а на картата, разширяващ се, с отстъп

        wxStaticText *title = new wxStaticText(card, wxID_ANY, environments[i]);               // Създава статичен текст за името на средата
        title->SetForegroundColour(*wxWHITE);                                                  // Задава бял цвят на текста
        title->SetFont(wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD)); // Задава шрифт на текста
        cardSizer->Add(title, 0, wxALIGN_LEFT | wxLEFT | wxRIGHT, 10);                         // Добавя заглавието към sizer-а на картата, с отстъп

        wxStaticText *info = new wxStaticText(card, wxID_ANY, "Package: " + packageNames[i]);  // Показва пакета, който ще бъде инсталиран
        info->SetForegroundColour(wxColour(180, 180, 180));                                     // Задава сив цвят на текста
        info->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL)); // Задава шрифт на текста
        cardSizer->Add(info, 0, wxALIGN_LEFT | wxLEFT | wxRIGHT | wxTOP, 10);                   // Добавя информацията към sizer-а на картата, с отстъп

        wxButton *installButton = new wxButton(card, wxID_ANY, "Install"); // Създава бутон "Install"
        installButton->SetMinSize(wxSize(80, 30));                         // Задава минимален размер на бутона
        installButton->SetBackgroundColour(wxColour(0, 120, 215));         // Задава син цвят на фона на бутона
        installButton->SetForegroundColour(*wxWHITE);                      // Задава бял цвят на текста на бутона
        cardSizer->Add(installButton, 0, wxALIGN_LEFT | wxALL, 10);        // Добавя бутона към sizer-а на картата, с отстъп

        installButton->Bind(wxEVT_BUTTON, [this, environments, packageNames, sessionNames, i](wxCommandEvent &event)                     // Свързва натискането на бутона с ламбда функция
                            {                                                                                              // Начало на ламбда функцията за инсталация
                                wxString newEnv = environments[i];                                                         // Взема името на новата среда
                                wxString newPackage = packageNames[i];                                                     // Взема името на пакета за новата среда
                                wxString newSession = sessionNames[i];                                                     // Име на сесията за LightDM
                                wxLogDebug("DesktopTab::InstallButton - Installing %s (package: %s)", newEnv, newPackage); // Записва в лога коя среда се инсталира

                                wxWindow *parent = GetParent();       // Взема родителския прозорец
                                SecondWindow *secondWindow = nullptr; // Указател към SecondWindow
                                wxString containerIdPath;             // Път до файла с ID на контейнера
                                wxString projectDir;                  // Проектна директория

                                while (parent)
                                {                                                       // Търси SecondWindow в йерархията на родителите
                                    secondWindow = wxDynamicCast(parent, SecondWindow); // Опитва преобразуване
                                    if (secondWindow)
                                    {                                                                                                  // Ако е намерен
                                        containerIdPath = wxFileName(secondWindow->GetProjectDir(), "container_id.txt").GetFullPath(); // Формира пътя до container_id.txt
                                        projectDir = secondWindow->GetProjectDir();                                                    // Взема проектната директория
                                        wxLogDebug("DesktopTab::InstallButton - Found SecondWindow, project dir: %s", projectDir);     // Записва в лога
                                        break;                                                                                         // Прекъсва търсенето
                                    } // Край на if (secondWindow)
                                    parent = parent->GetParent(); // Преминава към следващия родител
                                } // Край на while (parent)

                                if (!secondWindow || containerIdPath.IsEmpty())
                                {                                                                                               // Ако SecondWindow или пътят до ID на контейнера не са намерени
                                    wxLogDebug("DesktopTab::InstallButton - Failed to find SecondWindow or container ID path"); // Записва грешка в лога
                                    wxMessageBox("Failed to find container ID!", "Error", wxICON_ERROR);                        // Показва съобщение за грешка
                                    return;                                                                                     // Прекратява
                                } // Край на if (!secondWindow || containerIdPath.IsEmpty())

                                wxFile file;          // Създава обект wxFile
                                wxString containerId; // Низ за ID на контейнера
                                if (file.Open(containerIdPath))
                                {                                                                            // Опитва да отвори файла с ID на контейнера
                                    file.ReadAll(&containerId);                                              // Чете цялото съдържание
                                    containerId.Trim();                                                      // Премахва празни символи
                                    wxLogDebug("DesktopTab::InstallButton - Container ID: %s", containerId); // Записва ID-то в лога
                                }
                                else
                                {                                                                                                   // Ако не успее да отвори файла
                                    wxLogDebug("DesktopTab::InstallButton - Failed to read container_id.txt: %s", containerIdPath); // Записва грешка в лога
                                    wxMessageBox("Failed to read container ID!", "Error", wxICON_ERROR);                            // Показва съобщение за грешка
                                    return;                                                                                         // Прекратява
                                } // Край на else

                                wxString currentEnv;                                                             // Низ за текущата инсталирана среда
                                wxString guiFilePath = wxFileName(projectDir, "detected_gui.txt").GetFullPath(); // Път до файла с информация за текущата GUI
                                if (wxFileExists(guiFilePath))
                                {                                                                                  // Проверява дали файлът съществува
                                    wxFile guiFileRead(guiFilePath);                                               // Използва различно име на променливата за четене
                                    guiFileRead.ReadAll(&currentEnv);                                              // Чете съдържанието
                                    currentEnv.Trim();                                                             // Премахва празни символи
                                    wxLogDebug("DesktopTab::InstallButton - Current environment: %s", currentEnv); // Записва текущата среда в лога
                                }
                                else
                                {                                                                              // Ако файлът не съществува
                                    currentEnv = "Not detected";                                               // Задава стойност по подразбиране
                                    wxLogDebug("DesktopTab::InstallButton - No current environment detected"); // Записва в лога
                                } // Край на else

                                if (currentEnv == newEnv)
                                {                                                                                                      // Ако избраната среда вече е инсталирана
                                    wxLogDebug("DesktopTab::InstallButton - %s is already installed", newEnv);                         // Записва в лога
                                    wxMessageBox("This desktop environment is already installed!", "Info", wxOK | wxICON_INFORMATION); // Показва информационно съобщение
                                    return;                                                                                            // Прекратява
                                } // Край на if (currentEnv == newEnv)

                                wxString currentPackage; // Низ за името на пакета на текущата инсталирана среда
                                for (size_t j = 0; j < environments.size(); ++j)
                                { // Търси пакета на текущата среда
                                    if (environments[j] == currentEnv)
                                    {                                     // Ако намери съвпадение
                                        currentPackage = packageNames[j]; // Запазва името на пакета
                                        break;                            // Прекъсва търсенето
                                    } // Край на if
                                } // Край на for

                                int confirm = wxMessageBox(                                               // Показва диалогов прозорец за потвърждение
                                    (currentPackage.IsEmpty() ? wxString::Format("Install %s into the ISO?", newEnv) : wxString::Format("Uninstall %s and install %s?", currentEnv, newEnv)), // Текст на въпроса
                                    "Confirm Desktop Environment Change",                                 // Заглавие на диалога
                                    wxYES_NO | wxICON_QUESTION                                            // Бутони YES/NO и икона с въпрос
                                );                                                                        // Край на wxMessageBox
                                if (confirm != wxYES)
                                {                                                                          // Ако потребителят не потвърди
                                    wxLogDebug("DesktopTab::InstallButton - User cancelled installation"); // Записва в лога
                                    return;                                                                // Прекратява
                                } // Край на if (confirm != wxYES)

                                wxButton *btn = wxDynamicCast(event.GetEventObject(), wxButton); // Взема указател към натиснатия бутон
                                if (btn)
                                    btn->Disable();                                                               // Деактивира бутона
                                wxLogDebug("DesktopTab::InstallButton - Disabled install button for %s", newEnv); // Записва в лога

                                // Скриптът се изпълнява в chroot средата на ISO образа (поддържат се Debian/Ubuntu базирани образи)
                                wxString script = "if ! command -v apt-get >/dev/null 2>&1; then\n"
                                                  "    echo 'Only Debian/Ubuntu based images are supported'\n"
                                                  "    exit 90\n"
                                                  "fi\n"
                                                  "apt-get update || exit 1\n";
                                if (!currentPackage.IsEmpty())                                                                          // Ако текущата среда е известна, тя се премахва
                                    script += "apt-get purge -y " + ContainerCommand::ShellQuote(currentPackage) + " || true\n";
                                script += "apt-get install -y " + ContainerCommand::ShellQuote(newPackage) + " || exit 1\n";           // Инсталира новата среда
                                if (!currentPackage.IsEmpty())
                                    script += "apt-get autoremove -y || true\n";                                                         // Премахва пакетите на старата среда
                                script += "mkdir -p /etc/lightdm/lightdm.conf.d\n"                                                       // LightDM стартира новата сесия по подразбиране
                                          "printf '[Seat:*]\\nuser-session=%s\\n' " + ContainerCommand::ShellQuote(newSession) +
                                          " > /etc/lightdm/lightdm.conf.d/90-linuxisopro.conf\n";

                                wxWeakRef<DesktopTab> self(this);    // Табът може да бъде затворен, докато инсталацията върви
                                wxWeakRef<wxButton> buttonRef(btn);  // Бутонът се активира отново след края
                                long pid = ContainerCommand::RunInChrootAsync(containerId, script, [self, buttonRef, newEnv](int status)
                                {
                                    if (buttonRef)
                                        buttonRef->Enable();
                                    if (!self)
                                        return;

                                    if (status != 0) // apt-get е върнал грешка (напр. пакетът не съществува за тази дистрибуция)
                                    {
                                        wxString reason = (status == 90) ? wxString("Only Debian/Ubuntu based images are supported.")
                                                                         : wxString::Format("apt-get failed with exit code %d. The package may not be available for this distribution.", status);
                                        wxMessageBox("Installing " + newEnv + " failed.\n\n" + reason, "Installation Failed", wxOK | wxICON_ERROR, self);
                                        return;
                                    }

                                    wxFile guiFile; // Записва новата среда, за да се показва след презареждане
                                    if (guiFile.Create(self->GetGuiFilePath(), true) && guiFile.IsOpened())
                                        guiFile.Write(newEnv);
                                    self->UpdateGUILabel(newEnv);
                                    wxMessageBox(newEnv + " was installed into the ISO.", "Success", wxOK | wxICON_INFORMATION, self);
                                });
                                if (pid == 0)
                                {                                                                                       // Ако стартирането на процеса е неуспешно
                                    wxLogDebug("DesktopTab::InstallButton - Failed to execute command for %s", newEnv); // Записва грешка в лога
                                    wxMessageBox("Failed to start the installation. Is Docker Desktop running?", "Error", wxOK | wxICON_ERROR, this);
                                    if (btn)
                                        btn->Enable(); // Активира отново бутона
                                }
                            }); // Край на ламбда функцията за инсталация

        card->SetSizer(cardSizer);           // Задава sizer-а на картата
        m_gridSizer->Add(card, 1, wxEXPAND); // Добавя картата към мрежата, разширяваща се
    } // Край на for (environments)

    scrolledSizer->Add(m_gridSizer, 1, wxEXPAND | wxALL, 20); // Добавя мрежата към sizer-а на скролируемия прозорец, с отстъп
    mainSizer->Add(m_scrolledWindow, 1, wxEXPAND);            // Добавя скролируемия прозорец към главния sizer, разширяващ се
    SetSizer(mainSizer);                                      // Задава главния sizer на таба

    CallAfter([this]()
              { RecalculateLayout(GetSize().GetWidth()); });                       // Планира извикване на RecalculateLayout след текущата обработка на събития, използвайки ламбда
    wxLogDebug("DesktopTab::CreateDesktopTab - Desktop tab created and laid out"); // Записва в лога, че табът е създаден
} // Край на тялото на CreateDesktopTab

void DesktopTab::UpdateGUILabel(const wxString &guiName)                                 // Метод за обновяване на етикета с името на GUI среда
{                                                                                        // Начало на тялото на UpdateGUILabel
    wxLogDebug("DesktopTab::UpdateGUILabel - Setting label with GUI name: %s", guiName); // Записва в лога името на GUI

    if (IsBeingDeleted())
    {
        wxLogDebug("DesktopTab::UpdateGUILabel - Widget is being deleted");
        return;
    }

    if (!m_textDisplay)
    {
        wxLogDebug("DesktopTab::UpdateGUILabel - m_textDisplay is null");
        return;
    }

    wxString displayText = "GUI Environment: " + guiName; // Формира текста за показване
    m_textDisplay->SetLabel(displayText);                 // Задава новия текст на етикета

    // Проверяваме дали можем безопасно да извикаме ApplyGrayscaleToCards
    if (m_scrolledWindow && !IsBeingDeleted())
    {
        ApplyGrayscaleToCards(guiName); // Прилага сива гама към картите
    }

    m_textDisplay->Refresh(); // Опреснява етикета

    if (m_scrolledWindow)
    {
        m_scrolledWindow->Layout(); // Преизчислява разположението на скролируемия прозорец
    }

    Refresh(true); // Опреснява таба (с изтриване на фона)
    Update();      // Налага незабавно обновяване на таба

    wxWindow *parent = GetParent();
    if (parent)
    {
        parent->Refresh(true); // Опреснява родителския прозорец
        parent->Update();      // Налага незабавно обновяване на родителския прозорец
    }

    wxLogDebug("DesktopTab::UpdateGUILabel - UI updated with %s", guiName); // Записва в лога, че UI е обновен
} // Край на тялото на UpdateGUILabel

void DesktopTab::OnFileCopyComplete(wxCommandEvent &event)                                    // Метод, извиква се при получаване на събитие FILE_COPY_COMPLETE_EVENT
{                                                                                             // Начало на тялото на OnFileCopyComplete
    wxString guiName = event.GetString();                                                     // Взема низа (името на GUI) от събитието
    wxLogDebug("DesktopTab::OnFileCopyComplete - Event received with GUI name: %s", guiName); // Записва в лога полученото име

    guiName.Trim(true).Trim(false);                               // Премахва празни символи от началото и края на името
    UpdateGUILabel(guiName.IsEmpty() ? "Not detected" : guiName); // Обновява етикета (ако името е празно, показва "Not detected")
} // Край на тялото на OnFileCopyComplete

void DesktopTab::UpdateTextFromFile()                                        // Метод за обновяване на текста от файл
{                                                                            // Начало на тялото на UpdateTextFromFile
    wxLogDebug("DesktopTab::UpdateTextFromFile - Reload UI button clicked"); // Записва в лога, че бутонът е натиснат

    wxString guiFilePath = GetGuiFilePath();                                          // Взема пътя до файла с информация за GUI
    wxLogDebug("DesktopTab::UpdateTextFromFile - Looking for file: %s", guiFilePath); // Записва в лога търсения път

    if (wxFileExists(guiFilePath))                                                    // Проверява дали файлът съществува
    {                                                                                 // Начало на if (wxFileExists)
        wxFile file(guiFilePath);                                                     // Създава обект wxFile
        wxString content;                                                             // Низ за съдържанието на файла
        if (file.ReadAll(&content))                                                   // Опитва да прочете цялото съдържание на файла
        {                                                                             // Начало на if (file.ReadAll)
            content.Trim(true).Trim(false);                                           // Премахва празни символи
            wxLogDebug("DesktopTab::UpdateTextFromFile - Read content: %s", content); // Записва прочетеното съдържание в лога
            UpdateGUILabel(content);                                                  // Обновява етикета с прочетеното съдържание
        } // Край на if (file.ReadAll)
        else                                                                             // Ако четенето е неуспешно
        {                                                                                // Начало на else
            wxLogDebug("DesktopTab::UpdateTextFromFile - Failed to read file contents"); // Записва грешка в лога
            UpdateGUILabel("Not detected");                                              // Обновява етикета с "Not detected"
        } // Край на else
    } // Край на if (wxFileExists)
    else                                                                                // Ако файлът не съществува
    {                                                                                   // Начало на else
        wxLogDebug("DesktopTab::UpdateTextFromFile - File not found: %s", guiFilePath); // Записва в лога, че файлът не е намерен
        UpdateGUILabel("Not detected");                                                 // Обновява етикета с "Not detected"
    } // Край на else
} // Край на тялото на UpdateTextFromFile