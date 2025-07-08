#include "LinuxTerminalPanel.h" // Включва заглавния файл за LinuxTerminalPanel.
#include <wx/sizer.h>           // Включва заглавния файл за wxSizer, за управление на оформлението.
                                // Празен ред.
#ifdef __WXGTK__                // Начало на условна компилация: кодът по-долу се компилира само ако се използва wxGTK (Linux).
#include <gtk/gtk.h>            // Включва заглавния файл на GTK (GIMP Toolkit).
#include <gdk/gdkx.h>           // Включва заглавния файл на GDK X11 функции (GIMP Drawing Kit).
#include <sys/ioctl.h>          // Включва заглавния файл за ioctl (input/output control).
#include <termios.h>            // Включва заглавния файл за POSIX терминален контрол.
#include <fcntl.h>              // Включва заглавния файл за контрол на файлови дескриптори.
#include <unistd.h>             // Включва заглавния файл за POSIX API (напр. fork, pipe).
#endif                          // Край на условната компилация за __WXGTK__.
                                // Празен ред.
// Define the static constants // Дефинира статични константи за класа.
const int LinuxTerminalPanel::TERM_CHAR_WIDTH = 8;               // Дефинира константа за ширината на символ в терминала (в пиксели).
const int LinuxTerminalPanel::TERM_CHAR_HEIGHT = 16;             // Дефинира константа за височината на символ в терминала (в пиксели).
const int LinuxTerminalPanel::TERM_SCROLLBAR_WIDTH = 12;         // Дефинира константа за ширината на плъзгача на терминала (в пиксели).
const int LinuxTerminalPanel::TERM_PADDING = 2;                  // Дефинира константа за отстъпа в терминала (в пиксели).
const int LinuxTerminalPanel::TERM_MIN_COLS = 80;                // Дефинира константа за минималния брой колони в терминала.
const int LinuxTerminalPanel::TERM_MIN_ROWS = 24;                // Дефинира константа за минималния брой редове в терминала.
                                                                 // Празен ред.
wxBEGIN_EVENT_TABLE(LinuxTerminalPanel, wxPanel)                 // Начало на дефиницията на таблицата със събития за класа LinuxTerminalPanel, който наследява wxPanel.
#ifdef __WXGTK__                                                 // Начало на условна компилация: кодът по-долу се компилира само ако се използва wxGTK.
    EVT_SIZE(LinuxTerminalPanel::OnSize)                         // Свързва събитието за промяна на размера (EVT_SIZE) с метода OnSize на класа.
    EVT_MOUSE_EVENTS(LinuxTerminalPanel::OnMouseEvent)           // Свързва събитията на мишката (EVT_MOUSE_EVENTS) с метода OnMouseEvent на класа.
    EVT_KEY_DOWN(LinuxTerminalPanel::OnKeyEvent)                 // Свързва събитието за натиснат клавиш (EVT_KEY_DOWN) с метода OnKeyEvent на класа.
