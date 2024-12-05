#include "ISOReader.h"
#include <archive.h>
#include <archive_entry.h>
#include <algorithm>
#include <stdexcept>

#ifdef _WIN32
    #include <windows.h>
    #define PATH_SEPARATOR "\\"
    // Define ssize_t for Windows
    #ifdef _WIN64
        typedef __int64 ssize_t;
    #else
        typedef long ssize_t;
    #endif
#else
    #include <unistd.h>
    #define PATH_SEPARATOR "/"
#endif

ISOReader::ISOReader(const std::string& path)
    : iso_path(normalizePath(path)), archive(nullptr) {}

ISOReader::~ISOReader() {
    close();
}

ISOReader::ISOReader(ISOReader&& other) noexcept
    : iso_path(std::move(other.iso_path))
    , archive(other.archive)
    , last_error(std::move(other.last_error)) {
    other.archive = nullptr;
}

ISOReader& ISOReader::operator=(ISOReader&& other) noexcept {
    if (this != &other) {
        close();
        iso_path = std::move(other.iso_path);
        archive = other.archive;
        last_error = std::move(other.last_error);
        other.archive = nullptr;
    }
    return *this;
}

bool ISOReader::open() {
    close();
    archive = archive_read_new();
    if (!archive) {
        last_error = "Failed to create archive struct";
        return false;
    }

    archive_read_support_format_iso9660(archive);
    
    int r;
    #ifdef _WIN32
        std::wstring wide_path = stringToWide(iso_path);
        r = archive_read_open_filename_w(archive, wide_path.c_str(), 10240);
    #else
        r = archive_read_open_filename(archive, iso_path.c_str(), 10240);
    #endif

    if (r != ARCHIVE_OK) {
        last_error = archive_error_string(archive);
        close();
        return false;
    }
    return true;
}

void ISOReader::close() {
    if (archive) {
        archive_read_close(archive);
        archive_read_free(archive);
        archive = nullptr;
    }
}

std::vector<std::string> ISOReader::listFiles() {
    std::vector<std::string> files;
    if (!archive) {
        last_error = "Archive not opened";
        return files;
    }

    struct archive_entry* entry;
    while (archive_read_next_header(archive, &entry) == ARCHIVE_OK) {
        #ifdef _WIN32
            std::string pathname = wideToString(archive_entry_pathname_w(entry));
        #else
            std::string pathname = archive_entry_pathname(entry);
        #endif
        files.push_back(normalizePath(pathname));
        archive_read_data_skip(archive);
    }

    // Reset archive position
    close();
    open();
    return files;
}

bool ISOReader::readFile(const std::string& filename, std::vector<char>& content) {
    if (!archive) {
        last_error = "Archive not opened";
        return false;
    }

    struct archive_entry* entry;
    std::string normalized_filename = normalizePath(filename);
    
    while (archive_read_next_header(archive, &entry) == ARCHIVE_OK) {
        std::string current_file;
        #ifdef _WIN32
            current_file = wideToString(archive_entry_pathname_w(entry));
        #else
            current_file = archive_entry_pathname(entry);
        #endif
        current_file = normalizePath(current_file);

        if (normalized_filename == current_file) {
            const size_t size = static_cast<size_t>(archive_entry_size(entry));
            content.resize(size);
            
            const ssize_t read_size = archive_read_data(archive, content.data(), size);
            
            // Reset archive position
            close();
            open();
            
            if (read_size < 0 || static_cast<size_t>(read_size) != size) {
                last_error = "Failed to read complete file";
                return false;
            }
            return true;
        }
        archive_read_data_skip(archive);
    }

    last_error = "File not found in archive";
    // Reset archive position
    close();
    open();
    return false;
}

std::string ISOReader::getLastError() const {
    return last_error;
}

std::string ISOReader::normalizePath(const std::string& path) const {
    std::string normalized = path;
    #ifdef _WIN32
        std::replace(normalized.begin(), normalized.end(), '/', '\\');
    #else
        std::replace(normalized.begin(), normalized.end(), '\\', '/');
    #endif
    return normalized;
}

#ifdef _WIN32
std::string ISOReader::wideToString(const wchar_t* wide) const {
    if (!wide) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
    if (size == 0) return "";
    std::vector<char> buffer(size);
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, buffer.data(), size, nullptr, nullptr);
    return std::string(buffer.data());
}

std::wstring ISOReader::stringToWide(const std::string& str) const {
    if (str.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    if (size == 0) return L"";
    std::vector<wchar_t> buffer(size);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, buffer.data(), size);
    return std::wstring(buffer.data());
}
#endif