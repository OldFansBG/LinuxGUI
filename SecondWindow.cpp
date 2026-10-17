#include "SecondWindow.h" // Включва заглавния файл за класа SecondWindow
#include "DesktopTab.h"   // Включва заглавния файл за класа DesktopTab
#include "MongoDBPanel.h" // <<< Включва отделения header за MongoDB панела
#include "WindowIDs.h"    // <<< Уверете се, че съдържа ID_MONGODB_PANEL_CLOSE
#include "CustomEvents.h" // <<< Включва за FILE_COPY_COMPLETE_EVENT и др.
#include <wx/utils.h>     // Включва помощни функции на wxWidgets
#include <wx/statline.h>  // Включва контрола wxStaticLine за статични линии
#include <wx/process.h>   // Включва wxProcess за управление на външни процеси
#include <wx/dcbuffer.h>  // Включва wxDCBuffer за буферирано рисуване, предотвратява трептене
#include <wx/filename.h>  // Включва wxFileName за работа с имена на файлове и пътища
#include <wx/file.h>      // Включва wxFile за основни файлови операции
#include <wx/arrstr.h>    // Включва wxArrayString за масиви от низове
#include <wx/log.h>       // Включва wxLog за системата за логиране на wxWidgets
#include <cmath>          // За M_PI, fmod, cos, sin в OverlayFrame // Включва стандартната C++ математическа библиотека
#include <chrono>         // За std::chrono (използва се индиректно) // Включва стандартната C++ библиотека за работа с време
#include <iomanip>        // За std::put_time (използва се индиректно) // Включва стандартната C++ библиотека за манипулация на входно-изходни потоци
#include <fstream>        // За std::ifstream (използва се индиректно) // Включва стандартната C++ библиотека за работа с файлови потоци
#include <sstream>        // За std::stringstream (използва се индиректно) // Включва стандартната C++ библиотека за работа с низови потоци
#include <string>         // За std::string // Включва стандартната C++ библиотека за низове
#include <wx/graphics.h>  // За wxGraphicsContext в OverlayFrame // Включва wxGraphicsContext за разширени възможности за 2D рисуване
#include "ContainerCommand.h" // Помощни функции за команди в Docker контейнера
#include <wx/weakref.h>        // wxWeakRef за безопасни указатели към прозорци в асинхронни callback-и

#ifdef __WXMSW__                   // Начало на условна компилация за Microsoft Windows (MSW) среди
#include <windows.h>               // Включва основния заглавен файл на Windows API
#include <dwmapi.h>                // Включва Desktop Window Manager (DWM) API за разширени ефекти на прозорците
#pragma comment(lib, "dwmapi.lib") // Директива към линкера да свърже с библиотеката dwmapi.lib
// Дефинира, ако не е налично във вашия SDK
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE    // Проверява дали макросът DWMWA_USE_IMMERSIVE_DARK_MODE не е дефиниран
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20 // Дефинира макроса DWMWA_USE_IMMERSIVE_DARK_MODE със стойност 20 (използва се за активиране на тъмен режим за рамката на прозореца)
#endif                                   // Край на условната директива #ifndef
#endif                                   // Край на условната компилация #ifdef __WXMSW__

wxIMPLEMENT_CLASS(SecondWindow, wxFrame); // RTTI за SecondWindow (нужно за wxDynamicCast)

// Име на готовия ISO файл, който create_iso.sh записва в /root/custom_iso
static const char *const OUTPUT_ISO_NAME = "custom_linux.iso";

//---------------------------------------------------------------------
// Клас PythonProcess: Управлява жизнения цикъл на бекенд Python процеса и действията след приключването му
class PythonProcess : public wxProcess                                           // Дефиниция на клас PythonProcess, наследяващ wxProcess за управление на външни процеси
{                                                                                // Начало на тялото на класа PythonProcess
public:                                                                          // Декларира публичния интерфейс на класа
    PythonProcess(SecondWindow *parent) : wxProcess(parent), m_parent(parent) {} // Конструктор, приема указател към SecondWindow като родител и го предава на базовия клас wxProcess, инициализира m_parent

    virtual void OnTerminate(int pid, int status) override                                                      // Виртуален метод, извиква се автоматично, когато управляваният процес завърши. Пренаписва метода от wxProcess.
    {                                                                                                           // Начало на тялото на метода OnTerminate
        if (m_parent)                                                                                           // Проверява дали указателят към родителския прозорец (m_parent) е валиден
        {                                                                                                       // Начало на блока if (m_parent)
            m_parent->CloseOverlay();                                                                           // Извиква метод на родителския прозорец за затваряне на наслагващия се екран (overlay)
            if (status != 0)                                                                                    // script.exe връща ненулев код, ако подготовката на контейнера е неуспешна
            {
                wxLogError("Backend setup failed with exit code %d", status);
                wxMessageBox(wxString::Format("Preparing the Docker container failed (exit code %d).\n\n"
                                              "See the newest file in the logs folder next to LinuxISOPro.exe for details.",
                                              status),
                             "Setup Failed", wxOK | wxICON_ERROR, m_parent);
                delete this; // Обектът е създаден с new в StartPythonExecutable
                return;
            }
            wxString containerIdPath = wxFileName(m_parent->GetProjectDir(), "container_id.txt").GetFullPath(); // Създава пълен път до файла "container_id.txt" в проектната директория
            wxFile file;                                                                                        // Декларира обект wxFile за работа с файлове
            if (file.Open(containerIdPath))                                                                     // Опитва да отвори файла "container_id.txt" за четене
            {                                                                                                   // Начало на блока if (file.Open)
                wxString containerId;                                                                           // Декларира низ за съхранение на ID на контейнера
                file.ReadAll(&containerId);                                                                     // Чете цялото съдържание на файла в променливата containerId
                containerId.Trim();                                                                             // Премахва водещи и крайни празни символи от прочетеното ID

                // Обновява SQLTab, ако съществува
                if (m_parent->GetSQLTab())                                      // Проверява дали родителският прозорец има валиден указател към SQLTab
                {                                                               // Начало на блока if (m_parent->GetSQLTab())
                    m_parent->GetSQLTab()->SetContainerId(containerId);         // Задава прочетеното ID на контейнера на SQLTab
                    wxLogDebug("Обновен SQLTab container ID: %s", containerId); // Записва в лога съобщение за успешно обновяване
                } // Край на блока if (m_parent->GetSQLTab())

                // Открива GUI средата
                wxString guiDetectCommand = wxString::Format("docker exec %s cat /root/custom_iso/detected_gui.txt", containerId); // Формира команда за изпълнение в Docker контейнер за четене на файла detected_gui.txt
                wxArrayString output, errors;                                                                                      // Декларира масиви от низове за съхранение на стандартния изход и грешките от командата
                long exitCode = wxExecute(guiDetectCommand, output, errors, wxEXEC_SYNC | wxEXEC_HIDE_CONSOLE);                    // Изпълнява командата синхронно, като скрива конзолния прозорец (wxExecute връща long)

                if (exitCode == 0 && !output.IsEmpty()) // Проверява дали командата е изпълнена успешно (exitCode == 0) и има изходни данни
                {                                       // Начало на блока if (exitCode == 0 && !output.IsEmpty())
                    wxString guiName = output[0];       // Взема първия ред от изхода (предполага се, че това е името на GUI)
                    guiName.Trim(true).Trim(false);     // Премахва празни символи от началото и края на името
                    guiName.Replace("\n", "");          // Премахва символите за нов ред от името

                    wxLogDebug("Открита GUI среда преди event: %s", guiName); // Записва в лога откритото име на GUI средата

                    // Запазва името на GUI локално
                    wxString localGuiPath = wxFileName(m_parent->GetProjectDir(), "detected_gui.txt").GetFullPath(); // Формира пълен път за локално запазване на името на GUI
                    wxFile guiFile;                                                                                  // Декларира обект wxFile за запис
                    if (guiFile.Create(localGuiPath, true) && guiFile.IsOpened())                                    // Създава (или презаписва, ако съществува) файла и проверява дали е отворен успешно
                    {                                                                                                // Начало на блока if (guiFile.Create)
                        guiFile.Write(guiName);                                                                      // Записва името на GUI във файла
                        guiFile.Close();                                                                             // Затваря файла
                        wxLogDebug("GUI името е записано във файл: %s", localGuiPath);                               // Записва в лога съобщение за успешно запазване
                    } // Край на блока if (guiFile.Create)

                    // Известява DesktopTab
                    if (m_parent->GetDesktopTab())                                               // Проверява дали родителският прозорец има валиден указател към DesktopTab
                    {                                                                            // Начало на блока if (m_parent->GetDesktopTab())
                        wxCommandEvent *guiEvent = new wxCommandEvent(FILE_COPY_COMPLETE_EVENT); // Създава нов обект на събитие от тип FILE_COPY_COMPLETE_EVENT
                        guiEvent->SetString(guiName);                                            // Задава името на GUI като данни (низ) на събитието
                        wxLogDebug("Изпраща event с GUI име: %s", guiName);                      // Записва в лога информация за изпращалото събитие
                        wxPostEvent(m_parent->GetDesktopTab(), *guiEvent);                       // Изпраща (post-ва) събитието асинхронно към DesktopTab на родителския прозорец
                        delete guiEvent;                                                         // Освобождава паметта, заета от обекта на събитието, след като е изпратено
                    } // Край на блока if (m_parent->GetDesktopTab())
                } // Край на блока if (exitCode == 0 && !output.IsEmpty())
                else                                                                                       // Ако командата за откриване на GUI е неуспешна или няма изход
                {                                                                                          // Начало на блока else
                    wxLogError("Неуспешно откриване на GUI среда или няма изход. ExitCode: %d", exitCode); // Записва грешка в лога
                } // Край на блока else
                // Изпълнява начална команда в терминала, ако е приложимо
                m_parent->ExecuteDockerCommand(containerId); // Извиква метод на родителския прозорец за изпълнение на начална Docker команда с ID-то на контейнера
            } // Край на блока if (file.Open)
            else                                                                               // Ако отварянето на файла container_id.txt е неуспешно
            {                                                                                  // Начало на блока else
                wxLogError("Не може да се отвори container_id.txt файл: %s", containerIdPath); // Записва грешка в лога
            } // Край на блока else
        } // Край на блока if (m_parent)
        delete this; // При пренаписан OnTerminate обектът трябва да се изтрие ръчно
    } // Край на тялото на метода OnTerminate

private:                    // Декларира частната секция на класа
    wxWeakRef<SecondWindow> m_parent; // Слаб указател към родителския прозорец (нулира се, ако прозорецът бъде унищожен)
}; // Край на дефиницията на класа PythonProcess
//---------------------------------------------------------------------

