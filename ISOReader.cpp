#include "ISOReader.h"     // Включва заглавния файл "ISOReader.h" за дефиницията на класа ISOReader.
#include <archive.h>       // Включва заглавния файл <archive.h> за работа с архиви (libarchive).
#include <archive_entry.h> // Включва заглавния файл <archive_entry.h> за работа със записи в архива (libarchive).
#include <algorithm>       // Включва заглавния файл <algorithm> за стандартни алгоритми като std::replace.
#include <stdexcept>       // Включва заглавния файл <stdexcept> за стандартни изключения.

#ifdef _WIN32               // Условна компилация: ако е дефиниран _WIN32 (за Windows системи).
#include <windows.h>        // Включва заглавния файл <windows.h> за специфични за Windows функции.
#define PATH_SEPARATOR "\\" // Дефинира макроса PATH_SEPARATOR като обратна наклонена черта за Windows.
// Дефинира ssize_t за Windows, тъй като може да не е наличен стандартно.
#ifdef _WIN64              // Условна компилация: ако е 64-битова Windows система.
typedef __int64 ssize_t;   // Дефинира типа ssize_t като __int64 (64-битово цяло число).
#else                      // Иначе (ако е 32-битова Windows система).
typedef long ssize_t; // Дефинира типа ssize_t като long (обикновено 32-битово цяло число).
#endif                     // Край на условната компилация за _WIN64.
#else                      // Иначе (ако не се компилира за Windows, т.е. за Unix-подобни системи).
#include <unistd.h>        // Включва заглавния файл <unistd.h> за POSIX API функции.
#define PATH_SEPARATOR "/" // Дефинира макроса PATH_SEPARATOR като наклонена черта за Unix-подобни системи.
#endif                     // Край на условната компилация за _WIN32.

// Конструктор на класа ISOReader.
ISOReader::ISOReader(const std::string &path) // Приема път до ISO файла като константна референция към std::string.
    : iso_path(normalizePath(path)), archive(nullptr)
{
} // Инициализира iso_path с нормализирания път (извиквайки normalizePath) и archive с nullptr. Тялото на конструктора е празно.

// Деструктор на класа ISOReader.
ISOReader::~ISOReader()
{            // Начало на тялото на деструктора.
    close(); // Извиква метода close() за освобождаване на ресурсите.
} // Край на тялото на деструктора.

// Конструктор за преместване.
ISOReader::ISOReader(ISOReader &&other) noexcept // Приема r-стойностна референция към друг ISOReader обект. noexcept указва, че конструкторът не хвърля изключения.
    : iso_path(std::move(other.iso_path))        // Премества стойността на iso_path от 'other' в текущия обект.
      ,
      archive(other.archive) // Копира указателя 'archive' от 'other'.
      ,
      last_error(std::move(other.last_error))
{                            // Премества стойността на last_error от 'other' и начало на тялото на конструктора за преместване.
    other.archive = nullptr; // Нулира указателя 'archive' в изходния обект 'other', за да се избегне двойно освобождаване.
} // Край на тялото на конструктора за преместване.

// Оператор за присвояване чрез преместване.
ISOReader &ISOReader::operator=(ISOReader &&other) noexcept
{ // Приема r-стойностна референция към друг ISOReader обект. noexcept указва, че операторът не хвърля изключения. Начало на тялото на оператора.
    if (this != &other)
    {                                             // Проверява за самоприсвояване (дали обектът не се присвоява сам на себе си).
        close();                                  // Затваря текущия архив, ако е отворен, за да освободи ресурсите.
        iso_path = std::move(other.iso_path);     // Премества стойността на iso_path от 'other'.
        archive = other.archive;                  // Копира указателя 'archive' от 'other'.
        last_error = std::move(other.last_error); // Премества стойността на last_error от 'other'.
        other.archive = nullptr;                  // Нулира указателя 'archive' в изходния обект 'other'.
    } // Край на условния блок 'if'.
    return *this; // Връща референция към текущия обект (*this).
} // Край на тялото на оператора за присвояване чрез преместване.

// Метод за отваряне на ISO файла.
bool ISOReader::open()
{                                 // Връща true при успех, false при грешка. Начало на тялото на метода.
    close();                      // Първо затваря всякакъв предишно отворен архив.
    archive = archive_read_new(); // Създава нова структура за четене на архив и присвоява указателя на 'archive'.
    if (!archive)
    {                                                   // Проверява дали създаването на структурата е неуспешно (archive е nullptr).
        last_error = "Failed to create archive struct"; // Записва съобщение за грешка (оригиналният английски текст е запазен).
        return false;                                   // Връща false, индикирайки неуспех.
    } // Край на условния блок 'if'.

    archive_read_support_format_iso9660(archive); // Активира поддръжката за ISO9660 формат в архива.

    int r;                                                               // Декларира променлива 'r' за съхранение на кода за резултат от операциите с архива.
#ifdef _WIN32                                                            // Условна компилация: ако се компилира за Windows.
    std::wstring wide_path = stringToWide(iso_path);                     // Конвертира пътя до файла (iso_path) в широк низ (wstring).
    r = archive_read_open_filename_w(archive, wide_path.c_str(), 10240); // Отваря архива, използвайки версията на функцията за широки низове; 10240 е размер на блока.
#else                                                                    // Иначе (за не-Windows системи).
    r = archive_read_open_filename(archive, iso_path.c_str(), 10240); // Отваря архива, използвайки стандартната версия на функцията; 10240 е размер на блока.
#endif                                                                   // Край на условната компилация за _WIN32.

    if (r != ARCHIVE_OK)
    {                                               // Проверява дали отварянето на архива е неуспешно (резултатът 'r' не е ARCHIVE_OK).
        last_error = archive_error_string(archive); // Получава и записва съобщението за грешка от libarchive (оригиналният текст от библиотеката е запазен).
        close();                                    // Затваря и освобождава ресурсите на архива.
        return false;                               // Връща false, индикирайки неуспех.
    } // Край на условния блок 'if'.
    return true; // Връща true, ако отварянето е успешно.
} // Край на тялото на метода open().

