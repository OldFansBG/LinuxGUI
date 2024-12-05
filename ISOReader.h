#pragma once

#include <string>
#include <vector>
#include <memory>

// Forward declaration to avoid including archive.h in header
struct archive;

class ISOReader {
public:
    explicit ISOReader(const std::string& path);
    ~ISOReader();

    // Prevent copying
    ISOReader(const ISOReader&) = delete;
    ISOReader& operator=(const ISOReader&) = delete;

    // Allow moving
    ISOReader(ISOReader&&) noexcept;
    ISOReader& operator=(ISOReader&&) noexcept;

    // Core functionality
    bool open();
    void close();
    std::vector<std::string> listFiles();
    bool readFile(const std::string& filename, std::vector<char>& content);
    
    // Get error message if operation fails
    std::string getLastError() const;

private:
    std::string normalizePath(const std::string& path) const;
    
    #ifdef _WIN32
    std::string wideToString(const wchar_t* wide) const;
    std::wstring stringToWide(const std::string& str) const;
    #endif

    std::string iso_path;
    struct archive* archive;
    std::string last_error;
};