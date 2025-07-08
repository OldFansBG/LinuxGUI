#pragma once // Директива за еднократно включване на заглавния файл

#include <wx/wx.h>       // Core wxWidgets header // Включва основния заглавен файл на wxWidgets
#include <wx/statbox.h>  // For wxStaticBoxSizer // Включва заглавен файл за wxStaticBoxSizer (статична кутия със заглавие)
#include <wx/checkbox.h> // For wxCheckBox // Включва заглавен файл за wxCheckBox (квадратче за отметка)
#include <wx/choice.h>   // For wxChoice // Включва заглавен файл за wxChoice (падащ списък)
#include <wx/textctrl.h> // For wxTextCtrl // Включва заглавен файл за wxTextCtrl (текстово поле за въвеждане)
#include <wx/sizer.h>    // Include the main sizer header // Включва основния заглавен файл за sizer-и (мениджъри на подредба)
#include <wx/process.h>  // Include for wxExecute (even if disabled) // Включва заглавен файл за wxExecute (изпълнение на процеси, дори ако е деактивирано)

// --- Class Definition --- // --- Декларация на клас ---

class SystemTab : public wxPanel // Декларация на клас SystemTab, който наследява wxPanel (панел за потребителски интерфейс)
{ // Начало на тялото на класа
public: // Секция за публични членове на класа
    /** // Начало на Doxygen коментарен блок
     * @brief Constructor for the SystemTab panel. // Кратко описание: Конструктор за панела SystemTab.
     * @param parent The parent window. // Параметър parent: Родителският прозорец.
     * @param containerId The ID of the Docker container being configured (can be empty initially). // Параметър containerId: Идентификаторът на Docker контейнера, който се конфигурира (може да е празен първоначално).
     */ // Край на Doxygen коментарен блок
    SystemTab(wxWindow *parent, const wxString &containerId = wxEmptyString); // Конструктор на SystemTab, приема родителски прозорец и ID на контейнер (по подразбиране празен низ)

private: // Секция за частни членове на класа
    /** // Начало на Doxygen коментарен блок
     * @brief Creates all the UI controls within the tab. // Кратко описание: Създава всички контроли на потребителския интерфейс в раздела.
     */ // Край на Doxygen коментарен блок
    void CreateControls(); // Декларация на метод за създаване на контролите

    /** // Начало на Doxygen коментарен блок
     * @brief Applies the defined theme colors to the controls. // Кратко описание: Прилага дефинираните цветове на темата към контролите.
     */ // Край на Doxygen коментарен блок
    void ApplyThemeColors(); // Декларация на метод за прилагане на цветовете на темата

    /** // Начало на Doxygen коментарен блок
     * @brief Populates UI controls from internal state and logs simulated read operations. // Кратко описание: Попълва контролите на потребителския интерфейс от вътрешното състояние и регистрира симулирани операции за четене.
     */ // Край на Doxygen коментарен блок
    void LoadSettings(); // Декларация на метод за зареждане на настройките

    /** // Начало на Doxygen коментарен блок
     * @brief Reads UI control values, updates internal state, and simulates (logs + disabled execute) // Кратко описание: Чете стойностите на контролите на ПИ, актуализира вътрешното състояние и симулира (регистрира + деактивирано изпълнение)
     *        applying changes to the container/chroot environment. // прилагане на промени в средата на контейнера/chroot.
     */ // Край на Doxygen коментарен блок
    void ApplySettings(); // Декларация на метод за прилагане на настройките

    // --- Event Handlers --- // --- Обработчици на събития ---
    /** // Начало на Doxygen коментарен блок
     * @brief Handles toggling the 'Lock root account' checkbox. // Кратко описание: Обработва превключването на квадратчето за отметка 'Заключи root акаунта'.
     * @param event The command event. // Параметър event: Събитието от командата.
     */ // Край на Doxygen коментарен блок
    void OnLockRootCheck(wxCommandEvent &event); // Декларация на обработчик на събитие за отметката за заключване на root акаунта

    /** // Начало на Doxygen коментарен блок
     * @brief Generic handler for changes in input controls. Triggers ApplySettings. // Кратко описание: Общ обработчик за промени във входните контроли. Задейства ApplySettings.
     * @param event The command event. // Параметър event: Събитието от командата.
     */ // Край на Doxygen коментарен блок
    void OnControlChange(wxCommandEvent &event); // Декларация на общ обработчик на събитие за промяна в контрол

    // --- Helper Methods --- // --- Помощни методи ---
    /** // Начало на Doxygen коментарен блок
     * @brief Logs and conditionally executes a command within the chroot environment via docker exec. // Кратко описание: Регистрира и условно изпълнява команда в chroot средата чрез docker exec.
     * @param commandInChroot The command to run inside the chroot (e.g., "localectl set-locale LANG=en_US.UTF-8"). // Параметър commandInChroot: Командата за изпълнение вътре в chroot (напр. "localectl set-locale LANG=en_US.UTF-8").
     * @param description A description of the action for logging purposes. // Параметър description: Описание на действието за целите на регистриране.
     * @return True if the (simulated) command succeeded, false otherwise. // Връща: True, ако (симулираната) команда е успешна, иначе false.
     */ // Край на Doxygen коментарен блок
    bool ExecuteChrootCommand(const wxString &commandInChroot, const wxString &description); // Декларация на метод за изпълнение на команда в chroot среда

