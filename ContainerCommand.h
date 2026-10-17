#ifndef CONTAINER_COMMAND_H // Предпазител от многократно включване на хедър файла
#define CONTAINER_COMMAND_H // Дефиниране на предпазителя

#include <wx/string.h> // wxString
#include <wx/arrstr.h> // wxArrayString
#include <functional>  // std::function за callback при асинхронно изпълнение

// Помощни функции за изпълнение на команди в chroot средата, която script.exe подготвя в Docker контейнера.
// Скриптовете се копират като файл в контейнера (docker cp), за да няма проблеми с кавички и специални символи.
namespace ContainerCommand
{
    // Път до разархивираната файлова система на ISO образа вътре в контейнера
    extern const char *const CHROOT_PATH;

    // Чете ID-то на контейнера от <projectDir>/container_id.txt (празен низ, ако файлът липсва)
    wxString ReadContainerId(const wxString &projectDir);

    // Огражда стойност в единични кавички за безопасно използване в bash скрипт
    wxString ShellQuote(const wxString &value);

    // Изпълнява bash скрипт в chroot средата и изчаква края му. Връща exit кода (-1, ако не може да стартира).
    long RunInChroot(const wxString &containerId, const wxString &script, wxArrayString *output = nullptr);

    // Стартира bash скрипт в chroot средата без да блокира интерфейса.
    // onDone(exitCode) се извиква в главната нишка, когато скриптът приключи. Връща PID на процеса (0 при грешка).
    long RunInChrootAsync(const wxString &containerId, const wxString &script, std::function<void(int)> onDone);

    // Стартира команда директно в контейнера (извън chroot), без да блокира интерфейса.
    long RunInContainerAsync(const wxString &containerId, const wxString &command, std::function<void(int)> onDone);

    // Стартира произволна команда (напр. "docker rm -f <id>") без да блокира интерфейса.
    long RunAsync(const wxString &command, std::function<void(int)> onDone);

    // Проверява дали низът е валидно Flatpak ID (напр. org.mozilla.firefox)
    bool IsValidFlatpakId(const wxString &appId);

    // Bash скрипт, който инсталира приложенията от Flathub в chroot средата.
    // Exit кодът е броят на неуспешните инсталации (FLATPAK_MISSING_EXIT_CODE, ако flatpak липсва в образа).
    wxString FlatpakInstallScript(const wxArrayString &appIds);
    const int FLATPAK_MISSING_EXIT_CODE = 100;

    // Последните редове от лога на FlatpakInstallScript (за съобщения за грешки)
    wxString FlatpakInstallLogTail(const wxString &containerId, int lines = 15);
}

#endif // CONTAINER_COMMAND_H