//---------------------------------------------------------------------
// Клас DockerExecProcess: Управлява процеса по създаване и копиране на ISO чрез Docker и обновява потребителския интерфейс
class DockerExecProcess : public wxProcess                                           // Дефинира клас DockerExecProcess, наследяващ wxProcess
{                                                                                    // Начало на тялото на класа DockerExecProcess
public:                                                                              // Декларира публичния интерфейс на класа
    DockerExecProcess(SecondWindow *parent) : wxProcess(parent), m_parent(parent) {} // Конструктор, инициализира базовия клас и член-данната m_parent

    virtual void OnTerminate(int pid, int status) override                                                          // Виртуален метод, извиква се при завършване на процеса
    {                                                                                                               // Начало на тялото на метода OnTerminate
        if (m_parent)                                                                                               // Проверява дали указателят към родителския прозорец е валиден
        {                                                                                                           // Начало на блока if (m_parent)
            if (status == 0)                                                                                        // Проверява дали create_iso.sh е завършил успешно (статус 0)
            {                                                                                                       // Начало на блока if (status == 0)
                wxString containerId = ContainerCommand::ReadContainerId(m_parent->GetProjectDir());               // Чете ID-то на контейнера от container_id.txt
                if (!containerId.IsEmpty())                                                                         // Ако ID-то е прочетено успешно
                {                                                                                                   // Начало на блока if
                    wxString sourcePath = wxString::Format("%s:/root/custom_iso/%s", containerId, OUTPUT_ISO_NAME);       // Изходен път на ISO файла в контейнера
                    wxString destPath = wxFileName(m_parent->GetProjectDir(), OUTPUT_ISO_NAME).GetFullPath();             // ISO файлът се записва в директорията на проекта
                    wxString copyCommand = wxString::Format("docker cp \"%s\" \"%s\"", sourcePath, destPath);             // Формира командата за копиране на файла от Docker контейнера
                    long copyStatus = wxExecute(copyCommand, wxEXEC_SYNC | wxEXEC_HIDE_CONSOLE);                          // Копира ISO файла синхронно (анимацията продължава да се рисува)
                    m_parent->CloseOverlay();                                                                             // Затваря наслагващия екран след копирането

                    if (copyStatus == 0)                                                                                  // Проверява дали копирането е успешно (статус 0)
                    {                                                                                                     // Начало на блока if (copyStatus == 0)
                        wxMessageBox("The ISO was created successfully:\n" + destPath, "ISO Created", wxOK | wxICON_INFORMATION, m_parent); // Показва съобщение за успех
                    } // Край на блока if (copyStatus == 0)
                    else                                                                                                  // Ако копирането е неуспешно
                    {                                                                                                     // Начало на блока else
                        wxMessageBox("The ISO was created in the container, but copying it to the project folder failed.", "Copy Failed", wxOK | wxICON_ERROR, m_parent); // Съобщение за грешка при копирането
                    } // Край на блока else
                } // Край на блока if
                else                                                                                                      // Ако container_id.txt не може да се прочете
                {                                                                                                         // Начало на блока else
                    m_parent->CloseOverlay();                                                                             // Затваря наслагващия екран
                    wxLogError("Could not read container_id.txt while copying the ISO.");                                 // Записва грешка в лога
                    wxMessageBox("The ISO was created, but the container ID could not be read to copy it.", "Copy Failed", wxOK | wxICON_ERROR, m_parent); // Съобщение за грешка
                } // Край на блока else
            } // Край на блока if (status == 0)
            else                                                                                   // Ако процесът по създаване на ISO е неуспешен
            {                                                                                      // Начало на блока else
                m_parent->CloseOverlay();                                                          // Затваря наслагващия екран
                wxLogError("create_iso.sh failed with exit code %d", status);                      // Записва грешка в лога със статус кода
                wxMessageBox(wxString::Format("Creating the ISO failed (exit code %d).\n\n"
                                              "Run \"docker exec <container> /bin/bash /create_iso.sh\" in a terminal to see the full output.",
                                              status),
                             "ISO Creation Failed", wxOK | wxICON_ERROR, m_parent);                // Показва съобщение за грешка на потребителя
            } // Край на блока else
            // Активира бутона 'Create ISO' независимо от успех/грешка
            wxButton *nextButton = wxDynamicCast(m_parent->FindWindow(ID_NEXT_BUTTON), wxButton); // Намира бутона чрез неговото ID и го преобразува към wxButton указател
            if (nextButton)                                                                       // Проверява дали бутонът е намерен успешно
                nextButton->Enable();                                                             // Активира бутона
        } // Край на блока if (m_parent)
        delete this; // При пренаписан OnTerminate обектът трябва да се изтрие ръчно
    } // Край на тялото на метода OnTerminate

private:                    // Декларира частната секция на класа
    wxWeakRef<SecondWindow> m_parent; // Слаб указател към родителския прозорец (нулира се, ако прозорецът бъде унищожен)
}; // Край на дефиницията на класа DockerExecProcess
//---------------------------------------------------------------------

//---------------------------------------------------------------------
// Клас OverlayFrame: Показва полупрозрачен overlay с анимация за зареждане
//---------------------------------------------------------------------
// OverlayFrame class (Копирано от СТАРАТА работеща версия)
class OverlayFrame : public wxDialog                                                                                                         // Дефиниция на клас OverlayFrame, наследяващ wxDialog
{                                                                                                                                            // Начало на тялото на класа OverlayFrame
public:                                                                                                                                      // Декларира публичния интерфейс на класа
    OverlayFrame(wxWindow *parent) : wxDialog(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxNO_BORDER | wxFRAME_FLOAT_ON_PARENT) // Конструктор, инициализира wxDialog с параметри и стилове
    {                                                                                                                                        // Начало на тялото на конструктора
        SetBackgroundColour(wxColour(0, 0, 0));                                                                                              // Задава черен цвят на фона
        SetTransparent(200);                                                                                                                 // Задава степен на прозрачност (0-255, 200 е доста прозрачно)
        SetBackgroundStyle(wxBG_STYLE_PAINT);                                                                                                // Задава стил на фона, за да се избегне трептене при прерисуване

        m_parentWindow = parent; // Запазва указателя към родителския прозорец
        UpdatePositionAndSize(); // Задава начална позиция/размер спрямо родителя

        m_animationAngle = 0.0; // Инициализира ъгъла на анимацията

        Bind(wxEVT_PAINT, &OverlayFrame::OnPaint, this); // Свързва събитието за рисуване с метода OnPaint
        Bind(wxEVT_TIMER, &OverlayFrame::OnTimer, this); // Свързва събитието от таймера с метода OnTimer

        // Свързва събитията за преместване/оразмеряване на родителя, за да държи overlay-а подравнен
        if (parent)                                                              // Проверява дали родителският прозорец е валиден
        {                                                                        // Начало на блока if (parent)
            parent->Bind(wxEVT_MOVE, &OverlayFrame::OnParentMoveOrResize, this); // Свързва събитието за преместване на родителя
            parent->Bind(wxEVT_SIZE, &OverlayFrame::OnParentMoveOrResize, this); // Свързва събитието за оразмеряване на родителя
        } // Край на блока if (parent)

        m_timer.SetOwner(this); // Асоциира таймера с този прозорец (OverlayFrame)
        m_timer.Start(30);      // Стартира таймера с интервал 30 милисекунди (за честота на анимацията)
        Show(true);             // Показва наслагващия се прозорец
    } // Край на тялото на конструктора

