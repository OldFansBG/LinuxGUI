#include "DockerTransfer.h"
#include <wx/process.h>
#include <wx/txtstrm.h>
#include <wx/filename.h>

DockerTransfer::DockerTransfer(const wxString& isoPath, ProgressCallback callback)
    : m_isoPath(isoPath), m_progressCallback(std::move(callback))
{
}

bool DockerTransfer::TransferISOToContainer() {
    // First copy the ISO file
    wxString command = wxString::Format("docker cp \"%s\" %s:/iso.iso", 
                                      m_isoPath, m_containerId);
    
    wxString output;
    if (!ExecuteDockerCommand(command, output)) {
        m_progressCallback(0, "Failed to copy ISO to container: " + output);
        return false;
    }

    m_progressCallback(50, "ISO file copied successfully");
    return true;
}

bool DockerTransfer::ExecuteDockerCommand(const wxString& command, wxString& output) {
    wxArrayString outputArr, errorArr;
    wxExecute(command, outputArr, errorArr, wxEXEC_SYNC);
    
    output = wxJoin(outputArr, '\n');
    if (!errorArr.IsEmpty()) {
        output += "\nErrors:\n" + wxJoin(errorArr, '\n');
        return false;
    }
    return true;
}