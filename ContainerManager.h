#ifndef CONTAINER_MANAGER_H
#define CONTAINER_MANAGER_H

#include <wx/string.h>
#include <wx/utils.h>  // For wxExecute
#include <wx/file.h>   // For file operations
#include <wx/log.h>    // For wxLogError
#include <mutex>
#include <string>

class ContainerManager {
public:
    static ContainerManager& Get();
    
    bool SaveContainerId(const wxString& containerId, const wxString& isoPath);
    wxString GetCurrentContainerId() const;
    bool CleanupContainer(const wxString& containerId);
    void CleanupAllContainers();
    bool EnsureOutputDirectory(const wxString& containerId);

private:
    ContainerManager() = default;
    ContainerManager(const ContainerManager&) = delete;
    ContainerManager& operator=(const ContainerManager&) = delete;

    wxString m_currentContainerId;
    std::mutex m_mutex;
};

#endif // CONTAINER_MANAGER_H