    ~OverlayFrame() // Деструктор на класа OverlayFrame
    {               // Начало на тялото на деструктора
        // Разкача събитията при унищожаване на overlay-а
        if (m_parentWindow)                                                                // Проверява дали указателят към родителския прозорец е валиден
        {                                                                                  // Начало на блока if (m_parentWindow)
            m_parentWindow->Unbind(wxEVT_MOVE, &OverlayFrame::OnParentMoveOrResize, this); // Разкача събитието за преместване на родителя
            m_parentWindow->Unbind(wxEVT_SIZE, &OverlayFrame::OnParentMoveOrResize, this); // Разкача събитието за оразмеряване на родителя
        } // Край на блока if (m_parentWindow)
        m_timer.Stop(); // Спира таймера за анимация
    } // Край на тялото на деструктора

private:                                                         // Декларира частната секция на класа
    void UpdatePositionAndSize()                                 // Метод за обновяване на позицията и размера на наслагващия екран
    {                                                            // Начало на тялото на метода UpdatePositionAndSize
        if (m_parentWindow)                                      // Проверява дали указателят към родителския прозорец е валиден
        {                                                        // Начало на блока if (m_parentWindow)
            wxRect clientRect = m_parentWindow->GetClientRect(); // Взема клиентската област на родителския прозорец
            // Проверява дали размерът е валиден преди да продължи
            if (clientRect.GetWidth() > 0 && clientRect.GetHeight() > 0)              // Проверява дали ширината и височината са по-големи от 0
            {                                                                         // Начало на блока if (валиден размер)
                SetPosition(m_parentWindow->ClientToScreen(clientRect.GetTopLeft())); // Задава позицията на наслагващия екран спрямо екрана
                SetSize(clientRect.GetSize());                                        // Задава размера на наслагващия екран да съответства на клиентската област на родителя
            } // Край на блока if (валиден размер)
            else // Ако размерът не е валиден
            {    // Начало на блока else
                // По избор: скрива overlay-а ако размерът е невалиден
                Hide(); // Скрива наслагващия екран
            } // Край на блока else
        } // Край на блока if (m_parentWindow)
    } // Край на тялото на метода UpdatePositionAndSize

    void OnParentMoveOrResize(wxEvent &event) // Метод, извиква се при преместване или оразмеряване на родителския прозорец
    {                                         // Начало на тялото на метода OnParentMoveOrResize
        UpdatePositionAndSize();              // Обновява позицията и размера на наслагващия екран
        event.Skip();                         // Позволява на други обработващи да обработят събитието
    } // Край на тялото на метода OnParentMoveOrResize

    void OnPaint(wxPaintEvent &event)   // Метод, извиква се при необходимост от прерисуване на прозореца
    {                                   // Начало на тялото на метода OnPaint
        wxAutoBufferedPaintDC dc(this); // Създава буфериран контекст за рисуване (DC) за избягване на трептене
        dc.Clear();                     // Изчиства фона на прозореца

        wxSize size = GetClientSize(); // Взема текущия размер на клиентската област на прозореца
        // Рисува полупрозрачен фон
        dc.SetBrush(wxBrush(wxColour(0, 0, 0, 230)));              // Настройва четката с черен цвят и алфа канал 230 (полупрозрачност)
        dc.SetPen(*wxTRANSPARENT_PEN);                             // Задава прозрачно перо (без контур)
        dc.DrawRectangle(0, 0, size.GetWidth(), size.GetHeight()); // Рисува правоъгълник, покриващ целия прозорец

        // Рисува анимацията за зареждане центрирано
        DrawLoadingAnimation(dc, size); // Извиква метода за рисуване на анимацията
    } // Край на тялото на метода OnPaint

    // *** Използва старата логика за DrawLoadingAnimation ***
    void DrawLoadingAnimation(wxDC &dc, const wxSize &size)           // Метод за рисуване на анимацията за зареждане
    {                                                                 // Начало на тялото на метода DrawLoadingAnimation
        wxPoint center(size.GetWidth() / 2, size.GetHeight() / 2);    // Изчислява централната точка на прозореца
        int radius = std::min(size.GetWidth(), size.GetHeight()) / 6; // Оригинално изчисление на радиуса на анимационния кръг
        int numPoints = 8;                                            // Брой на точките в анимацията
        int pointRadius = radius / 4;                                 // Оригинално изчисление на радиуса на всяка точка

        // Използва непрозрачен pen и brush за плътни точки
        dc.SetBrush(*wxWHITE_BRUSH); // Задава бяла четка за запълване на точките
        dc.SetPen(*wxWHITE_PEN);     // Оригинален pen (бяло перо за контура на точките)

        for (int i = 0; i < numPoints; ++i)                                 // Цикъл за рисуване на всяка точка
        {                                                                   // Начало на цикъла for
            double angle = m_animationAngle + (2.0 * M_PI * i / numPoints); // Изчислява ъгъла за текущата точка
            int x = center.x + static_cast<int>(radius * cos(angle));       // Изчислява x-координатата на точката
            int y = center.y + static_cast<int>(radius * sin(angle));       // Изчислява y-координатата на точката
            dc.DrawCircle(x, y, pointRadius);                               // Оригинално извикване за рисуване на кръг (точка)
        } // Край на цикъла for
    } // Край на тялото на метода DrawLoadingAnimation

    // *** Използва старата логика за OnTimer ***
    void OnTimer(wxTimerEvent &event)       // Метод, извиква се при всяко събитие от таймера
    {                                       // Начало на тялото на метода OnTimer
        m_animationAngle += 0.1;            // Оригинална скорост (увеличава ъгъла на анимацията)
        if (m_animationAngle > 2.0 * M_PI)  // Проверява дали ъгълът е надхвърлил 2*PI (пълен кръг)
            m_animationAngle -= 2.0 * M_PI; // Нулира ъгъла, като изважда 2*PI
        Refresh();                          // Оригинално Refresh извикване (изисква прерисуване на прозореца)
    } // Край на тялото на метода OnTimer

    double m_animationAngle;  // Променлива за съхранение на текущия ъгъл на анимацията
    wxTimer m_timer;          // Обект таймер за управление на анимацията
    wxWindow *m_parentWindow; // Указател към прозореца, който покрива overlay-а
}; // Край на дефиницията на класа OverlayFrame
//---------------------------------------------------------------------

// Таблица с event-и за SecondWindow: Свързва събитията с техните обработващи функции
wxBEGIN_EVENT_TABLE(SecondWindow, wxFrame)                                // Начало на дефиницията на таблицата със събития за SecondWindow
    EVT_CLOSE(SecondWindow::OnClose)                                      // Свързва събитието за затваряне на прозореца (EVT_CLOSE) с метода SecondWindow::OnClose
    EVT_BUTTON(ID_TERMINAL_TAB, SecondWindow::OnTabChanged)               // Свързва натискането на бутона ID_TERMINAL_TAB с метода SecondWindow::OnTabChanged
    EVT_BUTTON(ID_SQL_TAB, SecondWindow::OnTabChanged)                    // Свързва натискането на бутона ID_SQL_TAB с метода SecondWindow::OnTabChanged
    EVT_BUTTON(ID_MONGODB_BUTTON, SecondWindow::OnMongoDBButton)          // Обработва натискането на главния MongoDB бутон (свързва с SecondWindow::OnMongoDBButton)
    EVT_BUTTON(ID_MONGODB_PANEL_CLOSE, SecondWindow::OnMongoDBPanelClose) // Обработва заявка за затваряне от панела (свързва с SecondWindow::OnMongoDBPanelClose)
    EVT_BUTTON(ID_NEXT_BUTTON, SecondWindow::OnNext)                      // Свързва натискането на бутона ID_NEXT_BUTTON с метода SecondWindow::OnNext
    wxEND_EVENT_TABLE()                                                   // Край на дефиницията на таблицата със събития


// --- Имплементация на SecondWindow ---

// Конструктор: Инициализира SecondWindow, създава UI и стартира бекенд процеса
SecondWindow::SecondWindow(wxWindow *parent,                                       // Параметър: указател към родителския прозорец
                           const wxString &title,                                  // Параметър: заглавие на прозореца
                           const wxString &isoPath,                                // Параметър: път до ISO файла
                           const wxString &projectDir,                             // Параметър: път до проектната директория
                           DesktopTab *desktopTab)                                 // Запазва параметъра DesktopTab // Параметър: указател към DesktopTab
    : wxFrame(parent, wxID_ANY, title,                                             // Инициализация на базовия клас wxFrame
              wxDefaultPosition, wxSize(800, 650),                                 // Коригиран размер по подразбиране (позиция и размер)
              wxDEFAULT_FRAME_STYLE | wxSYSTEM_MENU | wxCAPTION |                  // Стилове на рамката
                  wxCLOSE_BOX | wxCLIP_CHILDREN | wxMINIMIZE_BOX |                 // Допълнителни стилове на рамката
                  wxMAXIMIZE_BOX | wxRESIZE_BORDER),                               // Още стилове на рамката
      m_isoPath(isoPath),                                                          // Инициализация на член-променливата m_isoPath
      m_projectDir(projectDir),                                                    // Инициализация на член-променливата m_projectDir
      m_threadRunning(false),                                                      // Инициализация на член-променливата m_threadRunning на false
      m_overlay(nullptr),                                                          // Инициализация на член-променливата m_overlay на nullptr
      m_desktopTab(desktopTab),                                                    // Запазва предадения DesktopTab указател // Инициализация на m_desktopTab
      m_terminalTab(nullptr),                                                      // Инициализира указателите // Инициализация на m_terminalTab на nullptr
      m_sqlTab(nullptr),                                                           // Инициализация на m_sqlTab на nullptr
      m_terminalPanel(nullptr),                                                    // Инициализация на m_terminalPanel на nullptr
      m_flatpakStore(nullptr),                                                     // Инициализира указателя FlatpakStore // Инициализация на m_flatpakStore на nullptr
      m_mongoPanel(nullptr),                                                       // Инициализация на m_mongoPanel на nullptr
      m_isClosing(false),                                                          // Инициализация на m_isClosing на false
      m_lastTab(nullptr)                                                         // Инициализация на m_lastTab на nullptr