#endif                                                           // Край на условната компилация за __WXGTK__.
    wxEND_EVENT_TABLE()                                          // Край на дефиницията на таблицата със събития.
                                                                 // Празен ред.
    LinuxTerminalPanel::LinuxTerminalPanel(wxWindow *parent)     // Конструктор на класа LinuxTerminalPanel, приема родителски прозорец.
    : wxPanel(parent, wxID_ANY)                                  // Извиква конструктора на базовия клас wxPanel с родителя и произволно ID.
{                                                                // Начало на тялото на конструктора.
#ifdef __WXGTK__                                                 // Начало на условна компилация: кодът по-долу се компилира само ако се използва wxGTK.
    panelId = 0;                                                 // Инициализира panelId (идентификатор на панела за X11) с 0.
    terminalProcess = nullptr;                                   // Инициализира указателя към процеса на терминала с nullptr.
    m_display = nullptr;                                         // Инициализира указателя към X11 дисплея с nullptr.
    m_initialized = false;                                       // Инициализира флага за инициализация на терминала с false.
                                                                 // Празен ред.
    SetBackgroundColour(*wxBLACK);                               // Задава черен цвят на фона на панела.
                                                                 // Празен ред.
    wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);          // Създава нов wxBoxSizer с вертикална ориентация.
                                                                 // Празен ред.
    m_terminalContainer = new wxPanel(this, wxID_ANY);           // Създава вложен панел (контейнер) за терминала.
    m_terminalContainer->SetBackgroundColour(*wxBLACK);          // Задава черен цвят на фона на контейнера на терминала.
                                                                 // Празен ред.
    mainSizer->Add(m_terminalContainer, 1, wxEXPAND | wxALL, 0); // Добавя контейнера на терминала към главния sizer, като му позволява да се разширява.
                                                                 // Празен ред.
    SetSizer(mainSizer);                                         // Задава главния sizer за този панел.
                                                                 // Празен ред.
    m_display = XOpenDisplay(NULL);                              // Отваря връзка към X сървъра (NULL означава текущия дисплей).
                                                                 // Празен ред.
    CallAfter(&LinuxTerminalPanel::InitializeTerminal);          // Планира извикването на метода InitializeTerminal след като текущият обработчик на събития приключи.
#else                                                            // Начало на алтернативния блок за компилация (ако не е __WXGTK__).
  // Create a simple text control as fallback for non-Linux systems // Създаване на обикновено текстово поле като заместител за системи, различни от Linux.
    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);                                        // Създава нов wxBoxSizer с вертикална ориентация.
    m_textCtrl = new wxTextCtrl(this, wxID_ANY,                                            // Създава ново текстово поле за многоредов текст, само за четене.
                                "Terminal emulation is only available on Linux systems.\n" // Съобщение, че емулацията на терминал е налична само за Linux.
                                "This is a placeholder text control.",                     // Това е заместващо текстово поле.
                                wxDefaultPosition, wxDefaultSize,                          // Използва подразбиращи се позиция и размер.
                                wxTE_MULTILINE | wxTE_READONLY);                           // Задава стилове: многоредов и само за четене.
    sizer->Add(m_textCtrl, 1, wxEXPAND | wxALL, 5);                                        // Добавя текстовото поле към sizer-а, позволявайки му да се разширява и добавяйки отстъп от 5 пиксела.
    SetSizer(sizer);                                                                       // Задава sizer-а за този панел.
#endif                                                           // Край на условната компилация.
} // Край на тялото на конструктора.
  // Празен ред.
LinuxTerminalPanel::~LinuxTerminalPanel() // Деструктор на класа LinuxTerminalPanel.
{                                         // Начало на тялото на деструктора.
#ifdef __WXGTK__                          // Начало на условна компилация: кодът по-долу се компилира само ако се използва wxGTK.
    CleanupTerminal();                    // Извиква метода за почистване на терминала.
    if (m_display)
    {                             // Проверява дали указателят към X11 дисплея е валиден.
        XCloseDisplay(m_display); // Затваря връзката към X сървъра.
        m_display = nullptr;      // Нулира указателя към X11 дисплея.
    } // Край на проверката за m_display.
#endif // Край на условната компилация за __WXGTK__.
} // Край на тялото на деструктора.
  // Празен ред.