// Метод за затваряне на ISO файла и освобождаване на ресурсите.
void ISOReader::close()
{ // Няма връщана стойност. Начало на тялото на метода.
    if (archive)
    {                                // Проверява дали указателят 'archive' е валиден (т.е. архивът е бил отворен).
        archive_read_close(archive); // Затваря архива за четене.
        archive_read_free(archive);  // Освобождава паметта, заета от структурата на архива.
        archive = nullptr;           // Нулира указателя 'archive', за да покаже, че архивът вече не е отворен.
    } // Край на условния блок 'if'.
} // Край на тялото на метода close().

// Метод за извличане на списък с всички файлове в ISO архива.
std::vector<std::string> ISOReader::listFiles()
{                                   // Връща вектор от низове (имената на файловете). Начало на тялото на метода.
    std::vector<std::string> files; // Инициализира празен вектор 'files' за съхранение на имената на файловете.
    if (!archive)
    {                                      // Проверява дали архивът не е отворен.
        last_error = "Archive not opened"; // Записва съобщение за грешка (оригиналният английски текст е запазен).
        return files;                      // Връща празния вектор 'files'.
    } // Край на условния блок 'if'.

    struct archive_entry *entry; // Декларира указател 'entry' към структура archive_entry (запис в архива).
    while (archive_read_next_header(archive, &entry) == ARCHIVE_OK)
    {                                                                         // Цикъл: итерира през всички записи (headers) в архива, докато има такива и няма грешки.
#ifdef _WIN32                                                                 // Условна компилация: ако се компилира за Windows.
        std::string pathname = wideToString(archive_entry_pathname_w(entry)); // Получава името на пътя на записа като широк низ и го конвертира към std::string.
#else                                                                         // Иначе (за не-Windows системи).
        std::string pathname = archive_entry_pathname(entry); // Получава името на пътя на записа като std::string.
#endif                                                                        // Край на условната компилация за _WIN32.
        files.push_back(normalizePath(pathname));                             // Нормализира получения път и го добавя към вектора 'files'.
        archive_read_data_skip(archive);                                      // Пропуска данните на текущия запис (тъй като ни интересуват само имената).
    } // Край на цикъла 'while'.

    // Нулира позицията в архива чрез затваряне и повторно отваряне.
    close();      // Затваря архива.
    open();       // Отваря архива отново, за да нулира позицията на четене до началото.
    return files; // Връща вектора 'files' с имената на файловете.
} // Край на тялото на метода listFiles().

// Метод за четене на съдържанието на конкретен файл от ISO архива.
bool ISOReader::readFile(const std::string &filename, std::vector<char> &content)
{ // Приема име на файл и референция към вектор от символи за съдържанието. Връща true при успех. Начало на тялото на метода.
    if (!archive)
    {                                      // Проверява дали архивът не е отворен.
        last_error = "Archive not opened"; // Записва съобщение за грешка (оригиналният английски текст е запазен).
        return false;                      // Връща false.
    } // Край на условния блок 'if'.

    struct archive_entry *entry;                               // Декларира указател 'entry' към структура archive_entry.
    std::string normalized_filename = normalizePath(filename); // Нормализира името на търсения файл.

    while (archive_read_next_header(archive, &entry) == ARCHIVE_OK)
    {                                                                 // Цикъл: итерира през всички записи в архива.
        std::string current_file;                                     // Декларира променлива 'current_file' за името на текущия файл в итерацията.
#ifdef _WIN32                                                         // Условна компилация: ако се компилира за Windows.
        current_file = wideToString(archive_entry_pathname_w(entry)); // Получава името на текущия запис (широк низ) и го конвертира.
#else                                                                 // Иначе (за не-Windows системи).
        current_file = archive_entry_pathname(entry); // Получава името на текущия запис.
#endif                                                                // Край на условната компилация за _WIN32.
        current_file = normalizePath(current_file);                   // Нормализира името на текущия файл.

        if (normalized_filename == current_file)
        {                                                                       // Проверява дали нормализираното име на търсения файл съвпада с нормализираното име на текущия файл.
            const size_t size = static_cast<size_t>(archive_entry_size(entry)); // Получава размера на файла като size_t.
            content.resize(size);                                               // Преоразмерява вектора 'content', за да побере данните от файла.

            const ssize_t read_size = archive_read_data(archive, content.data(), size); // Чете данните от файла в буфера 'content.data()'. 'read_size' е броят прочетени байтове.

            // Нулира позицията в архива чрез затваряне и повторно отваряне.
            close(); // Затваря архива.
            open();  // Отваря архива отново, за да нулира позицията.

            if (read_size < 0 || static_cast<size_t>(read_size) != size)
            {                                                // Проверява дали е възникнала грешка при четене (read_size < 0) или не са прочетени всички данни.
                last_error = "Failed to read complete file"; // Записва съобщение за грешка (оригиналният английски текст е запазен).
                return false;                                // Връща false.
            } // Край на условния блок 'if'.
            return true; // Връща true, ако файлът е прочетен успешно.
        } // Край на условния блок 'if' (за сравнение на имената на файловете).
        archive_read_data_skip(archive); // Пропуска данните на текущия запис, ако не е търсеният файл.
    } // Край на цикъла 'while'.

    last_error = "File not found in archive"; // Записва съобщение за грешка (оригиналният английски текст е запазен).
    // Нулира позицията в архива чрез затваряне и повторно отваряне (след неуспешно търсене).
    close();      // Затваря архива.
    open();       // Отваря архива отново.
    return false; // Връща false, тъй като файлът не е намерен.
} // Край на тялото на метода readFile().