#ifdef __WXMSW__                                                                   // Условна компилация за Windows
      ,                                                                            // Запетая за следващия инициализатор в списъка (само за Windows)
      m_winTerminalManager(nullptr)                                                // Инициализира указателя WinTerminalManager // Инициализация на m_winTerminalManager на nullptr
#endif                                                                             // Край на условната компилация за Windows
{                                                                                  // Начало на тялото на конструктора на SecondWindow
    wxLogDebug("SecondWindow constructor: desktopTab pointer = %p", m_desktopTab); // Записва в лога указателя към desktopTab

#ifdef __WXMSW__ // Условна компилация за Windows
    // Прилага подсказки за тъмен режим за заглавната лента/рамките на Windows, ако е приложимо
    BOOL value = TRUE;                                                                          // Декларира и инициализира променлива от тип BOOL на TRUE
    ::DwmSetWindowAttribute(GetHandle(), DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value)); // Извиква Windows API функция за задаване на атрибут на прозореца (тъмен режим)
#endif                                                                                          // Край на условната компилация за Windows

    // Опит за незабавно получаване на ID на контейнера, ако е налично (може да е празно първоначално)
    m_containerId = ContainerManager::Get().GetCurrentContainerId();                // Взема текущото ID на контейнера от ContainerManager
    wxLogDebug("SecondWindow constructor initial container ID: %s", m_containerId); // Записва в лога първоначалното ID на контейнера

    CreateControls();                          // Създава всички UI елементи (извиква метод за създаване на контролите)
    Centre();                                  // Центрира прозореца на екрана
    SetBackgroundColour(wxColour(40, 44, 52)); // Задава основен цвят на фона на прозореца
    m_lastTab = m_terminalTab;                 // Задава последния видим таб първоначално (на m_terminalTab)
    Show(true);                                // Показва прозореца

    // Показва overlay, докато Python скриптът работи
    m_overlay = new OverlayFrame(this); // Създава нов обект OverlayFrame

    // Стартира Python процеса на бекенда
    StartPythonExecutable(); // Извиква метод за стартиране на Python изпълнимия файл

    // Инициализира Windows Terminal, ако е на Windows
#ifdef __WXMSW__              // Условна компилация за Windows
    if (m_winTerminalManager) // Проверява дали WinTerminalManager е инициализиран
    {                         // Начало на блока if (m_winTerminalManager)
                              // Команда след инициализация, ако е необходима, след като Python скриптът предостави ID на контейнера
                              // Това може да се наложи да бъде отложено до OnTerminate на PythonProcess
                              // m_winTerminalManager->SendCommand(L"echo Terminal Ready\r\n"); // Примерна команда за изпращане (коментар)
    } // Край на блока if (m_winTerminalManager)
#endif // Край на условната компилация за Windows
} // Край на тялото на конструктора на SecondWindow

SecondWindow::~SecondWindow()                          // Деструктор на класа SecondWindow
{                                                      // Начало на тялото на деструктора
    wxLogDebug("SecondWindow::~SecondWindow - START"); // Записва в лога началото на изпълнение на деструктора

    // Уверява се, че Python бекенд нишката/процесът е сигнализиран да спре, ако е приложимо
    CleanupThread(); // (Тази функция може да се нуждае от имплементация в зависимост от управлението на Python процеса) // Извиква метод за почистване на нишката

#ifdef __WXMSW__ // Условна компилация за Windows
    // Почиства Windows Terminal Manager
    delete m_winTerminalManager;                                              // Изтрива обекта WinTerminalManager
    m_winTerminalManager = nullptr;                                           // Занулява указателя към WinTerminalManager
    wxLogDebug("SecondWindow::~SecondWindow - m_winTerminalManager deleted"); // Записва в лога, че WinTerminalManager е изтрит
#endif                                                                        // Край на условната компилация за Windows

    wxLogDebug("SecondWindow::~SecondWindow - END"); // Записва в лога края на изпълнение на деструктора
} // Край на тялото на деструктора

// Затваря и унищожава overlay-а за зареждане, ако съществува
void SecondWindow::CloseOverlay()                    // Метод за затваряне на наслагващия екран
{                                                    // Начало на тялото на метода CloseOverlay
    if (m_overlay)                                   // Проверява дали наслагващият екран съществува
    {                                                // Начало на блока if (m_overlay)
        m_overlay->Hide();                           // Първо го скрива
        m_overlay->Destroy();                        // След това го унищожава
        m_overlay = nullptr;                         // Занулява указателя към наслагващия екран
        wxLogDebug("Overlay closed and destroyed."); // Записва в лога, че наслагващият екран е затворен и унищожен
    } // Край на блока if (m_overlay)
} // Край на тялото на метода CloseOverlay

// Стартира бекенд Python изпълнимия файл (script.exe) и показва overlay
void SecondWindow::StartPythonExecutable() // Метод за стартиране на Python изпълнимия файл
{                                          // Начало на тялото на метода StartPythonExecutable
    wxString pythonExePath = "script.exe"; // Уверете се, че това е в PATH или предоставете пълен път // Задава името на Python изпълнимия файл

    // Проверява дали изпълнимият файл съществува
    if (!wxFileName::FileExists(pythonExePath))                                                                            // Проверява дали файлът съществува на указания път
    {                                                                                                                      // Начало на блока if (!wxFileName::FileExists)
        pythonExePath = wxFileName::GetCwd() + wxFILE_SEP_PATH + "script.exe";                                             // Опитва текущата директория // Формира път до файла в текущата работна директория
        if (!wxFileName::FileExists(pythonExePath))                                                                        // Проверява отново дали файлът съществува
        {                                                                                                                  // Начало на блока if (!wxFileName::FileExists) - втори опит
            wxMessageBox("Error: script.exe not found!\nPlease ensure it is in the application directory or system PATH.", // Показва съобщение за грешка
                         "Executable Not Found", wxOK | wxICON_ERROR);                                                     // Заглавие и стил на съобщението за грешка
            CloseOverlay();                                                                                                // Затваря overlay-а, ако не можем да стартираме // Извиква метод за затваряне на наслагващия екран
            // По избор затваря прозореца или деактивира функционалността
            // GetParent()->Close(); // Пример: Затваря родителя (MainFrame) // Коментиран ред за затваряне на родителския прозорец
            return; // Прекратява изпълнението на метода
        } // Край на блока if (!wxFileName::FileExists) - втори опит
    } // Край на блока if (!wxFileName::FileExists)

    // Уверява се, че пътищата на проекта и ISO са валидни, преди да продължи
    if (m_projectDir.IsEmpty() || !wxDirExists(m_projectDir))                                         // Проверява дали пътят до проектната директория е празен или директорията не съществува
    {                                                                                                 // Начало на блока if (невалидна проектна директория)
        wxMessageBox("Error: Project directory is invalid or does not exist.\nPath: " + m_projectDir, // Показва съобщение за грешка
                     "Invalid Project Directory", wxOK | wxICON_ERROR);                               // Заглавие и стил на съобщението за грешка
        CloseOverlay();                                                                               // Извиква метод за затваряне на наслагващия екран
        return;                                                                                       // Прекратява изпълнението на метода
    } // Край на блока if (невалидна проектна директория)
    if (m_isoPath.IsEmpty() || !wxFileExists(m_isoPath))                                                // Проверява дали пътят до ISO файла е празен или файлът не съществува
    {                                                                                                   // Начало на блока if (невалиден ISO път)
        wxMessageBox("Error: ISO file path is invalid or the file does not exist.\nPath: " + m_isoPath, // Показва съобщение за грешка
                     "Invalid ISO Path", wxOK | wxICON_ERROR);                                          // Заглавие и стил на съобщението за грешка
        CloseOverlay();                                                                                 // Извиква метод за затваряне на наслагващия екран
        return;                                                                                         // Прекратява изпълнението на метода
    } // Край на блока if (невалиден ISO път)

    wxString command = wxString::Format("\"%s\" --project-dir \"%s\" --iso-path \"%s\"", // Формира командата за изпълнение
                                        pythonExePath, m_projectDir, m_isoPath);         // Аргументи за командата
    wxLogDebug("Executing Python command: %s", command);                                 // Записва в лога командата, която ще бъде изпълнена

    PythonProcess *proc = new PythonProcess(this);                           // Използва нашия персонализиран клас за процеси // Създава нов обект PythonProcess
    long pid = wxExecute(command, wxEXEC_ASYNC | wxEXEC_HIDE_CONSOLE, proc); // Стартира скрито // Изпълнява командата асинхронно и скрива конзолата, запазва PID

    if (pid == 0)                                                                                            // Проверява дали стартирането на процеса е неуспешно (PID е 0)
    {                                                                                                        // Начало на блока if (pid == 0)
        wxLogError("Failed to start Python executable: %s", command);                                        // Записва грешка в лога
        wxMessageBox("Failed to launch the backend process (script.exe)!", "Execution Error", wxICON_ERROR); // Показва съобщение за грешка
        delete proc;                                                                                         // Почиства обекта на процеса // Изтрива обекта на процеса
        CloseOverlay();                                                                                      // Затваря overlay-а за зареждане // Извиква метод за затваряне на наслагващия екран
        m_threadRunning = false;                                                                             // Уверява се, че флагът е нулиран // Задава флага за работеща нишка на false
        // Обмислете деактивиране на по-нататъшни действия или затваряне на прозореца тук
    } // Край на блока if (pid == 0)
    else                                                                     // Ако стартирането на процеса е успешно
    {                                                                        // Начало на блока else
        wxLogDebug("Python executable started successfully, PID: %ld", pid); // Записва в лога успешното стартиране и PID
        m_threadRunning = true;                                              // Показва, че процесът работи // Задава флага за работеща нишка на true
    } // Край на блока else
} // Край на тялото на метода StartPythonExecutable

