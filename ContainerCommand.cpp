#include "ContainerCommand.h" // Декларации на помощните функции
#include <wx/filename.h>      // wxFileName за пътища и временни файлове
#include <wx/file.h>          // wxFile за четене/запис на файлове
#include <wx/process.h>       // wxProcess за асинхронни процеси
#include <wx/utils.h>         // wxExecute
#include <wx/log.h>           // wxLogDebug / wxLogError

namespace ContainerCommand
{
    const char *const CHROOT_PATH = "/root/custom_iso/squashfs-root"; // Същият път, който използват setup_chroot.sh и create_iso.sh
}

namespace
{
    // wxProcess, който извиква callback при приключване на процеса и след това се самоизтрива
    class CallbackProcess : public wxProcess
    {
    public:
        explicit CallbackProcess(std::function<void(int)> onDone) : m_onDone(std::move(onDone)) {}

        void OnTerminate(int WXUNUSED(pid), int status) override
        {
            if (m_onDone)        // Ако има зададен callback
                m_onDone(status); // Подава exit кода на процеса
            delete this;         // Обектът е създаден с new и никой друг не го държи
        }

    private:
        std::function<void(int)> m_onDone; // Функция, която се извиква при приключване
    };

    // Записва скрипта във временен файл, копира го в /tmp на chroot средата и връща пътя му вътре в chroot.
    bool CopyScriptIntoChroot(const wxString &containerId, const wxString &script, wxString &chrootScriptPath)
    {
        wxString localPath = wxFileName::CreateTempFileName("linuxisopro"); // Създава уникален временен файл
        if (localPath.IsEmpty())
        {
            wxLogError("Could not create a temporary file for the container script.");
            return false;
        }

        // Prologue: скриптът изтрива себе си и не задава интерактивни въпроси (apt-get и др.)
        wxString content = "#!/bin/bash\nrm -f -- \"$0\"\nexport DEBIAN_FRONTEND=noninteractive\n" + script + "\n";
        content.Replace("\r\n", "\n"); // bash не приема Windows редове

        wxFile file;
        if (!file.Open(localPath, wxFile::write) || !file.Write(content, wxConvUTF8))
        {
            wxLogError("Could not write the temporary container script: %s", localPath);
            wxRemoveFile(localPath);
            return false;
        }
        file.Close();

        chrootScriptPath = "/tmp/" + wxFileName(localPath).GetName() + ".sh"; // Път вътре в chroot средата
        wxString copyCommand = wxString::Format("docker cp \"%s\" %s:%s%s",
                                                localPath, containerId, ContainerCommand::CHROOT_PATH, chrootScriptPath);
        wxArrayString output, errors;
        long exitCode = wxExecute(copyCommand, output, errors, wxEXEC_SYNC | wxEXEC_HIDE_CONSOLE); // Копира файла в контейнера
        wxRemoveFile(localPath); // Локалното копие вече не е нужно

        if (exitCode != 0)
        {
            wxLogError("docker cp failed (%ld): %s", exitCode, wxJoin(errors, '\n'));
            return false;
        }
        return true;
    }

    // Команда, която изпълнява вече копирания скрипт в chroot средата
    wxString ChrootExecCommand(const wxString &containerId, const wxString &chrootScriptPath)
    {
        return wxString::Format("docker exec %s chroot %s /bin/bash %s",
                                containerId, ContainerCommand::CHROOT_PATH, chrootScriptPath);
    }
}

namespace ContainerCommand
{
    wxString ReadContainerId(const wxString &projectDir)
    {
        if (projectDir.IsEmpty())
            return wxEmptyString;

        wxString path = wxFileName(projectDir, "container_id.txt").GetFullPath(); // script.exe записва ID-то тук
        if (!wxFileExists(path))
            return wxEmptyString;

        wxFile file(path);
        wxString containerId;
        if (!file.IsOpened() || !file.ReadAll(&containerId))
            return wxEmptyString;
        return containerId.Trim(true).Trim(false);
    }

