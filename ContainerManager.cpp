#include "ContainerManager.h"
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <fstream>

ContainerManager& ContainerManager::Get() {
    static ContainerManager instance;
    return instance;
}

bool ContainerManager::SaveContainerId(const wxString& containerId, const wxString& isoPath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
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
             
    wxString tempPath = wxFileName::GetTempDir() + wxFileName::GetPathSeparator() + "container_id.txt";
    std::ofstream tempFile(tempPath.ToStdString());
    if (tempFile) {
        tempFile << containerId;
    }
    
    m_currentContainerId = containerId;
    return true;
}

wxString ContainerManager::GetCurrentContainerId() const {
    return m_currentContainerId;
}

bool ContainerManager::CleanupContainer(const wxString& containerId) {
    if (containerId.IsEmpty()) return false;
    
    wxString cmd = "docker rm -f " + containerId;
    wxExecute(cmd, wxEXEC_SYNC);
    
    wxString tempPath = wxFileName::GetTempDir() + wxFileName::GetPathSeparator() + "container_id.txt";
    if (wxFileExists(tempPath)) {
        wxRemoveFile(tempPath);
    }
    
    return true;
}

bool ContainerManager::EnsureOutputDirectory(const wxString& containerId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    wxLogMessage("Ensuring output directory exists for container: %s", containerId);
    
    wxString mkdirCmd = wxString::Format(
        "docker exec %s mkdir -p /output", containerId);
    
    wxArrayString output, errors;
    if (wxExecute(mkdirCmd, output, errors, wxEXEC_SYNC) != 0) {
        wxLogError("Failed to create output directory");
        return false;
    }
    
    wxString chmodCmd = wxString::Format(
        "docker exec %s chmod 777 /output", containerId);
    
    if (wxExecute(chmodCmd, output, errors, wxEXEC_SYNC) != 0) {
        wxLogError("Failed to set directory permissions");
        return false;
    }
    
    return true;
}

void ContainerManager::CleanupAllContainers() {
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