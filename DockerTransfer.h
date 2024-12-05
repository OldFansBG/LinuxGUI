#ifndef DOCKER_TRANSFER_H
#define DOCKER_TRANSFER_H

#include <wx/wx.h>
#include <string>
#include <functional>

class DockerTransfer {
public:
    using ProgressCallback = std::function<void(int, const wxString&)>;
    
    DockerTransfer(const wxString& isoPath, ProgressCallback callback);
    bool TransferISOToContainer();
    bool ExecuteInitialSetup();
    void SetContainerId(const wxString& id) { m_containerId = id; }
    const wxString& GetContainerId() const { return m_containerId; }

private:
    bool ExecuteDockerCommand(const wxString& command, wxString& output);
    bool WaitForContainer();
    
    wxString m_isoPath;
    wxString m_containerId;
    ProgressCallback m_progressCallback;
};

#endif // DOCKER_TRANSFER_H