// Почиства бекенд процеса/нишката, ако работи
void SecondWindow::CleanupThread()                                                                       // Метод за почистване на бекенд нишката/процеса
{                                                                                                        // Начало на тялото на метода CleanupThread
    wxLogDebug("SecondWindow::CleanupThread - START");                                                   // Записва в лога началото на изпълнение на метода
    if (m_threadRunning)                                                                                 // Проверява дали нишката/процесът работи
    {                                                                                                    // Начало на блока if (m_threadRunning)
        wxLogDebug("SecondWindow::CleanupThread - Backend process was running, setting flag to false."); // Записва в лога, че процесът е работил
        // Ако PythonProcess има PID, може да изпратите сигнал тук, ако е необходимо плавно прекратяване
        // напр., if (python_pid > 0) wxKill(python_pid, wxSIGTERM); // Коментиран пример за изпращане на сигнал
        m_threadRunning = false; // Задава флага за работеща нишка на false
    } // Край на блока if (m_threadRunning)
    else                                                                                     // Ако нишката/процесът не работи
    {                                                                                        // Начало на блока else
        wxLogDebug("SecondWindow::CleanupThread - Backend process flag was already false."); // Записва в лога, че флагът вече е бил false
    } // Край на блока else
    wxLogDebug("SecondWindow::CleanupThread - END"); // Записва в лога края на изпълнение на метода
} // Край на тялото на метода CleanupThread

// Изпраща началната Docker exec команда към терминала (Windows или Linux)
void SecondWindow::ExecuteDockerCommand(const wxString &containerId)          // Метод за изпълнение на Docker команда
{                                                                             // Начало на тялото на метода ExecuteDockerCommand
    if (containerId.IsEmpty())                                                // Проверява дали ID-то на контейнера е празно
    {                                                                         // Начало на блока if (containerId.IsEmpty())
        wxLogWarning("ExecuteDockerCommand called with empty container ID."); // Записва предупреждение в лога
        return;                                                               // Прекратява изпълнението на метода
    } // Край на блока if (containerId.IsEmpty())

#ifdef __WXMSW__ // Условна компилация за Windows
    // За Windows Terminal Manager
    if (m_winTerminalManager) // Проверява дали WinTerminalManager е инициализиран
    {                         // Начало на блока if (m_winTerminalManager)
        // Изчиства екрана, изпълнява docker команда, след това влиза в chroot
        // Изпращането на команди, разделени с '\r\n', симулира натискане на Enter след всяка
        // Влиза директно в chroot средата на ISO-то, за да може потребителят да я персонализира (SendCommand натиска Enter)
        wxString command = wxString::Format(L"docker exec -it %s chroot %s /bin/bash", // Команда за влизане в chroot средата в контейнера
                                            containerId, ContainerCommand::CHROOT_PATH); // ID на контейнера и път до chroot
        wxLogDebug("Sending command to Windows Terminal: %s", command); // Записва в лога командата, която се изпраща
        m_winTerminalManager->SendCommand(command.ToStdWstring());      // Изпраща командата към Windows Terminal Manager
    } // Край на блока if (m_winTerminalManager)
#else  // Приема се Linux Terminal Panel за системи, различни от Windows // Условна компилация за системи, различни от Windows
    if (m_terminalPanel) // Проверява дали терминалният панел е инициализиран
    {                    // Начало на блока if (m_terminalPanel)
        // Linux терминалът може да обработи това по различен начин, вероятно чрез първоначалната xterm команда
        // или чрез директно изпращане на вход, ако терминалният уиджет го поддържа.
        // За базов вграден xterm, първоначалната команда обикновено е достатъчна.
        wxLogDebug("Linux terminal detected, initial command should handle container entry."); // Записва в лога, че е засечен Linux терминал
        // Ако се наложи да изпращате *още* команди по-късно, ще ви е необходим механизъм
        // за писане в PTY (псевдотерминала) на xterm, което е по-сложно.
    } // Край на блока if (m_terminalPanel)
#endif // Край на условната компилация
} // Край на тялото на метода ExecuteDockerCommand

// Създава всички UI контроли, панели и layout-и за прозореца
void SecondWindow::CreateControls()                         // Метод за създаване на UI контролите
{                                                           // Начало на тялото на метода CreateControls
    m_mainPanel = new wxPanel(this);                        // Създава нов основен панел
    wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);     // Създава нов вертикален BoxSizer за основния панел
    m_mainPanel->SetBackgroundColour(wxColour(40, 44, 52)); // Постоянен фон // Задава цвят на фона на основния панел

    // --- Горен панел с бутон MongoDB ---
    wxPanel *topPanel = new wxPanel(m_mainPanel);        // Създава нов горен панел, дъщерен на основния панел
    topPanel->SetBackgroundColour(wxColour(20, 20, 20)); // По-тъмен нюанс // Задава цвят на фона на горния панел
    wxBoxSizer *topSizer = new wxBoxSizer(wxHORIZONTAL); // Създава нов хоризонтален BoxSizer за горния панел

    wxButton *mongoButton = new wxButton(topPanel, ID_MONGODB_BUTTON, "MongoDB",             // Създава нов бутон "MongoDB"
                                         wxDefaultPosition, wxSize(100, 30), wxBORDER_NONE); // Позиция, размер и стил на бутона
    // Стилизира бутона (Обмислете използването на ThemeConfig)
    mongoButton->SetBackgroundColour(wxColour(77, 171, 68)); // MongoDB зелено // Задава зелен цвят на фона на бутона
    mongoButton->SetForegroundColour(*wxWHITE);              // Задава бял цвят на текста на бутона
    wxFont topButtonFont = mongoButton->GetFont();           // Взема текущия шрифт на бутона
    topButtonFont.SetWeight(wxFONTWEIGHT_BOLD);              // Задава удебелен шрифт
    mongoButton->SetFont(topButtonFont);                     // Прилага новия шрифт на бутона

    topSizer->AddStretchSpacer(1);                                     // Избутва бутона надясно // Добавя разтегливо празно пространство отляво на бутона
    topSizer->Add(mongoButton, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5); // Добавя бутона към sizer-а с отстъп и вертикално центриране
    topSizer->AddSpacer(10);                                           // Десен отстъп // Добавя фиксирано празно пространство отдясно на бутона
    topPanel->SetSizer(topSizer);                                      // Задава sizer-а на горния панел

    // --- Лента с табове ---
    wxPanel *tabPanel = new wxPanel(m_mainPanel);                // Създава нов панел за лентата с табове
    tabPanel->SetBackgroundColour(wxColour(30, 30, 30));         // Фон на лентата с табове // Задава цвят на фона на панела за табове
    wxBoxSizer *tabSizer = new wxBoxSizer(wxHORIZONTAL);         // Главен sizer за центриране // Създава главен хоризонтален BoxSizer
    wxBoxSizer *centeredTabSizer = new wxBoxSizer(wxHORIZONTAL); // Съдържа действителните бутони // Създава вложен хоризонтален BoxSizer за бутоните на табовете

    wxButton *terminalButton = new wxButton(tabPanel, ID_TERMINAL_TAB, "TERMINAL",              // Създава бутон "TERMINAL"
                                            wxDefaultPosition, wxSize(120, 40), wxBORDER_NONE); // По-широки бутони // Позиция, размер и стил
    wxButton *sqlButton = new wxButton(tabPanel, ID_SQL_TAB, "CONFIG",                          // Преименуван от SQL // Създава бутон "CONFIG"
                                       wxDefaultPosition, wxSize(120, 40), wxBORDER_NONE);      // Позиция, размер и стил

    // Първоначално стилизиране (табът Terminal е активен по подразбиране)
    terminalButton->SetBackgroundColour(wxColour(44, 49, 58)); // Цвят на активния таб // Задава цвят на фона на активния бутон (Terminal)
    terminalButton->SetForegroundColour(*wxWHITE);             // Задава бял цвят на текста на активния бутон
    sqlButton->SetBackgroundColour(wxColour(30, 30, 30));      // Цвят на неактивния таб // Задава цвят на фона на неактивния бутон (Config)
    sqlButton->SetForegroundColour(wxColour(156, 163, 175));   // Затъмнен текст за неактивен // Задава сив цвят на текста на неактивния бутон

    wxFont tabFont = terminalButton->GetFont(); // Взема шрифта на бутона Terminal
    tabFont.SetWeight(wxFONTWEIGHT_BOLD);       // Задава удебелен шрифт
    terminalButton->SetFont(tabFont);           // Прилага удебеления шрифт на бутона Terminal
    sqlButton->SetFont(tabFont);                // Прилага удебеления шрифт на бутона Config

    // Добавя бутоните към центрирания sizer
    centeredTabSizer->Add(terminalButton, 0, wxEXPAND); // Добавя бутона Terminal към центрирания sizer, разширяващ се
    // По избор: Добавя визуален разделител, ако е желано
    // wxStaticLine* separator = new wxStaticLine(tabPanel, wxID_ANY, wxDefaultPosition, wxSize(-1, 20), wxLI_VERTICAL); // Коментирано създаване на разделител
    // separator->SetBackgroundColour(wxColour(60,60,60)); // Цвят на разделителя // Коментирано задаване на цвят на разделителя
    // centeredTabSizer->Add(separator, 0, wxEXPAND | wxLEFT | wxRIGHT, 0); // Коментирано добавяне на разделителя към sizer-а
    centeredTabSizer->Add(sqlButton, 0, wxEXPAND); // Добавя бутона Config към центрирания sizer, разширяващ се

    // Центрира групата бутони хоризонтално
    tabSizer->AddStretchSpacer(1);                      // Добавя разтегливо празно пространство отляво
    tabSizer->Add(centeredTabSizer, 0, wxALIGN_CENTER); // Добавя центрирания sizer с бутоните, центриран
    tabSizer->AddStretchSpacer(1);                      // Добавя разтегливо празно пространство отдясно
    tabPanel->SetSizer(tabSizer);                       // Задава главния sizer на панела за табове

    // --- Панели със съдържание ---
    // Панел на таб Terminal
    m_terminalTab = new wxPanel(m_mainPanel);                 // Създава нов панел за таба Terminal
    m_terminalTab->SetBackgroundColour(wxColour(30, 30, 30)); // Съвпада с цвета на лентата с табове или е леко различен // Задава цвят на фона на панела Terminal
    wxBoxSizer *terminalSizer = new wxBoxSizer(wxVERTICAL);   // Създава вертикален BoxSizer за панела Terminal

    OSDetector::OS currentOS = m_osDetector.GetCurrentOS();          // Определя текущата операционна система
    if (currentOS == OSDetector::OS::Linux)                          // Проверява дали ОС е Linux
    {                                                                // Начало на блока if (ОС е Linux)
#ifdef __WXGTK__                                                     // Уверява се, че специфичният за Linux код е защитен // Условна компилация за GTK (Linux)
        m_terminalPanel = new LinuxTerminalPanel(m_terminalTab);     // Създава нов Linux терминален панел
        m_terminalPanel->SetMinSize(wxSize(680, 400));               // Минимален размер // Задава минимален размер на терминалния панел
        terminalSizer->Add(m_terminalPanel, 1, wxEXPAND | wxALL, 5); // Добавя с отстъп // Добавя терминалния панел към sizer-а с разширяване и отстъп
#else                                                                // Ако не е GTK
                                                                     // Резервен вариант за Linux компилации без GTK или специфичен терминален уиджет
        wxStaticText *linuxPlaceholder = new wxStaticText(m_terminalTab, wxID_ANY, "Linux Terminal Placeholder"); // Създава заместващ статичен текст
        terminalSizer->Add(linuxPlaceholder, 1, wxEXPAND | wxALL, 5);                                             // Добавя заместващия текст към sizer-а
#endif                                                               // Край на условната компилация за GTK
    } // Край на блока if (ОС е Linux)
