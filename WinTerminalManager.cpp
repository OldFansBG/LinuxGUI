#include "WinTerminalManager.h" // Включва заглавния файл за WinTerminalManager

wxBEGIN_EVENT_TABLE(WinTerminalManager, wxEvtHandler) // Начало на дефиницията на таблицата със събития за WinTerminalManager, наследяващ wxEvtHandler
    EVT_SIZE(WinTerminalManager::OnParentResize)      // Декларира, че събитието за промяна на размера (EVT_SIZE) ще бъде обработено от метода OnParentResize
    wxEND_EVENT_TABLE()                               // Край на дефиницията на таблицата със събития

    WinTerminalManager::WinTerminalManager(wxPanel *parentPanel) // Конструктор на класа WinTerminalManager, приема указател към wxPanel като родителски панел
    : m_parentPanel(parentPanel), m_hwndCmd(nullptr)             // Инициализира член-променливата m_parentPanel с подадения панел и m_hwndCmd с nullptr
{                                                                // Начало на тялото на конструктора
    memset(&m_pi, 0, sizeof(PROCESS_INFORMATION));               // Запълва структурата m_pi с нули, за да я инициализира
    if (m_parentPanel)
    {                                                                               // Проверява дали родителският панел е валиден (не е nullptr)
        m_parentPanel->Bind(wxEVT_SIZE, &WinTerminalManager::OnParentResize, this); // Свързва събитието wxEVT_SIZE на родителския панел с метода OnParentResize на текущия обект
    } // Край на проверката за родителския панел
} // Край на тялото на конструктора

WinTerminalManager::~WinTerminalManager()
{                     // Деструктор на класа WinTerminalManager
    CleanupProcess(); // Извиква метода за почистване на процеса
    if (m_parentPanel)
    {                                                                                 // Проверява дали родителският панел е валиден
        m_parentPanel->Unbind(wxEVT_SIZE, &WinTerminalManager::OnParentResize, this); // Премахва свързването на събитието wxEVT_SIZE с метода OnParentResize
    } // Край на проверката за родителския панел
} // Край на тялото на деструктора

bool WinTerminalManager::Initialize()
{                                                        // Метод за инициализация на терминала, връща true при успех, false при неуспех
    std::wstring cmd = L"cmd.exe /k title EMBEDDED_CMD"; // Дефинира командата, която ще бъде изпълнена (стартиране на cmd.exe с определено заглавие)
    STARTUPINFOW si = {sizeof(si)};                      // Инициализира структурата STARTUPINFOW, задавайки нейния размер
    si.dwFlags = STARTF_USESHOWWINDOW;                   // Задава флаг, указващ, че ще се използва wShowWindow
    si.wShowWindow = SW_HIDE;                            // Задава прозорецът да бъде скрит при стартиране

    if (CreateProcessW(NULL, const_cast<wchar_t *>(cmd.c_str()), // Опитва се да създаде нов процес
                       NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &m_pi))
    { // Параметри за CreateProcessW: без име на приложение, команден ред, без атрибути за сигурност на процеса/нишката, без наследяване на манипулатори, флаг за създаване на нова конзола, без променливи на средата, без текуща директория, указател към STARTUPINFO, указател към PROCESS_INFORMATION

        return FindAndEmbedTerminal(); // Ако процесът е създаден успешно, опитва се да намери и вгради терминала
    } // Край на проверката за успешно създаване на процес
    return false; // Връща false, ако създаването на процес е неуспешно
} // Край на метода Initialize

