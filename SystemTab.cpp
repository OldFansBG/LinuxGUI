#include "SystemTab.h"        // Декларация на класа SystemTab
#include "ContainerCommand.h" // Команди в chroot средата на Docker контейнера
#include <vector>             // std::vector
#include <wx/log.h>           // wxLogDebug
#include <wx/regex.h>         // wxRegEx за проверка на hostname и потребителско име

// ID-та на контролите, които имат обработчици
const int ID_LOCK_ROOT_CHECKBOX = wxID_HIGHEST + 500;
const int ID_APPLY_SYSTEM_SETTINGS = wxID_HIGHEST + 501;

wxBEGIN_EVENT_TABLE(SystemTab, wxPanel)
    EVT_CHECKBOX(ID_LOCK_ROOT_CHECKBOX, SystemTab::OnLockRootCheck)
    EVT_BUTTON(ID_APPLY_SYSTEM_SETTINGS, SystemTab::OnApply)
wxEND_EVENT_TABLE()

SystemTab::SystemTab(wxWindow *parent, const wxString &projectDir)
    : wxPanel(parent),
      m_hostnameInput(nullptr), m_languageChoice(nullptr), m_keyboardLayoutChoice(nullptr),
      m_timezoneChoice(nullptr), m_usernameInput(nullptr), m_fullNameInput(nullptr),
      m_userPasswordInput(nullptr), m_confirmUserPasswordInput(nullptr), m_adminCheckbox(nullptr),
      m_lockRootCheckbox(nullptr), m_rootPasswordLabel(nullptr), m_rootPasswordInput(nullptr),
      m_confirmRootPasswordLabel(nullptr), m_confirmRootPasswordInput(nullptr), m_rootPasswordSizer(nullptr),
      m_applyButton(nullptr),
      m_projectDir(projectDir)
{
    // Цветова схема (тъмна)
    m_bgColor = wxColour(18, 18, 27);
    m_panelBgColor = wxColour(30, 30, 46);
    m_textColor = wxColour(205, 214, 244);
    m_inputBgColor = wxColour(49, 50, 68);
    m_inputFgColor = wxColour(205, 214, 244);
    m_labelColor = wxColour(166, 173, 200);

    SetBackgroundColour(m_bgColor);
    CreateControls();
    LoadSettings();
    ApplyThemeColors();
}

