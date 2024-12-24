// ContainerManager.h
#ifndef CONTAINER_MANAGER_H
#define CONTAINER_MANAGER_H

#include <wx/wx.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <fstream>
#include <mutex>

class ContainerManager {
public:
    static ContainerManager& Get() {
        static ContainerManager instance;
        return instance;
    }

    bool SaveContainerId(const wxString& containerId, const wxString& isoPath) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        // Save in app data directory
        wxString appDataPath = wxStandardPaths::Get().GetUserDataDir();
        if (!wxDirExists(appDataPath)) {
            wxMkdir(appDataPath);
        }
        
        wxString containerFile = appDataPath + wxFileName::GetPathSeparator() + "containers.dat";
        std::ofstream file(containerFile.ToStdString(), std::ios::app);
        if (!file) return false;
        
        wxString timestamp = wxDateTime::Now().Format("%Y-%m-%d %H:%M:%S");
        file << containerId.ToStdString() << "|" 
             << isoPath.ToStdString() << "|"
             << timestamp.ToStdString() << "\n";
             
        // Also save to Windows temp for batch script compatibility
        wxString tempPath = wxFileName::GetTempDir() + wxFileName::GetPathSeparator() + "container_id.txt";
        std::ofstream tempFile(tempPath.ToStdString());
        if (tempFile) {
            tempFile << containerId;
        }
        
        m_currentContainerId = containerId;
        return true;
    }

    wxString GetCurrentContainerId() const {
        return m_currentContainerId;
    }

    bool CleanupContainer(const wxString& containerId) {
        if (containerId.IsEmpty()) return false;
        
        wxString cmd = "docker rm -f " + containerId;
        wxExecute(cmd, wxEXEC_SYNC);
        
        // Clean up container ID files
        wxString tempPath = wxFileName::GetTempDir() + wxFileName::GetPathSeparator() + "container_id.txt";
        if (wxFileExists(tempPath)) {
            wxRemoveFile(tempPath);
        }
        
        return true;
    }

    void CleanupAllContainers() {
        wxString appDataPath = wxStandardPaths::Get().GetUserDataDir();
        wxString containerFile = appDataPath + wxFileName::GetPathSeparator() + "containers.dat";
        
        if (!wxFileExists(containerFile)) return;
        
        std::ifstream file(containerFile.ToStdString());
        std::string line;
        while (std::getline(file, line)) {
            size_t pos = line.find('|');
            if (pos != std::string::npos) {
                wxString containerId = wxString::FromUTF8(line.substr(0, pos));
                CleanupContainer(containerId);
            }
        }
        
        wxRemoveFile(containerFile);
    }

private:
    ContainerManager() = default;
    ContainerManager(const ContainerManager&) = delete;
    ContainerManager& operator=(const ContainerManager&) = delete;

    wxString m_currentContainerId;
    std::mutex m_mutex;
};

#endif // CONTAINER_MANAGER_H