bool WinTerminalManager::FindAndEmbedTerminal()
{                     // Метод за намиране и вграждане на терминалния прозорец, връща true при успех
    int retries = 20; // Задава брой опити за намиране на прозореца (увеличено за по-бавни системи)
    while (retries-- > 0)
    {                                                                    // Цикъл, който се изпълнява, докато има останали опити
        m_hwndCmd = FindWindowW(L"ConsoleWindowClass", L"EMBEDDED_CMD"); // Опитва се да намери прозорец по клас име ("ConsoleWindowClass") и заглавие ("EMBEDDED_CMD")
        if (m_hwndCmd)
            break;  // Ако прозорецът е намерен, излиза от цикъла
        Sleep(150); // Изчаква 150 милисекунди преди следващия опит (увеличено време за изчакване)
    } // Край на цикъла за търсене

    if (!m_hwndCmd)
        return false; // Ако прозорецът не е намерен след всички опити, връща false

    // Ensure parent window is valid // Уверява се, че родителският прозорец е валиден
    if (!::IsWindow((HWND)m_parentPanel->GetHWND()))
    {                 // Проверява дали манипулаторът на родителския панел е валиден прозорец
        return false; // Връща false, ако родителският прозорец не е валиден
    } // Край на проверката за валидност на родителския прозорец

    // Modify window styles // Променя стиловете на прозореца
    LONG_PTR style = GetWindowLongPtrW(m_hwndCmd, GWL_STYLE);                                         // Взема текущите стилове на прозореца на командния ред
    style &= ~(WS_POPUP | WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU); // Премахва стиловете за изскачащ прозорец, заглавна лента, дебела рамка, бутони за минимизиране/максимизиране и системно меню
    style |= WS_CHILD;                                                                                // Добавя стил WS_CHILD, за да направи прозореца дъщерен
    SetWindowLongPtrW(m_hwndCmd, GWL_STYLE, style);                                                   // Задава новите стилове на прозореца

    // Reparent and resize // Сменя родителя и преоразмерява
    ::SetParent(m_hwndCmd, (HWND)m_parentPanel->GetHWND()); // Задава родителския панел като нов родител на прозореца на командния ред
    UpdateTerminalSize();                                   // Извиква метода за актуализиране на размера на терминала

    // Force redraw // Принудително прерисуване
    ::ShowWindow(m_hwndCmd, SW_SHOW); // Показва прозореца на командния ред
    ::UpdateWindow(m_hwndCmd);        // Актуализира (прерисува) прозореца на командния ред
    return true;                      // Връща true, указвайки успешно вграждане
} // Край на метода FindAndEmbedTerminal

void WinTerminalManager::UpdateTerminalSize()
{ // Метод за актуализиране на размера на терминалния прозорец
    if (m_hwndCmd && m_parentPanel)
    {                                                              // Проверява дали манипулаторът на командния ред и родителският панел са валидни
        ::SetWindowPos(m_hwndCmd, NULL, 0, 0,                      // Позиционира и преоразмерява прозореца на командния ред
                       m_parentPanel->GetClientSize().GetWidth(),  // Задава новата ширина да е равна на ширината на клиентската област на родителския панел
                       m_parentPanel->GetClientSize().GetHeight(), // Задава новата височина да е равна на височината на клиентската област на родителския панел
                       SWP_NOZORDER | SWP_FRAMECHANGED);           // Флагове: не променяй Z-реда и изпрати WM_NCCALCSIZE (рамката се е променила)
    } // Край на проверката за валидност
} // Край на метода UpdateTerminalSize

void WinTerminalManager::OnParentResize(wxSizeEvent &event)
{                         // Метод, който се извиква при промяна на размера на родителския панел
    UpdateTerminalSize(); // Актуализира размера на терминала
    event.Skip();         // Позволява на други обработчици на събития да обработят това събитие
} // Край на метода OnParentResize

void WinTerminalManager::CleanupProcess()
{ // Метод за почистване на създадения процес
    if (m_pi.hProcess)
    { // Проверява дали има валиден манипулатор на процес
        // Gracefully terminate the process // Прекратява процеса "грациозно" (всъщност TerminateProcess е принудително)
        TerminateProcess(m_pi.hProcess, 0); // Прекратява процеса, асоцииран с m_pi.hProcess
        CloseHandle(m_pi.hProcess);         // Затваря манипулатора на процеса
        CloseHandle(m_pi.hThread);          // Затваря манипулатора на главната нишка на процеса
    } // Край на проверката за валиден манипулатор на процес
} // Край на метода CleanupProcess

// New method to send commands to the terminal // Нов метод за изпращане на команди към терминала
bool WinTerminalManager::SendCommand(const std::wstring &command)
{ // Метод за изпращане на команда към терминала, приема командата като std::wstring
    if (!m_hwndCmd)
        return false; // Ако няма валиден манипулатор на прозореца на командния ред, връща false

    // Simulate typing the command into the terminal // Симулира въвеждане на командата в терминала
    for (wchar_t c : command)
    {                                             // Итерира през всеки символ от командата
        ::PostMessageW(m_hwndCmd, WM_CHAR, c, 0); // Изпраща съобщение WM_CHAR (символ) към прозореца на командния ред
    } // Край на цикъла за символите

    // Simulate pressing Enter // Симулира натискане на Enter
    ::PostMessageW(m_hwndCmd, WM_KEYDOWN, VK_RETURN, 0); // Изпраща съобщение WM_KEYDOWN (натиснат клавиш) за клавиша Enter
    ::PostMessageW(m_hwndCmd, WM_KEYUP, VK_RETURN, 0);   // Изпраща съобщение WM_KEYUP (пуснат клавиш) за клавиша Enter

    return true; // Връща true, указвайки успешно изпращане на командата
} // Край на метода SendCommand