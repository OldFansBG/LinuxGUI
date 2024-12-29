// ScriptManager.cpp
#include "ScriptManager.h"
#include <wx/filename.h>
#include <wx/dir.h>

// Include embedded script contents
extern const char* SETUP_CHROOT_SCRIPT;
extern const char* CREATE_ISO_SCRIPT;
extern const char* SETUP_OUTPUT_SCRIPT;

ScriptManager& ScriptManager::Get() {
    static ScriptManager instance;
    return instance;
}

ScriptManager::ScriptManager() {
    m_tempDir = wxFileName::GetTempDir() + wxFileName::GetPathSeparator() + "iso_analyzer_scripts";
    InitializeScripts();
}

void ScriptManager::InitializeScripts() {
    m_scriptContents = {
        {"setup_chroot.sh", SETUP_CHROOT_SCRIPT},
        {"create_iso.sh", CREATE_ISO_SCRIPT},
        {"setup_output.sh", SETUP_OUTPUT_SCRIPT}
    };
}

bool ScriptManager::EnsureTempDirectory() {
    if (!wxDirExists(m_tempDir)) {
        if (!wxMkdir(m_tempDir)) {
            wxLogError("Failed to create temporary directory: %s", m_tempDir);
            return false;
        }
    }
    return true;
}

bool ScriptManager::WriteScriptToFile(const wxString& path, const wxString& content) {
    wxFile file;
    if (!file.Create(path, true) || !file.Write(content)) {
        wxLogError("Failed to write script to: %s", path);
        return false;
    }
    
    #ifndef __WINDOWS__
    chmod(path.c_str(), 0755);  // Make executable on Unix-like systems
    #endif
    
    return true;
}

bool ScriptManager::ExtractScriptToTemp(const wxString& scriptName) {
    if (!EnsureTempDirectory()) return false;
    
    auto it = m_scriptContents.find(scriptName);
    if (it == m_scriptContents.end()) {
        wxLogError("Unknown script: %s", scriptName);
        return false;
    }
    
    wxString path = GetTempScriptPath(scriptName);
    return WriteScriptToFile(path, it->second);
}

wxString ScriptManager::GetTempScriptPath(const wxString& scriptName) const {
    return m_tempDir + wxFileName::GetPathSeparator() + scriptName;
}