#ifdef __WXMSW__                                                                                            // Условна компилация за Windows
    else if (currentOS == OSDetector::OS::Windows)                                                          // Проверява дали ОС е Windows (използва се else if, за да се избегне припокриване, ако currentOS може да има други стойности)
    {                                                                                                       // Начало на блока else if (ОС е Windows)
        wxPanel *winTerminalPanel = new wxPanel(m_terminalTab, wxID_ANY, wxDefaultPosition, wxDefaultSize); // Създава нов панел за Windows терминала
        // Нека sizer-ът управлява размера, няма нужда от изричен размер тук, освен ако не е необходимо
        winTerminalPanel->SetBackgroundColour(wxColour(12, 12, 12));  // Стандартно черно за Windows Terminal // Задава черен цвят на фона
        terminalSizer->Add(winTerminalPanel, 1, wxEXPAND | wxALL, 0); // Без отстъп за вградения терминал // Добавя панела към sizer-а с разширяване, без отстъп

        m_winTerminalManager = new WinTerminalManager(winTerminalPanel); // Създава нов WinTerminalManager
        if (!m_winTerminalManager->Initialize())                         // Проверява дали инициализацията на терминалния мениджър е неуспешна
        {                                                                // Начало на блока if (неуспешна инициализация)
            wxLogError("Failed to initialize Windows terminal");         // Записва грешка в лога
            // По избор показва съобщение за грешка в панела
            delete m_winTerminalManager;                                                                                       // Изтрива обекта WinTerminalManager
            m_winTerminalManager = nullptr;                                                                                    // Занулява указателя
            wxStaticText *errorText = new wxStaticText(winTerminalPanel, wxID_ANY, "Failed to initialize embedded terminal."); // Създава текст за грешка
            wxBoxSizer *errorSizer = new wxBoxSizer(wxVERTICAL);                                                               // Създава sizer за текста за грешка
            errorSizer->Add(errorText, 1, wxCENTER | wxALL, 10);                                                               // Добавя текста за грешка към sizer-а, центриран с отстъп
            winTerminalPanel->SetSizer(errorSizer);                                                                            // Задава sizer-а на панела
        } // Край на блока if (неуспешна инициализация)
    } // Край на блока else if (ОС е Windows)
#endif   // Край на условната компилация за Windows
    else // За други операционни системи
    {    // Начало на блока else (други ОС)
        // Запазено място за други ОС или ако не е имплементиран специфичен терминал
        wxStaticText *osPlaceholder = new wxStaticText(m_terminalTab, wxID_ANY, "Terminal not available on this OS."); // Създава заместващ статичен текст
        terminalSizer->Add(osPlaceholder, 1, wxEXPAND | wxALL, 5);                                                     // Добавя заместващия текст към sizer-а
    } // Край на блока else (други ОС)
    m_terminalTab->SetSizer(terminalSizer); // Задава sizer-а на панела Terminal

    // Панел на таб Config (използва клас SQLTab)
    m_sqlTab = new SQLTab(m_mainPanel, m_projectDir); // Предава projectDir // Създава нов обект SQLTab
    m_sqlTab->Hide();                                 // Започва скрит // Скрива панела Config първоначално

    // MongoDB Панел
    m_mongoPanel = new MongoDBPanel(m_mainPanel, m_projectDir); // Предава projectDir // Създава нов обект MongoDBPanel
    m_mongoPanel->Hide();                                       // Скрива MongoDB панела първоначално
    // --- Долен панел с бутони ---
    wxPanel *buttonPanel = new wxPanel(m_mainPanel);        // Създава нов долен панел за бутони
    buttonPanel->SetBackgroundColour(wxColour(30, 30, 30)); // Съвпада с лентата с табове // Задава цвят на фона на долния панел
    wxBoxSizer *buttonSizer = new wxBoxSizer(wxHORIZONTAL); // Създава хоризонтален BoxSizer за долния панел

    wxButton *nextButton = new wxButton(buttonPanel, ID_NEXT_BUTTON, "Create ISO", // По-описателен етикет // Създава бутон "Create ISO"
                                        wxDefaultPosition, wxSize(140, 35));       // По-широк бутон // Позиция и размер на бутона
    // Стилизира бутона (Обмислете използването на ThemeConfig)
    nextButton->SetBackgroundColour(wxColour(189, 147, 249)); // Отличителен акцентен цвят // Задава цвят на фона на бутона
    nextButton->SetForegroundColour(*wxWHITE);                // Задава бял цвят на текста на бутона
    wxFont bottomButtonFont = nextButton->GetFont();          // Взема текущия шрифт на бутона
    bottomButtonFont.SetWeight(wxFONTWEIGHT_BOLD);            // Задава удебелен шрифт
    nextButton->SetFont(bottomButtonFont);                    // Прилага новия шрифт на бутона

    buttonSizer->AddStretchSpacer(1);                                     // Избутва бутона надясно // Добавя разтегливо празно пространство отляво
    buttonSizer->Add(nextButton, 0, wxALL | wxALIGN_CENTER_VERTICAL, 10); // Добавя отстъп // Добавя бутона към sizer-а с отстъп и вертикално центриране
    buttonPanel->SetSizer(buttonSizer);                                   // Задава sizer-а на долния панел

    // --- Сглобяване на основния Layout ---
    mainSizer->Add(topPanel, 0, wxEXPAND); // Добавя горния панел към основния sizer, разширяващ се хоризонтално
    // По избор: Добавя разделителна линия под горната лента
    // mainSizer->Add(new wxStaticLine(m_mainPanel, wxID_ANY), 0, wxEXPAND | wxLEFT | wxRIGHT, 5); // Коментирано добавяне на статична линия
    mainSizer->Add(tabPanel, 0, wxEXPAND);      // Добавя лентата с табове // Добавя панела за табове към основния sizer
    mainSizer->Add(m_terminalTab, 1, wxEXPAND); // Добавя панела със съдържание на терминала (първоначално показан) // Добавя панела Terminal, заемащ оставащото вертикално пространство
    mainSizer->Add(m_sqlTab, 1, wxEXPAND);      // Добавя панела със съдържание на конфигурацията (първоначално скрит) // Добавя панела Config
    mainSizer->Add(m_mongoPanel, 1, wxEXPAND);  // Добавя панела със съдържание на mongo (първоначално скрит) // Добавя MongoDB панела
    // Добавя разделителна линия над долните бутони
    mainSizer->Add(new wxStaticLine(m_mainPanel, wxID_ANY), 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 5); // Добавя статична разделителна линия
    mainSizer->Add(buttonPanel, 0, wxEXPAND);                                                           // Добавя долния панел с бутони // Добавя долния панел към основния sizer

    m_mainPanel->SetSizer(mainSizer); // Задава основния sizer на основния панел

    // Задава ограничения за общия размер на рамката
    SetMinSize(wxSize(800, 650)); // Коригиран минимален размер // Задава минимален размер на прозореца
    SetSize(wxSize(850, 700));    // Малко по-голям размер по подразбиране // Задава размер по подразбиране на прозореца
} // Край на тялото на метода CreateControls