// Създава контролите на таба
void SystemTab::CreateControls()
{
    wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->AddSpacer(20);

    // --- Локализация ---
    wxStaticBox *localeBox = new wxStaticBox(this, wxID_ANY, "Localization");
    wxStaticBoxSizer *localeSizer = new wxStaticBoxSizer(localeBox, wxVERTICAL);
    wxPanel *localePanel = new wxPanel(localeBox);
    wxBoxSizer *localePanelSizer = new wxBoxSizer(wxVERTICAL);
    wxFlexGridSizer *localeGrid = new wxFlexGridSizer(4, 2, 15, 25);
    localeGrid->AddGrowableCol(1, 1);

    auto hostnameLabel = new wxStaticText(localePanel, wxID_ANY, "Hostname:");
    m_hostnameInput = new wxTextCtrl(localePanel, wxID_ANY, "");
    localeGrid->Add(hostnameLabel, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    localeGrid->Add(m_hostnameInput, 1, wxEXPAND);

    auto langLabel = new wxStaticText(localePanel, wxID_ANY, "Language:");
    m_languageChoice = new wxChoice(localePanel, wxID_ANY);
    std::vector<wxString> languages = {"en_US.UTF-8 (English - US)", "en_GB.UTF-8 (English - UK)", "bg_BG.UTF-8 (Bulgarian)",
                                       "es_ES.UTF-8 (Spanish)", "fr_FR.UTF-8 (French)", "de_DE.UTF-8 (German)"};
    m_languageChoice->Append(languages);
    localeGrid->Add(langLabel, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    localeGrid->Add(m_languageChoice, 1, wxEXPAND);

    auto kbdLabel = new wxStaticText(localePanel, wxID_ANY, "Keyboard Layout:");
    m_keyboardLayoutChoice = new wxChoice(localePanel, wxID_ANY);
    std::vector<wxString> layouts = {"us (US English)", "gb (UK English)", "bg (Bulgarian)", "es (Spanish)", "fr (French)", "de (German)"};
    m_keyboardLayoutChoice->Append(layouts);
    localeGrid->Add(kbdLabel, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    localeGrid->Add(m_keyboardLayoutChoice, 1, wxEXPAND);

    auto tzLabel = new wxStaticText(localePanel, wxID_ANY, "Timezone:");
    m_timezoneChoice = new wxChoice(localePanel, wxID_ANY);
    std::vector<wxString> timezones = {"UTC", "Europe/Sofia", "Europe/London", "Europe/Paris", "Europe/Berlin", "America/New_York", "Asia/Tokyo"};
    m_timezoneChoice->Append(timezones);
    localeGrid->Add(tzLabel, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    localeGrid->Add(m_timezoneChoice, 1, wxEXPAND);

    localePanelSizer->Add(localeGrid, 1, wxEXPAND | wxALL, 15);
    localePanel->SetSizer(localePanelSizer);
    localeSizer->Add(localePanel, 1, wxEXPAND);
    mainSizer->Add(localeSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 20);

    // --- Потребителски акаунт ---
    wxStaticBox *userBox = new wxStaticBox(this, wxID_ANY, "User Account (created if it does not exist)");
    wxStaticBoxSizer *userSizer = new wxStaticBoxSizer(userBox, wxVERTICAL);
    wxPanel *userPanel = new wxPanel(userBox);
    wxBoxSizer *userPanelSizer = new wxBoxSizer(wxVERTICAL);
    wxFlexGridSizer *userGrid = new wxFlexGridSizer(5, 2, 15, 25);
    userGrid->AddGrowableCol(1, 1);

    auto userLabel = new wxStaticText(userPanel, wxID_ANY, "Username:");
    m_usernameInput = new wxTextCtrl(userPanel, wxID_ANY, "");
    m_usernameInput->SetHint("Leave empty to skip");
    userGrid->Add(userLabel, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    userGrid->Add(m_usernameInput, 1, wxEXPAND);

    auto nameLabel = new wxStaticText(userPanel, wxID_ANY, "Full Name:");
    m_fullNameInput = new wxTextCtrl(userPanel, wxID_ANY, "");
    userGrid->Add(nameLabel, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    userGrid->Add(m_fullNameInput, 1, wxEXPAND);

    auto passLabel = new wxStaticText(userPanel, wxID_ANY, "Password:");
    m_userPasswordInput = new wxTextCtrl(userPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);
    m_userPasswordInput->SetHint("Leave empty to keep the current password");
    userGrid->Add(passLabel, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    userGrid->Add(m_userPasswordInput, 1, wxEXPAND);

    auto confirmPassLabel = new wxStaticText(userPanel, wxID_ANY, "Confirm Password:");
    m_confirmUserPasswordInput = new wxTextCtrl(userPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);
    userGrid->Add(confirmPassLabel, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    userGrid->Add(m_confirmUserPasswordInput, 1, wxEXPAND);

    m_adminCheckbox = new wxCheckBox(userPanel, wxID_ANY, " Make this user an administrator (sudo access)");
    m_adminCheckbox->SetValue(true);
    userGrid->AddSpacer(0);
    userGrid->Add(m_adminCheckbox, 0, wxALIGN_CENTER_VERTICAL | wxTOP, 10);

    userPanelSizer->Add(userGrid, 1, wxEXPAND | wxALL, 15);
    userPanel->SetSizer(userPanelSizer);
    userSizer->Add(userPanel, 1, wxEXPAND);
    mainSizer->Add(userSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 20);

    // --- Root акаунт ---
    wxStaticBox *rootBox = new wxStaticBox(this, wxID_ANY, "Root Account");
    wxStaticBoxSizer *rootSizer = new wxStaticBoxSizer(rootBox, wxVERTICAL);
    wxPanel *rootPanel = new wxPanel(rootBox);
    wxBoxSizer *rootPanelSizer = new wxBoxSizer(wxVERTICAL);

    m_lockRootCheckbox = new wxCheckBox(rootPanel, ID_LOCK_ROOT_CHECKBOX, " Lock the root account (recommended)");
    m_lockRootCheckbox->SetValue(true);
    rootPanelSizer->Add(m_lockRootCheckbox, 0, wxEXPAND | wxALL, 15);

    m_rootPasswordSizer = new wxFlexGridSizer(2, 2, 15, 25);
    m_rootPasswordSizer->AddGrowableCol(1, 1);
    m_rootPasswordLabel = new wxStaticText(rootPanel, wxID_ANY, "Root Password:");
    m_rootPasswordInput = new wxTextCtrl(rootPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);
    m_rootPasswordSizer->Add(m_rootPasswordLabel, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    m_rootPasswordSizer->Add(m_rootPasswordInput, 1, wxEXPAND);
    m_confirmRootPasswordLabel = new wxStaticText(rootPanel, wxID_ANY, "Confirm Root Password:");
    m_confirmRootPasswordInput = new wxTextCtrl(rootPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);
    m_rootPasswordSizer->Add(m_confirmRootPasswordLabel, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    m_rootPasswordSizer->Add(m_confirmRootPasswordInput, 1, wxEXPAND);
    rootPanelSizer->Add(m_rootPasswordSizer, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 15);

    rootPanel->SetSizer(rootPanelSizer);
    rootSizer->Add(rootPanel, 1, wxEXPAND);
    mainSizer->Add(rootSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 20);

    // --- Бутон за прилагане ---
    m_applyButton = new wxButton(this, ID_APPLY_SYSTEM_SETTINGS, "Apply to ISO", wxDefaultPosition, wxSize(160, 36));
    mainSizer->Add(m_applyButton, 0, wxALIGN_RIGHT | wxRIGHT | wxBOTTOM, 20);

    mainSizer->AddStretchSpacer(1);
    SetSizer(mainSizer);
    UpdateRootPasswordVisibility();
}

// Прилага тъмната цветова схема към контролите
void SystemTab::ApplyThemeColors()
{
    auto styleInput = [&](wxWindow *ctrl)
    {
        if (!ctrl)
            return;
        ctrl->SetBackgroundColour(m_inputBgColor);
        ctrl->SetForegroundColour(m_inputFgColor);
    };
    auto styleLabel = [&](wxWindow *ctrl)
    {
        if (!ctrl)
            return;
        ctrl->SetForegroundColour(m_labelColor);
        if (ctrl->GetParent())
            ctrl->SetBackgroundColour(ctrl->GetParent()->GetBackgroundColour());
    };

    // Всяка група е wxStaticBox с един вътрешен панел, в който са контролите
    for (wxWindow *child : GetChildren())
    {
        wxStaticBox *box = wxDynamicCast(child, wxStaticBox);
        if (!box)
            continue;
        box->SetForegroundColour(m_textColor);
        for (wxWindow *panelWindow : box->GetChildren())
        {
            wxPanel *panel = wxDynamicCast(panelWindow, wxPanel);
            if (!panel)
                continue;
            panel->SetBackgroundColour(m_panelBgColor);
            for (wxWindow *control : panel->GetChildren())
            {
                if (wxDynamicCast(control, wxTextCtrl) || wxDynamicCast(control, wxChoice))
                    styleInput(control);
                else if (wxDynamicCast(control, wxStaticText) || wxDynamicCast(control, wxCheckBox))
                    styleLabel(control);
            }
        }
    }

    m_applyButton->SetBackgroundColour(wxColour(137, 180, 250));
    m_applyButton->SetForegroundColour(wxColour(17, 17, 27));
    Layout();
    Refresh();
}

// Избира елемента, който започва с value; ако няма такъв, го добавя
void SystemTab::SelectByPrefix(wxChoice *choice, const wxString &value)
{
    if (value.IsEmpty())
    {
        if (choice->GetSelection() == wxNOT_FOUND)
            choice->SetSelection(0);
        return;
    }
    for (unsigned int i = 0; i < choice->GetCount(); i++)
    {
        if (choice->GetString(i).BeforeFirst(' ') == value)
        {
            choice->SetSelection(i);
            return;
        }
    }
    choice->SetSelection(choice->Append(value)); // Стойност от образа, която не е в списъка
}

wxString SystemTab::ChoiceValue(const wxChoice *choice)
{
    return choice->GetSelection() == wxNOT_FOUND ? wxString() : choice->GetStringSelection().BeforeFirst(' ');
}

// Чете текущите настройки от chroot средата на ISO образа
void SystemTab::LoadSettings()
{
    m_hostnameInput->SetValue("custom-linux");
    m_languageChoice->SetSelection(0);
    m_keyboardLayoutChoice->SetSelection(0);
    m_timezoneChoice->SetSelection(0);

    wxString containerId = ContainerCommand::ReadContainerId(m_projectDir);
    if (containerId.IsEmpty()) // Контейнерът още не е готов - остават стойностите по подразбиране
        return;

    wxArrayString output;
    {
        wxBusyCursor busy;
        ContainerCommand::RunInChroot(containerId,
                                      "echo \"hostname=$(cat /etc/hostname 2>/dev/null)\"\n"
                                      "echo \"lang=$(cat /etc/default/locale /etc/locale.conf 2>/dev/null | grep '^LANG=' | head -n1 | cut -d= -f2 | tr -d '\\\"')\"\n"
                                      "echo \"keyboard=$(grep -s '^XKBLAYOUT=' /etc/default/keyboard | head -n1 | cut -d= -f2 | tr -d '\\\"' | cut -d, -f1)\"\n"
                                      "TZ_NAME=$(cat /etc/timezone 2>/dev/null)\n"
                                      "[ -z \"$TZ_NAME\" ] && TZ_NAME=$(readlink /etc/localtime 2>/dev/null | sed 's#.*/zoneinfo/##')\n"
                                      "echo \"timezone=$TZ_NAME\"\n"
                                      "echo \"rootstatus=$(passwd -S root 2>/dev/null | awk '{print $2}')\"\n",
                                      &output);
    }

    for (const wxString &line : output) // Редове във формат ключ=стойност
    {
        wxString key = line.BeforeFirst('=');
        wxString value = line.AfterFirst('=');
        value.Trim(true).Trim(false);
        if (key == "hostname" && !value.IsEmpty())
            m_hostnameInput->SetValue(value);
        else if (key == "lang")
            SelectByPrefix(m_languageChoice, value);
        else if (key == "keyboard")
            SelectByPrefix(m_keyboardLayoutChoice, value);
        else if (key == "timezone")
            SelectByPrefix(m_timezoneChoice, value);
        else if (key == "rootstatus" && !value.IsEmpty())
            m_lockRootCheckbox->SetValue(value != "P"); // P = има парола (отключен); L/NP = заключен
    }
    UpdateRootPasswordVisibility();
}

void SystemTab::UpdateRootPasswordVisibility()
{
    bool showPasswords = !m_lockRootCheckbox->GetValue();
    m_rootPasswordSizer->ShowItems(showPasswords);
    if (!showPasswords)
    {
        m_rootPasswordInput->Clear();
        m_confirmRootPasswordInput->Clear();
    }
    Layout();
}

void SystemTab::OnLockRootCheck(wxCommandEvent &event)
{
    UpdateRootPasswordVisibility();
}

// Проверява въведените данни преди прилагане
bool SystemTab::ValidateInput(wxString &error) const
{
    wxString hostname = m_hostnameInput->GetValue();
    if (!wxRegEx("^[A-Za-z0-9]([A-Za-z0-9-]{0,61}[A-Za-z0-9])?$").Matches(hostname))
    {
        error = "The hostname may contain only letters, digits and '-' (max 63 characters).";
        return false;
    }

    wxString username = m_usernameInput->GetValue();
    if (!username.IsEmpty() && !wxRegEx("^[a-z_][a-z0-9_-]{0,31}$").Matches(username))
    {
        error = "The username must start with a lowercase letter and contain only a-z, 0-9, '_' or '-'.";
        return false;
    }
    if (m_userPasswordInput->GetValue() != m_confirmUserPasswordInput->GetValue())
    {
        error = "The user passwords do not match.";
        return false;
    }
    if (!m_lockRootCheckbox->GetValue() && m_rootPasswordInput->GetValue() != m_confirmRootPasswordInput->GetValue())
    {
        error = "The root passwords do not match.";
        return false;
    }
    return true;
}

// Създава bash скрипт, който прилага настройките в chroot средата
wxString SystemTab::BuildApplyScript() const
{
    using ContainerCommand::ShellQuote;
    wxString script = "set -e\n";

    // Hostname
    wxString hostname = m_hostnameInput->GetValue();
    script += "echo " + ShellQuote(hostname) + " > /etc/hostname\n";
    script += "if grep -q '^127.0.1.1' /etc/hosts 2>/dev/null; then sed -i \"s/^127.0.1.1.*/127.0.1.1\\t$(cat /etc/hostname)/\" /etc/hosts; "
              "else printf '127.0.1.1\\t%s\\n' \"$(cat /etc/hostname)\" >> /etc/hosts; fi\n";

    // Език
    wxString lang = ChoiceValue(m_languageChoice);
    if (!lang.IsEmpty())
    {
        script += "LANG_VALUE=" + ShellQuote(lang) + "\n";
        script += "if [ -f /etc/locale.gen ]; then sed -i \"s/^# *\\($LANG_VALUE\\)/\\1/\" /etc/locale.gen; "
                  "grep -q \"^$LANG_VALUE\" /etc/locale.gen || echo \"$LANG_VALUE UTF-8\" >> /etc/locale.gen; fi\n";
        script += "if command -v locale-gen >/dev/null 2>&1; then locale-gen || true; fi\n";
        script += "echo \"LANG=$LANG_VALUE\" > /etc/default/locale\n";
        script += "echo \"LANG=$LANG_VALUE\" > /etc/locale.conf\n";
    }

    // Клавиатурна подредба
    wxString keyboard = ChoiceValue(m_keyboardLayoutChoice);
    if (!keyboard.IsEmpty())
    {
        script += "KB_VALUE=" + ShellQuote(keyboard) + "\n";
        script += "if [ -f /etc/default/keyboard ]; then sed -i \"s/^XKBLAYOUT=.*/XKBLAYOUT=\\\"$KB_VALUE\\\"/\" /etc/default/keyboard; "
                  "else printf 'XKBLAYOUT=\"%s\"\\n' \"$KB_VALUE\" > /etc/default/keyboard; fi\n";
        script += "echo \"KEYMAP=$KB_VALUE\" > /etc/vconsole.conf\n";
    }

    // Часова зона
    wxString timezone = ChoiceValue(m_timezoneChoice);
    if (!timezone.IsEmpty())
    {
        script += "TZ_VALUE=" + ShellQuote(timezone) + "\n";
        script += "if [ -f \"/usr/share/zoneinfo/$TZ_VALUE\" ]; then ln -sf \"/usr/share/zoneinfo/$TZ_VALUE\" /etc/localtime; echo \"$TZ_VALUE\" > /etc/timezone; "
                  "else echo \"Timezone $TZ_VALUE not found in the image\" >&2; fi\n";
    }

    // Потребител
    wxString username = m_usernameInput->GetValue();
    if (!username.IsEmpty())
    {
        script += "USER_NAME=" + ShellQuote(username) + "\n";
        script += "FULL_NAME=" + ShellQuote(m_fullNameInput->GetValue()) + "\n";
        script += "if id -u \"$USER_NAME\" >/dev/null 2>&1; then usermod -c \"$FULL_NAME\" \"$USER_NAME\"; "
                  "else useradd -m -s /bin/bash -c \"$FULL_NAME\" \"$USER_NAME\"; fi\n";
        if (!m_userPasswordInput->GetValue().IsEmpty())
            script += "printf '%s:%s\\n' \"$USER_NAME\" " + ShellQuote(m_userPasswordInput->GetValue()) + " | chpasswd\n";
        script += "if getent group sudo >/dev/null; then ADMIN_GROUP=sudo; else ADMIN_GROUP=wheel; fi\n";
        if (m_adminCheckbox->GetValue())
            script += "getent group \"$ADMIN_GROUP\" >/dev/null && usermod -aG \"$ADMIN_GROUP\" \"$USER_NAME\"\n";
        else
            script += "gpasswd -d \"$USER_NAME\" \"$ADMIN_GROUP\" >/dev/null 2>&1 || true\n";
    }

    // Root акаунт
    if (m_lockRootCheckbox->GetValue())
    {
        script += "passwd -l root\n";
    }
    else
    {
        if (!m_rootPasswordInput->GetValue().IsEmpty())
            script += "printf 'root:%s\\n' " + ShellQuote(m_rootPasswordInput->GetValue()) + " | chpasswd\n";
        script += "passwd -u root || echo 'Set a root password to unlock the root account.' >&2\n";
    }

    script += "echo 'System settings applied.'\n";
    return script;
}

// Записва настройките в ISO образа
void SystemTab::OnApply(wxCommandEvent &event)
{
    wxString error;
    if (!ValidateInput(error))
    {
        wxMessageBox(error, "Invalid Settings", wxOK | wxICON_WARNING, this);
        return;
    }

    wxString containerId = ContainerCommand::ReadContainerId(m_projectDir);
    if (containerId.IsEmpty())
    {
        wxMessageBox("The Docker container is not ready yet. Wait until the initial setup has finished.",
                     "Container Not Ready", wxOK | wxICON_INFORMATION, this);
        return;
    }

    wxArrayString output;
    long exitCode;
    {
        wxBusyCursor busy;
        m_applyButton->Disable();
        exitCode = ContainerCommand::RunInChroot(containerId, BuildApplyScript(), &output);
        m_applyButton->Enable();
    }

    if (exitCode == 0)
    {
        m_userPasswordInput->Clear(); // Паролите не се пазят в интерфейса
        m_confirmUserPasswordInput->Clear();
        m_rootPasswordInput->Clear();
        m_confirmRootPasswordInput->Clear();
        wxMessageBox("The system settings were applied to the ISO.", "Settings Applied", wxOK | wxICON_INFORMATION, this);
    }
    else
    {
        // Показва последните редове от изхода, за да се види коя команда е неуспешна
        wxArrayString tail;
        for (size_t i = output.GetCount() > 12 ? output.GetCount() - 12 : 0; i < output.GetCount(); i++)
            tail.Add(output[i]);
        wxMessageBox(wxString::Format("Applying the settings failed (exit code %ld).\n\n%s", exitCode, wxJoin(tail, '\n')),
                     "Settings Not Applied", wxOK | wxICON_ERROR, this);
    }
}