#ifdef __WXGTK__                              // Начало на условна компилация: кодът по-долу се компилира само ако се използва wxGTK.
void LinuxTerminalPanel::InitializeTerminal() // Метод за инициализация на терминала.
{                                             // Начало на тялото на метода InitializeTerminal.
    if (!m_terminalContainer || !m_display)
        return;                                                                                    // Ако контейнерът на терминала или X11 дисплеят не са валидни, прекратява изпълнението.
                                                                                                   // Празен ред.
    gtk_widget_realize(GTK_WIDGET(m_terminalContainer->GetHandle()));                              // Реализира GTK уиджета на контейнера на терминала (създава асоциирания GdkWindow).
    panelId = GDK_WINDOW_XID(gtk_widget_get_window(GTK_WIDGET(m_terminalContainer->GetHandle()))); // Взема X11 ID на прозореца на контейнера на терминала.
                                                                                                   // Празен ред.
    if (panelId == 0)
    {                                                                   // Проверява дали X11 ID на прозореца е получен успешно.
        wxLogError("Failed to get the window ID for embedding xterm."); // Записва грешка, че не е успяло да се вземе ID-то на прозореца за вграждане на xterm.
        return;                                                         // Прекратява изпълнението.
    } // Край на проверката за panelId.
    // Празен ред.
    wxSize containerSize = m_terminalContainer->GetClientSize(); // Взема клиентския размер на контейнера на терминала.
                                                                 // Празен ред.
    // Calculate columns and rows // Изчисляване на колони и редове.
    int cols = (containerSize.GetWidth() - TERM_SCROLLBAR_WIDTH) / TERM_CHAR_WIDTH; // Изчислява броя на колоните, като се взема предвид ширината на плъзгача.
    int rows = containerSize.GetHeight() / TERM_CHAR_HEIGHT;                        // Изчислява броя на редовете.
                                                                                    // Празен ред.
    cols = std::max(cols, TERM_MIN_COLS);                                           // Уверява се, че броят на колоните не е по-малък от минимално допустимия.
    rows = std::max(rows, TERM_MIN_ROWS);                                           // Уверява се, че броят на редовете не е по-малък от минимално допустимия.
                                                                                    // Празен ред.
    // Build xterm command with specific settings for better text wrapping // Формира командата за стартиране на xterm с различни параметри.
    wxString command = wxString::Format(              // Форматира низ за командата за стартиране на xterm.
        "xterm "                                      // Команда xterm.
        "-into %ld "                                  // Опция за вграждане в прозорец с дадено XID.
        "-bg black "                                  // Задава черен цвят на фона.
        "-fg white "                                  // Задава бял цвят на текста.
        "-fa 'Monospace' "                            // Задава моноширинен шрифт.
        "-fs 10 "                                     // Задава размер на шрифта 10.
        "-geometry %dx%d "                            // Задава геометрията (колони x редове) на терминала.
        "-b 0 "                                       // Задава вътрешен отстъп на 0.
        "+sb "                                        // Enable scrollbar // Активира плъзгача.
        "-rightbar "                                  // Поставя плъзгача отдясно.
        "-aw "                                        // Enable autowrap // Активира автоматично пренасяне на текст (autowrap).
        "-mk "                                        // Enable margin bell // Активира звънец при достигане на полето (margin bell).
        "-si "                                        // Enable scroll on tty output // Активира превъртане при изход от TTY (scroll on TTY output).
        "-sk "                                        // Enable scroll on keypress // Активира превъртане при натискане на клавиш (scroll on keypress).
        "-sl 5000 "                                   // Scrollback lines // Задава 5000 реда за история на превъртане (scrollback).
        "-bc "                                        // Enable text cursor blinking // Активира мигане на текстовия курсор.
        "-bcf 500 "                                   // Blink frequency // Честота на мигане на курсора (500ms).
        "-wf "                                        // Enable wide font // Активира поддръжка на широки шрифтове (wide font).
        "-cc 33:48,35-38:48,40-58:48,63-255:48 "      // Character classes // Дефинира класове на символи (за маркиране на думи и др.).
        "-e bash -c '"                                // Изпълнява следната команда в bash.
        "export COLUMNS=%d; "                         // Експортира променливата на средата COLUMNS.
        "export LINES=%d; "                           // Експортира променливата на средата LINES.
        "cd \"%s\"; "                                 // Сменя текущата директория с работната директория на приложението.
        "stty columns $COLUMNS rows $LINES; "         // Задава размера на терминала (колони и редове) чрез stty.
        "echo \"Terminal size: $COLUMNS x $LINES\"; " // Отпечатва размера на терминала.
        "docker pull ubuntu && "                      // Изтегля последната версия на Ubuntu Docker image.
        "docker run --privileged -it "                // Стартира Docker контейнер с привилегии, интерактивен (-i) и с псевдо-TTY (-t).
        "--rm "                                       // Премахва контейнера автоматично след излизане.
        "-e TERM=xterm-256color "                     // Задава типа на терминала в контейнера.
        "-e COLUMNS=$COLUMNS "                        // Предава броя на колоните към контейнера.
        "-e LINES=$LINES "                            // Предава броя на редовете към контейнера.
        "ubuntu bash --login'",                       // Стартира bash в Ubuntu контейнера като login shell.
        panelId,                                      // XID на панела, в който да се вгради xterm.
        cols, rows,                                   // Геометрия (колони, редове) за xterm.
        cols, rows,                                   // Стойности за COLUMNS и LINES в bash скрипта.
        wxGetCwd());                                  // Текуща работна директория.
                                                      // Празен ред.
    terminalProcess = wxProcess::Open(command);       // Стартира процеса на xterm с конструираната команда.
                                                      // Празен ред.
    if (!terminalProcess)
    {                                                  // Проверява дали процесът на терминала е стартиран успешно.
        wxLogError("Failed to launch xterm process."); // Записва грешка, че не е успяло да се стартира процесът на xterm.
        return;                                        // Прекратява изпълнението.
    } // Край на проверката за terminalProcess.
    // Празен ред.
    m_initialized = true;          // Задава флага за инициализация на терминала на true.
    wxMilliSleep(100);             // Изчаква 100 милисекунди, за да даде време на xterm да се инициализира.
    ResizeTerminal(containerSize); // Преоразмерява терминала според размера на контейнера.
} // Край на тялото на метода InitializeTerminal.
  // Празен ред.