    wxString ShellQuote(const wxString &value)
    {
        wxString escaped = value;
        escaped.Replace("'", "'\\''"); // Затваря кавичката, добавя екранирана ', отваря отново
        return "'" + escaped + "'";
    }

    long RunInChroot(const wxString &containerId, const wxString &script, wxArrayString *output)
    {
        if (containerId.IsEmpty())
            return -1;

        wxString chrootScriptPath;
        if (!CopyScriptIntoChroot(containerId, script, chrootScriptPath))
            return -1;

        wxArrayString stdoutLines, stderrLines;
        long exitCode = wxExecute(ChrootExecCommand(containerId, chrootScriptPath), stdoutLines, stderrLines,
                                  wxEXEC_SYNC | wxEXEC_HIDE_CONSOLE);
        if (output)
        {
            *output = stdoutLines;                   // Стандартният изход
            for (const wxString &line : stderrLines) // и грешките, за да се покажат на потребителя
                output->Add(line);
        }
        wxLogDebug("ContainerCommand::RunInChroot exit code: %ld", exitCode);
        return exitCode;
    }

    long RunInChrootAsync(const wxString &containerId, const wxString &script, std::function<void(int)> onDone)
    {
        if (containerId.IsEmpty())
            return 0;

        wxString chrootScriptPath;
        if (!CopyScriptIntoChroot(containerId, script, chrootScriptPath))
            return 0;

        return RunInContainerAsync(containerId, wxString::Format("chroot %s /bin/bash %s", CHROOT_PATH, chrootScriptPath),
                                   std::move(onDone));
    }

    long RunInContainerAsync(const wxString &containerId, const wxString &command, std::function<void(int)> onDone)
    {
        if (containerId.IsEmpty())
            return 0;
        return RunAsync(wxString::Format("docker exec %s %s", containerId, command), std::move(onDone));
    }

    bool IsValidFlatpakId(const wxString &appId)
    {
        if (appId.IsEmpty() || !appId.Contains("."))
            return false;
        for (wxUniChar c : appId) // Позволени са само ASCII символите, които Flatpak допуска в ID
        {
            bool allowed = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
                           c == '.' || c == '_' || c == '-';
            if (!allowed)
                return false;
        }
        return true;
    }

    wxString FlatpakInstallScript(const wxArrayString &appIds)
    {
        wxString apps;
        for (const wxString &appId : appIds)
            apps += " " + ShellQuote(appId);

        return wxString::Format(
            "LOG=/tmp/linuxisopro_flatpak.log\n"
            ": > \"$LOG\"\n"
            "if ! command -v flatpak >/dev/null 2>&1; then\n"
            "    echo \"flatpak is not installed in this image\" >> \"$LOG\"\n"
            "    exit %d\n"
            "fi\n"
            "flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo >> \"$LOG\" 2>&1\n"
            "failed=0\n"
            "for app in%s; do\n"
            "    echo \"== Installing $app\" >> \"$LOG\"\n"
            "    flatpak install -y --noninteractive flathub \"$app\" >> \"$LOG\" 2>&1 || failed=$((failed+1))\n"
            "done\n"
            "exit $failed\n",
            FLATPAK_MISSING_EXIT_CODE, apps);
    }

    wxString FlatpakInstallLogTail(const wxString &containerId, int lines)
    {
        wxString command = wxString::Format("docker exec %s tail -n %d %s/tmp/linuxisopro_flatpak.log",
                                            containerId, lines, CHROOT_PATH);
        wxArrayString output, errors;
        if (wxExecute(command, output, errors, wxEXEC_SYNC | wxEXEC_HIDE_CONSOLE) != 0)
            return wxEmptyString;
        return wxJoin(output, '\n');
    }

    long RunAsync(const wxString &command, std::function<void(int)> onDone)
    {
        CallbackProcess *process = new CallbackProcess(std::move(onDone));
        long pid = wxExecute(command, wxEXEC_ASYNC | wxEXEC_HIDE_CONSOLE, process);
        if (pid == 0)
        {
            wxLogError("Failed to start: %s", command);
            delete process; // При неуспех wxExecute не поема обекта
        }
        return pid;
    }
}