// Метод за получаване на последното съобщение за грешка.
std::string ISOReader::getLastError() const
{                      // Връща низ с последната грешка. 'const' показва, че методът не променя състоянието на обекта. Начало на тялото на метода.
    return last_error; // Връща съхраненото съобщение за грешка 'last_error'.
} // Край на тялото на метода getLastError().

// Помощен метод за нормализиране на пътища (заменя наклонените черти според операционната система).
std::string ISOReader::normalizePath(const std::string &path) const
{                                                                  // Приема път като низ и връща нормализиран път. 'const' метод. Начало на тялото на метода.
    std::string normalized = path;                                 // Копира входния път 'path' в променливата 'normalized'.
#ifdef _WIN32                                                      // Условна компилация: ако се компилира за Windows.
    std::replace(normalized.begin(), normalized.end(), '/', '\\'); // Заменя всички символи '/' с '\' в низа 'normalized'.
#else                                                              // Иначе (за не-Windows системи).
    std::replace(normalized.begin(), normalized.end(), '\\', '/'); // Заменя всички символи '\' с '/' в низа 'normalized'.
#endif                                                             // Край на условната компилация за _WIN32.
    return normalized;                                             // Връща нормализирания низ 'normalized'.
} // Край на тялото на метода normalizePath().

#ifdef _WIN32 // Условна компилация: следващите методи са дефинирани само при компилация за Windows.
// Помощен метод за конвертиране на широк низ (wchar_t*) към std::string (UTF-8).
std::string ISOReader::wideToString(const wchar_t *wide) const
{ // Приема указател към широк низ. 'const' метод. Начало на тялото на метода.
    if (!wide)
        return "";                                                                      // Ако указателят 'wide' е нулев, връща празен низ.
    int size = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr); // Изчислява необходимия размер на буфера за UTF-8 низа. -1 означава, че 'wide' е нул-терминиран.
    if (size == 0)
        return "";                                                                    // Ако изчисленият размер е 0 (грешка или празен входен низ), връща празен низ.
    std::vector<char> buffer(size);                                                   // Създава буфер (вектор от char) с изчисления размер.
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, buffer.data(), size, nullptr, nullptr); // Конвертира широкия низ в UTF-8 и го записва в буфера.
    return std::string(buffer.data());                                                // Създава и връща std::string от данните в буфера (които са нул-терминирани от WideCharToMultiByte).
} // Край на тялото на метода wideToString().

// Помощен метод за конвертиране на std::string (UTF-8) към широк низ (std::wstring).
std::wstring ISOReader::stringToWide(const std::string &str) const
{ // Приема std::string. 'const' метод. Начало на тялото на метода.
    if (str.empty())
        return L"";                                                          // Ако входният низ 'str' е празен, връща празен широк низ (L"").
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0); // Изчислява необходимия размер на буфера за широкия низ. -1 означава, че str.c_str() е нул-терминиран.
    if (size == 0)
        return L"";                                                        // Ако изчисленият размер е 0 (грешка или празен входен низ), връща празен широк низ.
    std::vector<wchar_t> buffer(size);                                     // Създава буфер (вектор от wchar_t) с изчисления размер.
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, buffer.data(), size); // Конвертира UTF-8 низа в широк низ и го записва в буфера.
    return std::wstring(buffer.data());                                    // Създава и връща std::wstring от данните в буфера (които са нул-терминирани от MultiByteToWideChar).
} // Край на тялото на метода stringToWide().
#endif // Край на условната компилация за _WIN32 (за помощните методи за конвертиране на низове).