void LinuxTerminalPanel::CleanupTerminal() // Метод за почистване и спиране на терминала.
{                                          // Начало на тялото на метода CleanupTerminal.
    if (terminalProcess)
    {                                                                // Проверява дали има активен процес на терминала.
        terminalProcess->Detach();                                   // Откача процеса, така че да не бъде убит автоматично при унищожаване на wxProcess обекта.
        terminalProcess->Kill(terminalProcess->GetPid(), wxSIGKILL); // Изпраща сигнал SIGKILL за принудително спиране на процеса.
        delete terminalProcess;                                      // Освобождава паметта, заета от wxProcess обекта.
        terminalProcess = nullptr;                                   // Нулира указателя към процеса на терминала.
    } // Край на проверката за terminalProcess.
    m_initialized = false; // Задава флага за инициализация на терминала на false.
} // Край на тялото на метода CleanupTerminal.
  // Празен ред.
void LinuxTerminalPanel::ResizeTerminal(const wxSize &size) // Метод за преоразмеряване на терминала.
{                                                           // Начало на тялото на метода ResizeTerminal.
    if (!m_initialized || !m_display || panelId == 0)
        return; // Ако терминалът не е инициализиран, X11 дисплеят не е валиден или panelId е 0, прекратява изпълнението.
                // Празен ред.
    // Calculate terminal dimensions accounting for scrollbar // Изчисляване на размерите на терминала, като се взема предвид плъзгача.
    int cols = std::max(TERM_MIN_COLS, (size.GetWidth() - TERM_SCROLLBAR_WIDTH) / TERM_CHAR_WIDTH); // Изчислява броя на колоните, като осигурява минимален брой и отчита ширината на плъзгача.
    int rows = std::max(TERM_MIN_ROWS, size.GetHeight() / TERM_CHAR_HEIGHT);                        // Изчислява броя на редовете, като осигурява минимален брой.
                                                                                                    // Празен ред.
    // Resize the X window // Преоразмеряване на X прозореца.
    XResizeWindow(m_display, panelId, size.GetWidth(), size.GetHeight()); // Преоразмерява X11 прозореца, в който е вграден xterm.
    XFlush(m_display);                                                    // Изпраща всички буферирани X команди към X сървъра.
                                                                          // Празен ред.
    // Update terminal dimensions // Актуализиране на размерите на терминала.
    if (terminalProcess)
    {                                        // Проверява дали има активен процес на терминала.
        int pid = terminalProcess->GetPid(); // Взема PID (Process ID) на процеса на xterm.
                                             // Празен ред.
        // Try all possible TTY paths // Опитва да намери пътя до TTY през възможни файлови дескриптори.
        const char *ttyPaths[] = {
            // Масив от възможни пътища до файловите дескриптори на TTY.
            "/proc/%d/fd/0", // Стандартен вход.
            "/proc/%d/fd/1", // Стандартен изход.
            "/proc/%d/fd/2"  // Стандартна грешка.
        }; // Край на масива ttyPaths.
        // Празен ред.
        bool resizeSuccess = false; // Флаг, показващ дали преоразмеряването е успешно.
        for (const char *ttyPathFormat : ttyPaths)
        {                                                            // Итерира през възможните пътища до TTY.
            wxString ttyPath = wxString::Format(ttyPathFormat, pid); // Форматира пътя с PID на процеса.
            int fd = open(ttyPath.c_str(), O_RDWR);                  // Опитва да отвори файловия дескриптор за четене и писане.
            if (fd != -1)
            { // Ако файловият дескриптор е отворен успешно.
                // Set terminal size // Задаване на размера на терминала.
                struct winsize ws;               // Структура за съхранение на размера на прозореца на терминала.
                ws.ws_col = cols;                // Задава броя на колоните.
                ws.ws_row = rows;                // Задава броя на редовете.
                ws.ws_xpixel = size.GetWidth();  // Задава ширината в пиксели.
                ws.ws_ypixel = size.GetHeight(); // Задава височината в пиксели.
                                                 // Празен ред.
                if (ioctl(fd, TIOCSWINSZ, &ws) == 0)
                {                         // Използва ioctl за задаване на размера на прозореца на терминала (TIOCSWINSZ).
                    resizeSuccess = true; // Преоразмеряването е успешно.
                                          // Празен ред.
                    // Update environment variables // Актуализиране на променливите на средата.
                    char envColumns[32], envLines[32];                            // Буфери за низовете на променливите на средата.
                    snprintf(envColumns, sizeof(envColumns), "COLUMNS=%d", cols); // Форматира низа за COLUMNS.
                    snprintf(envLines, sizeof(envLines), "LINES=%d", rows);       // Форматира низа за LINES.
                    putenv(envColumns);                                           // Задава променливата на средата COLUMNS за текущия процес (и неговите деца, ако ги наследяват).
                    putenv(envLines);                                             // Задава променливата на средата LINES.
                                                                                  // Празен ред.
                    // Update stty settings // Актуализиране на настройките на stty.
                    wxString sttyCmd = wxString::Format("stty columns %d rows %d\n", cols, rows); // Форматира командата за stty.
                    write(fd, sttyCmd.c_str(), sttyCmd.length());                                 // Записва командата за stty във файловия дескриптор на TTY.
                } // Край на проверката за ioctl.
                  // Празен ред.
                close(fd); // Затваря файловия дескриптор.
                if (resizeSuccess)
                    break; // Ако преоразмеряването е успешно, излиза от цикъла.
            } // Край на проверката за fd.
        } // Край на цикъла for.
        // Празен ред.
        if (resizeSuccess)
        { // Ако преоразмеряването на TTY е било успешно.
            // Send SIGWINCH to terminal and all child processes // Изпращане на SIGWINCH до терминала и всички дъщерни процеси.
            kill(pid, SIGWINCH); // Изпраща сигнал SIGWINCH (промяна на размера на прозореца) до процеса на xterm.
                                 // Празен ред.
            // Find and signal all child processes // Намиране и сигнализиране на всички дъщерни процеси.
            wxString pgrepCmd = wxString::Format("pgrep -P %d", pid); // Форматира команда за pgrep за намиране на дъщерни процеси на xterm.
            wxArrayString output;                                     // Масив от низове за съхранение на изхода от командата.
            wxExecute(pgrepCmd, output);                              // Изпълнява командата pgrep и съхранява изхода.
                                                                      // Празен ред.
            for (const wxString &childPid : output)
            {                     // Итерира през намерените PID на дъщерни процеси.
                long childPidNum; // Променлива за съхранение на PID като число.
                if (childPid.ToLong(&childPidNum))
                {                                // Преобразува низа с PID в число.
                    kill(childPidNum, SIGWINCH); // Изпраща сигнал SIGWINCH до дъщерния процес.
                                                 // Празен ред.
                    // Also try to update Docker container if it exists // Опитва се също да актуализира размера на терминала в Docker контейнера, ако съществува.
                    wxString dockerCmd = wxString::Format(                                  // Форматира команда за Docker.
                        "docker exec $(docker ps -q) /bin/sh -c 'stty columns %d rows %d'", // Изпълнява stty в активния Docker контейнер.
                        cols, rows);                                                        // Колони и редове за stty командата.
                    wxExecute(dockerCmd);                                                   // Изпълнява командата за Docker.
                } // Край на проверката за ToLong.
            } // Край на цикъла for.
        } // Край на проверката за resizeSuccess.
    } // Край на проверката за terminalProcess.
    // Празен ред.
    // Update container and refresh // Актуализиране на контейнера и обновяване.
    m_terminalContainer->SetSize(size); // Задава размера на контейнера на терминала.
    m_terminalContainer->Refresh();     // Прерисува контейнера на терминала.
    Layout();                           // Преизчислява оформлението на панела.
} // Край на тялото на метода ResizeTerminal.
  // Празен ред.
