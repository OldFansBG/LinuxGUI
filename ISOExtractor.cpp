#include "ISOExtractor.h"
#include <archive.h>
#include <archive_entry.h>
#include <filesystem>

namespace fs = std::filesystem;

ISOExtractor::ISOExtractor(const wxString& isoPath,
                         const wxString& destPath,
                         ProgressCallback progressCallback)
    : wxThread(wxTHREAD_DETACHED)
    , m_isoPath(isoPath)
    , m_destPath(destPath)
    , m_progressCallback(std::move(progressCallback))
    , m_stopRequested(false)
{
}

wxThread::ExitCode ISOExtractor::Entry() {
    if (ExtractTo(m_destPath.ToStdString())) {
        m_progressCallback(100, "Extraction completed successfully");
        return (ExitCode)0;
    }
    return (ExitCode)1;
}

bool ISOExtractor::ExtractTo(const std::string& destPath) {
    // First pass: Calculate total size
    struct archive* archive = archive_read_new();
    archive_read_support_format_iso9660(archive);
    
    if (archive_read_open_filename(archive, m_isoPath.ToStdString().c_str(), 10240) != ARCHIVE_OK) {
        m_progressCallback(0, "Could not open archive: " + 
                         wxString::FromUTF8(archive_error_string(archive)));
        archive_read_free(archive);
        return false;
    }

    int64_t totalSize = 0;
    int64_t processedSize = 0;
    struct archive_entry* entry;
    std::vector<std::pair<std::string, std::string>> hardlinks;

    while (archive_read_next_header(archive, &entry) == ARCHIVE_OK) {
        if (!archive_entry_hardlink(entry)) {
            totalSize += archive_entry_size(entry);
        }
        archive_read_data_skip(archive);
    }

    archive_read_close(archive);
    archive_read_free(archive);

    // Second pass: Extract files
    archive = archive_read_new();
    archive_read_support_format_iso9660(archive);
    
    int flags = ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_SECURE_NODOTDOT;
    struct archive* ext = archive_write_disk_new();
    archive_write_disk_set_options(ext, flags);
    archive_write_disk_set_standard_lookup(ext);
    
    if (archive_read_open_filename(archive, m_isoPath.ToStdString().c_str(), 10240) != ARCHIVE_OK) {
        m_progressCallback(0, "Could not open archive: " + 
                         wxString::FromUTF8(archive_error_string(archive)));
        archive_read_free(archive);
        archive_write_free(ext);
        return false;
    }

    while (archive_read_next_header(archive, &entry) == ARCHIVE_OK) {
        if (m_stopRequested) {
            archive_read_close(archive);
            archive_read_free(archive);
            archive_write_free(ext);
            return false;
        }

        const char* currentFile = archive_entry_pathname(entry);
        std::string fullOutPath = destPath + "/" + currentFile;

        const char* hardlink = archive_entry_hardlink(entry);
        if (hardlink) {
            hardlinks.push_back({std::string(hardlink), std::string(currentFile)});
            continue;
        }

        archive_entry_set_pathname(entry, fullOutPath.c_str());
        
        int r = archive_write_header(ext, entry);
        if (r < ARCHIVE_OK) {
            m_progressCallback(0, "Failed to write header: " + 
                             wxString::FromUTF8(archive_error_string(ext)));
            continue;
        }

        if (archive_entry_size(entry) > 0) {
            const void* buff;
            size_t size;
            int64_t offset;

            while (archive_read_data_block(archive, &buff, &size, &offset) == ARCHIVE_OK) {
                if (archive_write_data_block(ext, buff, size, offset) != ARCHIVE_OK) {
                    m_progressCallback(0, "Failed to write data: " + 
                                     wxString::FromUTF8(archive_error_string(ext)));
                    break;
                }
                processedSize += size;
                int progress = static_cast<int>((processedSize * 100) / totalSize);
                m_progressCallback(progress, wxString::Format("Extracting: %s", currentFile));
            }
        }

        r = archive_write_finish_entry(ext);
        if (r < ARCHIVE_OK) {
            m_progressCallback(0, "Failed to finish entry: " + 
                             wxString::FromUTF8(archive_error_string(ext)));
        }
    }

    archive_read_close(archive);
    archive_read_free(archive);
    archive_write_free(ext);

    // Third pass: Create hard links
    for (const auto& [target, link] : hardlinks) {
        std::string targetPath = destPath + "/" + target;
        std::string linkPath = destPath + "/" + link;
        
        #ifdef _WIN32
            std::wstring wideTarget(targetPath.begin(), targetPath.end());
            std::wstring wideLink(linkPath.begin(), linkPath.end());
            CreateHardLinkW(wideLink.c_str(), wideTarget.c_str(), nullptr);
        #else
            link(targetPath.c_str(), linkPath.c_str());
        #endif
    }

    return true;
}