// Обработва събитието за затваряне на прозореца: скрива прозореца и изтрива Docker контейнера асинхронно
void SecondWindow::OnClose(wxCloseEvent &event)                                      // Метод, извиква се при опит за затваряне на прозореца
{                                                                                    // Начало на тялото на метода OnClose
    if (m_isClosing)                                                                 // Проверява дали процесът на затваряне вече е стартиран
    {                                                                                // Начало на блока if (m_isClosing)
        wxLogDebug("SecondWindow::OnClose - Already in closing process, skipping."); // Записва в лога, че затварянето вече е в ход
        return;                                                                      // Прозорецът ще бъде унищожен от DoDestroy
    } // Край на блока if (m_isClosing)

    if (m_containerId.IsEmpty())                                      // ID-то на контейнера се записва от script.exe в container_id.txt
        m_containerId = ContainerCommand::ReadContainerId(m_projectDir); // Чете го, ако все още не е известно

    // Контейнерът съдържа всички промени по ISO-то, затова потребителят потвърждава изтриването му
    if (event.CanVeto() && !m_containerId.IsEmpty())
    {
        int answer = wxMessageBox("Closing this window deletes the Docker container together with every change "
                                  "that is not in a created ISO yet.\n\nClose anyway?",
                                  "Close Project", wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION, this);
        if (answer != wxYES)
        {
            event.Veto(); // Потребителят се отказа от затварянето
            return;
        }
    }

    wxLogDebug("SecondWindow::OnClose - Starting close process."); // Записва в лога началото на процеса на затваряне
    m_isClosing = true;                                            // Задава флага за процес на затваряне на true
    Hide();                                                        // Скрива прозореца незабавно
    CleanupContainerAsync();                                       // Изтрива контейнера и след това унищожава прозореца
} // Край на тялото на метода OnClose

// Действително унищожава прозореца и показва отново началния прозорец
void SecondWindow::DoDestroy()                                  // Метод за действително унищожаване на прозореца
{                                                               // Начало на тялото на метода DoDestroy
    wxLogDebug("SecondWindow::DoDestroy - Calling Destroy()."); // Записва в лога извикването на Destroy()
    wxWindow *parent = GetParent();                             // MainFrame беше скрит при отваряне на този прозорец
    if (parent && !parent->IsBeingDeleted())                    // Ако приложението не се затваря изцяло
        parent->Show();                                         // Показва го отново, за да може да се започне нов проект
    Destroy();                                                  // Унищожава прозореца
} // Край на тялото на метода DoDestroy

// Изтрива Docker контейнера с "docker rm -f" без да блокира интерфейса и след това унищожава прозореца
void SecondWindow::CleanupContainerAsync()                                           // Метод за асинхронно изтриване на контейнера
{                                                                                    // Начало на тялото на метода CleanupContainerAsync
    if (m_containerId.IsEmpty())                                                     // Ако няма създаден контейнер
    {                                                                                // Начало на блока if
        wxLogDebug("SecondWindow::CleanupContainerAsync - No container ID, nothing to remove."); // Записва в лога, че няма какво да се изтрие
        CallAfter(&SecondWindow::DoDestroy);                                         // Унищожава прозореца след текущото събитие
        return;                                                                      // Прекратява изпълнението на метода
    } // Край на блока if

    wxLogDebug("SecondWindow::CleanupContainerAsync - Removing container %s", m_containerId); // Записва в лога ID-то на контейнера
    wxWeakRef<SecondWindow> self(this);                                                       // Прозорецът може да бъде унищожен от родителя преди края на командата
    long pid = ContainerCommand::RunAsync("docker rm -f " + m_containerId, [self](int status) // Стартира изтриването асинхронно
    {
        wxLogDebug("SecondWindow::CleanupContainerAsync - docker rm finished with status %d", status); // Записва в лога резултата
        if (self)                       // Ако прозорецът все още съществува
            self->DoDestroy();          // Унищожава го
    });
    if (pid == 0)                        // Ако командата не може да стартира
        CallAfter(&SecondWindow::DoDestroy); // Все пак затваря прозореца
} // Край на тялото на метода CleanupContainerAsync

// Обработва бутона 'Create ISO': стартира /create_iso.sh в контейнера и копира готовия ISO в директорията на проекта
void SecondWindow::OnNext(wxCommandEvent &event)                                // Метод, извиква се при натискане на бутона "Create ISO"
{                                                                               // Начало на тялото на метода OnNext
    wxButton *nextButton = wxDynamicCast(FindWindow(ID_NEXT_BUTTON), wxButton); // Намира бутона "Create ISO" по ID и го преобразува към wxButton указател

    wxString containerId = ContainerCommand::ReadContainerId(m_projectDir); // Чете ID-то на контейнера от container_id.txt
    if (containerId.IsEmpty())                                                                                                         // Ако контейнерът още не е готов
    {                                                                                                                                  // Начало на блока if
        wxLogError("OnNext: Container ID file not found or empty!");                                                                   // Записва грешка в лога
        wxMessageBox("Container information not found. Has the initial setup completed?", "Configuration Error", wxOK | wxICON_ERROR, this); // Показва съобщение за грешка
        return;                                                                                                                        // Прекратява изпълнението на метода
    } // Край на блока if

    if (nextButton)            // Проверява дали бутонът е намерен успешно
        nextButton->Disable(); // Деактивира бутона, докато ISO-то се създава

    // Показва overlay
    if (m_overlay)            // Проверява дали наслагващият екран вече съществува
    {                         // Начало на блока if (m_overlay) // Унищожава съществуващия overlay, ако има такъв
        m_overlay->Destroy(); // Унищожава съществуващия наслагващ екран
        m_overlay = nullptr;  // Занулява указателя
    } // Край на блока if (m_overlay)
    m_overlay = new OverlayFrame(this);   // Създава нов наслагващ екран
    wxLogDebug("OnNext: Overlay shown."); // Записва в лога, че наслагващият екран е показан

    // Стартира create_iso.sh (създаден от script.exe) в контейнера; DockerExecProcess копира ISO-то при успех
    wxString command = wxString::Format("docker exec %s /bin/bash /create_iso.sh", containerId); // Командата за създаване на ISO
    DockerExecProcess *process = new DockerExecProcess(this);                                    // Обработва резултата в OnTerminate
    long pid = wxExecute(command, wxEXEC_ASYNC | wxEXEC_HIDE_CONSOLE, process);                  // Стартира командата асинхронно
    if (pid == 0)                                                                                // Ако командата не може да стартира
    {
        delete process;   // При неуспех wxExecute не поема обекта
        CloseOverlay();   // Затваря наслагващия екран
        if (nextButton)
            nextButton->Enable();
        wxMessageBox("Could not start docker. Is Docker Desktop running?", "Execution Error", wxOK | wxICON_ERROR, this);
        return;
    }
    wxLogDebug("OnNext: create_iso.sh started, PID: %ld", pid); // Записва в лога PID на процеса
} // Край на тялото на метода OnNext

