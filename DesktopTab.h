#ifndef DESKTOPTAB_H // Ако макросът DESKTOPTAB_H не е дефиниран (предпазител за включване - include guard)
#define DESKTOPTAB_H // Дефинира макроса DESKTOPTAB_H, за да се предотврати многократно включване на този файл

#include <wx/wx.h>        // Включва основния заглавен файл на библиотеката wxWidgets за GUI функционалност
#include "CustomEvents.h" // Включва заглавния файл "CustomEvents.h" - Включва споделения заглавен файл за потребителски събития

// Предварителни декларации (Forward declarations)
class SecondWindow; // Предварителна декларация на клас SecondWindow, за да се избегне циклично включване

class DesktopTab : public wxPanel
{                                                 // Декларация на клас DesktopTab, който наследява от wxPanel (панел в wxWidgets)
public:                                           // Секция за публични членове на класа
    DesktopTab(wxWindow *parent);                 // Конструктор на класа, приемащ указател към родителския прозорец
    void RecalculateLayout(int windowWidth);      // Метод за преизчисляване на оформлението на таба, базирано на ширината на прозореца
    void UpdateTextFromFile();                    // Метод за обновяване на текстово съдържание от файл
    void UpdateGUILabel(const wxString &guiName); // Метод за обновяване на етикет в потребителския интерфейс
    wxString GetGuiFilePath();                    // Метод за получаване на пътя до файла, свързан с GUI елемента

private:                                                     // Секция за частни членове на класа (достъпни само вътре в класа)
    void CreateDesktopTab();                                 // Частен метод за създаване на съдържанието и елементите на "Desktop" таба
    wxBitmap LoadImage(const wxString &imageName);           // Частен метод за зареждане на изображение по дадено име
    void OnSize(wxSizeEvent &event);                         // Обработчик на събитие, извиквано при промяна на размера на таба
    void OnFileCopyComplete(wxCommandEvent &event);          // Обработчик на събитие, извиквано след завършване на операция по копиране на файл
    wxBitmap ConvertToGrayscale(const wxBitmap &original);   // Частен метод за конвертиране на цветно изображение (wxBitmap) в сива скала
    void ApplyGrayscaleToCards(const wxString &detectedEnv); // Частен метод за прилагане на ефект "сива скала" към визуални "карти" в зависимост от засечената среда

    wxFlexGridSizer *m_gridSizer;       // Указател към wxFlexGridSizer, използван за подреждане на елементи в грид
    wxScrolledWindow *m_scrolledWindow; // Указател към wxScrolledWindow, позволяващ скролиране на съдържанието
    wxStaticText *m_textDisplay;        // Указател към wxStaticText, използван за показване на статичен текст
    wxButton *m_reloadButton;           // Указател към wxButton, вероятно бутон за презареждане/обновяване
}; // Край на дефиницията на клас DesktopTab

#endif // DESKTOPTAB_H // Край на предпазителя за включване (директива #ifndef)