    /** // Начало на Doxygen коментарен блок
     * @brief Logs and conditionally executes a command directly within the container via docker exec. // Кратко описание: Регистрира и условно изпълнява команда директно в контейнера чрез docker exec.
     * @param commandAfterExec The command to run after 'docker exec <id>' (e.g., "/bin/bash -c \"echo '...' > /etc/hostname\""). // Параметър commandAfterExec: Командата за изпълнение след 'docker exec <id>' (напр. "/bin/bash -c \"echo '...' > /etc/hostname\"").
     * @param description A description of the action for logging purposes. // Параметър description: Описание на действието за целите на регистриране.
     * @return True if the (simulated) command succeeded, false otherwise. // Връща: True, ако (симулираната) команда е успешна, иначе false.
     */ // Край на Doxygen коментарен блок
    bool ExecuteDockerCommand(const wxString &commandAfterExec, const wxString &description); // Декларация на метод за изпълнение на Docker команда

    // --- Control Pointers (Member Variables) --- // --- Указатели към контроли (променливи-членове) ---
    wxTextCtrl *m_hostnameInput; // Указател към текстово поле за име на хост
    wxChoice *m_languageChoice; // Указател към падащ списък за избор на език
    wxChoice *m_keyboardLayoutChoice; // Указател към падащ списък за избор на клавиатурна подредба
    wxChoice *m_timezoneChoice; // Указател към падащ списък за избор на часова зона
    wxTextCtrl *m_usernameInput; // Указател към текстово поле за потребителско име
    wxTextCtrl *m_fullNameInput; // Указател към текстово поле за пълно име
    wxTextCtrl *m_userPasswordInput; // Указател към текстово поле за парола на потребител
    wxTextCtrl *m_confirmUserPasswordInput; // Указател към текстово поле за потвърждение на паролата на потребител
    wxCheckBox *m_adminCheckbox; // Указател към квадратче за отметка за администраторски права
    wxCheckBox *m_lockRootCheckbox; // Указател към квадратче за отметка за заключване на root акаунта
    wxStaticText *m_rootPasswordLabel; // Указател към статичен текст (етикет) за паролата на root
    wxTextCtrl *m_rootPasswordInput; // Указател към текстово поле за парола на root
    wxStaticText *m_confirmRootPasswordLabel; // Указател към статичен текст (етикет) за потвърждение на паролата на root
    wxTextCtrl *m_confirmRootPasswordInput; // Указател към текстово поле за потвърждение на паролата на root
    wxFlexGridSizer *m_rootPasswordSizer; // Указател към wxFlexGridSizer за подредба на полетата за root парола

    // --- State Variables (for tracking internal config) --- // --- Променливи за състоянието (за проследяване на вътрешната конфигурация) ---
    wxString m_currentHostname; // Променлива за съхранение на текущото име на хост
    int m_currentLanguageIndex; // Променлива за съхранение на индекса на текущия избран език
    int m_currentKeyboardIndex; // Променлива за съхранение на индекса на текущата избрана клавиатурна подредба
    int m_currentTimezoneIndex; // Променлива за съхранение на индекса на текущата избрана часова зона
    wxString m_currentUsername; // Променлива за съхранение на текущото потребителско име
    wxString m_currentFullName; // Променлива за съхранение на текущото пълно име
    wxString m_currentUserPassword; // Променлива за съхранение на текущата парола на потребител
    bool m_isAdmin; // Булева променлива, указваща дали потребителят е администратор
    bool m_isRootLocked; // Булева променлива, указваща дали root акаунтът е заключен
    wxString m_currentRootPassword; // Променлива за съхранение на текущата парола на root

    // --- Container and Path Info --- // --- Информация за контейнера и пътищата ---
    wxString m_containerId; // Променлива за съхранение на ID на Docker контейнера
    const wxString m_chrootPath = "/root/custom_iso/squashfs-root"; // Standard path inside container // Константен низ за пътя до chroot средата (стандартен път вътре в контейнера)

    // --- Styling Variables --- // --- Променливи за стилизиране ---
    wxColour m_bgColor; // Променлива за цвят на фона
    wxColour m_panelBgColor; // Променлива за цвят на фона на панела
    wxColour m_textColor; // Променлива за цвят на текста
    wxColour m_inputBgColor; // Променлива за цвят на фона на полетата за въвеждане
    wxColour m_inputFgColor; // Променлива за цвят на текста в полетата за въвеждане (предния план)
    wxColour m_labelColor; // Променлива за цвят на етикетите

    // --- Event Table Declaration --- // --- Декларация на таблицата на събитията ---
    wxDECLARE_EVENT_TABLE(); // Макрос за деклариране на таблицата на събитията на wxWidgets
}; // Край на дефиницията на класа SystemTab