// Обработва смяната на табове между Terminal и Config, обновява стила на бутоните
void SecondWindow::OnTabChanged(wxCommandEvent &event)                               // Метод, извиква се при смяна на таб
{                                                                                    // Начало на тялото на метода OnTabChanged
    wxButton *terminalButton = wxDynamicCast(FindWindow(ID_TERMINAL_TAB), wxButton); // Намира бутона за таб Terminal
    wxButton *configButton = wxDynamicCast(FindWindow(ID_SQL_TAB), wxButton);        // Променено име на променливата // Намира бутона за таб Config (преди SQL)

    // Първо деактивира визуално и двата бутона
    const wxColour inactiveBg(30, 30, 30);    // Дефинира цвят за фон на неактивен бутон
    const wxColour inactiveFg(156, 163, 175); // Дефинира цвят за текст на неактивен бутон
    const wxColour activeBg(44, 49, 58);      // Дефинира цвят за фон на активен бутон
    const wxColour activeFg(*wxWHITE);        // Дефинира цвят за текст на активен бутон (бял)

    if (terminalButton)                                  // Проверява дали бутонът Terminal съществува
    {                                                    // Начало на блока if (terminalButton)
        terminalButton->SetBackgroundColour(inactiveBg); // Задава фон на неактивен
        terminalButton->SetForegroundColour(inactiveFg); // Задава текст на неактивен
        terminalButton->Refresh();                       // Прилага промените // Обновява бутона, за да се видят промените
    } // Край на блока if (terminalButton)
    if (configButton)                                  // Проверява дали бутонът Config съществува
    {                                                  // Начало на блока if (configButton)
        configButton->SetBackgroundColour(inactiveBg); // Задава фон на неактивен
        configButton->SetForegroundColour(inactiveFg); // Задава текст на неактивен
        configButton->Refresh();                       // Прилага промените // Обновява бутона
    } // Край на блока if (configButton)

    // Първо скрива всички основни панели със съдържание
    m_terminalTab->Hide(); // Скрива панела Terminal
    m_sqlTab->Hide();      // Скрива панела Config
    m_mongoPanel->Hide();  // Също скрива mongo панела, ако е бил отворен // Скрива MongoDB панела

    // Показва избрания таб и активира неговия бутон
    int id = event.GetId();                                // Взема ID-то на бутона, предизвикал събитието
    if (id == ID_TERMINAL_TAB)                             // Проверява дали е натиснат бутонът за таб Terminal
    {                                                      // Начало на блока if (id == ID_TERMINAL_TAB)
        m_terminalTab->Show();                             // Показва панела Terminal
        if (terminalButton)                                // Проверява дали бутонът Terminal съществува
        {                                                  // Начало на блока if (terminalButton)
            terminalButton->SetBackgroundColour(activeBg); // Задава фон на активен
            terminalButton->SetForegroundColour(activeFg); // Задава текст на активен
            terminalButton->Refresh();                     // Обновява бутона
        } // Край на блока if (terminalButton)
        m_lastTab = m_terminalTab;              // Обновява последния активен таб // Запазва текущия таб като последно активен
        wxLogDebug("Switched to Terminal Tab"); // Записва в лога смяната на таба
    } // Край на блока if (id == ID_TERMINAL_TAB)
    else if (id == ID_SQL_TAB)                           // Проверява дали е натиснат бутонът за таб Config
    {                                                    // Начало на блока else if (id == ID_SQL_TAB) // Променена проверка на ID
        m_sqlTab->Show();                                // Показва панела Config
        if (configButton)                                // Проверява дали бутонът Config съществува
        {                                                // Начало на блока if (configButton)
            configButton->SetBackgroundColour(activeBg); // Задава фон на активен
            configButton->SetForegroundColour(activeFg); // Задава текст на активен
            configButton->Refresh();                     // Обновява бутона
        } // Край на блока if (configButton)
        m_lastTab = m_sqlTab;                 // Обновява последния активен таб // Запазва текущия таб като последно активен
        wxLogDebug("Switched to Config Tab"); // Записва в лога смяната на таба
    } // Край на блока else if (id == ID_SQL_TAB)

    // Нулира външния вид на бутона MongoDB, ако е бил активен
    wxButton *mongoButton = wxDynamicCast(FindWindow(ID_MONGODB_BUTTON), wxButton); // Намира бутона MongoDB
    if (mongoButton)                                                                // Проверява дали бутонът MongoDB съществува
    {                                                                               // Начало на блока if (mongoButton)
        mongoButton->SetBackgroundColour(wxColour(77, 171, 68));                    // Оригинално зелено // Възстановява оригиналния зелен цвят на фона
        mongoButton->Refresh();                                                     // Обновява бутона
    } // Край на блока if (mongoButton)

    m_mainPanel->Layout(); // Преизчислява layout-а за основния панел // Обновява layout-а на основния панел
} // Край на тялото на метода OnTabChanged

// Обработва натискането на MongoDB бутона: показва MongoDB панела и обновява стила на бутона
void SecondWindow::OnMongoDBButton(wxCommandEvent &event) // Метод, извиква се при натискане на бутона MongoDB
{                                                         // Начало на тялото на метода OnMongoDBButton
    wxLogDebug("MongoDB button clicked");                 // Записва в лога натискането на бутона
    // Запазва кой таб е бил видим преди това
    if (m_terminalTab->IsShown())  // Проверява дали табът Terminal е бил показан
    {                              // Начало на блока if (m_terminalTab->IsShown())
        m_lastTab = m_terminalTab; // Запазва Terminal като последно активен таб
    } // Край на блока if (m_terminalTab->IsShown())
    else if (m_sqlTab->IsShown()) // Проверява дали табът Config е бил показан
    {                             // Начало на блока else if (m_sqlTab->IsShown())
        m_lastTab = m_sqlTab;     // Запазва Config като последно активен таб
    } // Край на блока else if (m_sqlTab->IsShown())
    else                           // Ако нито един от познатите табове не е бил показан
    {                              // Начало на блока else
        m_lastTab = m_terminalTab; // Резервен вариант по подразбиране, ако нито един от познатите табове не е показан // Запазва Terminal като резервен вариант
    } // Край на блока else

    wxLogDebug("Last active tab stored."); // Записва в лога, че последният активен таб е запазен

    // Скрива другите табове
    m_terminalTab->Hide();                          // Скрива таба Terminal
    m_sqlTab->Hide();                               // Скрива таба Config
    wxLogDebug("Terminal and Config tabs hidden."); // Записва в лога, че табовете са скрити

    // Показва и зарежда MongoDB панела
    m_mongoPanel->Show();                             // Показва MongoDB панела
    wxLogDebug("MongoDB panel shown.");               // Записва в лога, че MongoDB панелът е показан
    m_mongoPanel->LoadMongoDBContent();               // Зарежда/обновява данните при показване // Извиква метод за зареждане на съдържанието на MongoDB
    wxLogDebug("MongoDB content loading initiated."); // Записва в лога, че зареждането на съдържанието е започнало

    // По избор: Променя външния вид на бутона, за да покаже активно състояние
    wxButton *mongoButton = wxDynamicCast(event.GetEventObject(), wxButton); // Взема указател към натиснатия бутон
    if (mongoButton)                                                         // Проверява дали бутонът е валиден
    {                                                                        // Начало на блока if (mongoButton)
        mongoButton->SetBackgroundColour(wxColour(100, 200, 100));           // Пример: По-светло зелено // Задава по-светло зелен цвят на фона
        mongoButton->Refresh();                                              // Обновява бутона
        wxLogDebug("MongoDB button appearance updated to active state.");    // Записва в лога, че външният вид на бутона е обновен
    } // Край на блока if (mongoButton)

    m_mainPanel->Layout();                    // Обновява layout-а // Обновява layout-а на основния панел
    wxLogDebug("Main panel layout updated."); // Записва в лога, че layout-ът е обновен
} // Край на тялото на метода OnMongoDBButton

// Обработва събитието за затваряне на MongoDB панела: скрива го и възстановява последния активен таб
void SecondWindow::OnMongoDBPanelClose(wxCommandEvent &event)       // Метод, извиква се при затваряне на MongoDB панела
{                                                                   // Начало на тялото на метода OnMongoDBPanelClose
    wxLogDebug("SecondWindow::OnMongoDBPanelClose received event"); // Записва в лога получаването на събитието
    m_mongoPanel->Hide();                                           // Скрива MongoDB панела

    // Показва предишния активен таб
    if (m_lastTab)                               // Проверява дали има запазен последно активен таб
    {                                            // Начало на блока if (m_lastTab)
        m_lastTab->Show();                       // Показва последния активен таб
        wxLogDebug("Restored last active tab."); // Записва в лога, че последният активен таб е възстановен
    } // Край на блока if (m_lastTab)
    else                                                  // Ако няма запазен последно активен таб
    {                                                     // Начало на блока else
        m_terminalTab->Show();                            // Резервен вариант по подразбиране към таб терминал // Показва таба Terminal като резервен вариант
        wxLogDebug("Fell back to showing Terminal tab."); // Записва в лога, че е показан резервният таб
    } // Край на блока else

    // Нулира външния вид на бутона MongoDB до нормалното му състояние
    wxButton *mongoButton = wxDynamicCast(FindWindow(ID_MONGODB_BUTTON), wxButton); // Намира бутона MongoDB
    if (mongoButton)                                                                // Проверява дали бутонът MongoDB е валиден
    {                                                                               // Начало на блока if (mongoButton)
        mongoButton->SetBackgroundColour(wxColour(77, 171, 68));                    // Оригинално MongoDB зелено // Възстановява оригиналния зелен цвят на фона
        mongoButton->Refresh();                                                     // Обновява бутона
        wxLogDebug("MongoDB button appearance reset.");                             // Записва в лога, че външният вид на бутона е нулиран
    } // Край на блока if (mongoButton)

    m_mainPanel->Layout();                                                // Обновява layout-а // Обновява layout-а на основния панел
    wxLogDebug("Main panel layout updated after closing MongoDB panel."); // Записва в лога, че layout-ът е обновен
} // Край на тялото на метода OnMongoDBPanelClose