void LinuxTerminalPanel::OnSize(wxSizeEvent &event) // Обработчик на събитието за промяна на размера на панела.
{                                                   // Начало на тялото на метода OnSize.
    const wxSize &size = event.GetSize();           // Взема новия размер от събитието.
    ResizeTerminal(size);                           // Извиква метода за преоразмеряване на терминала с новия размер.
    event.Skip();                                   // Позволява на събитието да бъде обработено и от други обработчики.
} // Край на тялото на метода OnSize.
  // Празен ред.
void LinuxTerminalPanel::OnTerminalSize(wxSizeEvent &event) // Обработчик на събитие за промяна на размера на терминала (не е свързан в текущата таблица на събитията).
{                                                           // Начало на тялото на метода OnTerminalSize.
    const wxSize &size = event.GetSize();                   // Взема новия размер от събитието.
    ResizeTerminal(size);                                   // Извиква метода за преоразмеряване на терминала с новия размер.
    event.Skip();                                           // Позволява на събитието да бъде обработено и от други обработчики.
} // Край на тялото на метода OnTerminalSize.
  // Празен ред.
void LinuxTerminalPanel::FocusOnTerminal() // Метод за фокусиране върху терминала.
{                                          // Начало на тялото на метода FocusOnTerminal.
    if (panelId != 0 && m_display)
    {                                                                    // Проверява дали panelId е валиден и X11 дисплеят е отворен.
        XSetInputFocus(m_display, panelId, RevertToParent, CurrentTime); // Задава фокуса на въвеждане към X11 прозореца на терминала.
        XFlush(m_display);                                               // Изпраща командата към X сървъра.
    } // Край на проверката.
} // Край на тялото на метода FocusOnTerminal.
  // Празен ред.
void LinuxTerminalPanel::OnMouseEvent(wxMouseEvent &event) // Обработчик на събития от мишката.
{                                                          // Начало на тялото на метода OnMouseEvent.
    event.Skip();                                          // Позволява на събитието да бъде обработено и от други обработчики (важно за wxWidgets).
    FocusOnTerminal();                                     // Фокусира върху терминала при събитие от мишката.
} // Край на тялото на метода OnMouseEvent.
  // Празен ред.
void LinuxTerminalPanel::OnKeyEvent(wxKeyEvent &event) // Обработчик на събития от клавиатурата (натиснат клавиш).
{                                                      // Начало на тялото на метода OnKeyEvent.
    event.Skip();                                      // Позволява на събитието да бъде обработено и от други обработчики (важно за wxWidgets).
    FocusOnTerminal();                                 // Фокусира върху терминала при натискане на клавиш.
} // Край на тялото на метода OnKeyEvent.
#endif // __WXGTK__ // Край на условната компилация за __WXGTK__.