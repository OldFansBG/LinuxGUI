// ScriptManager.h
#ifndef SCRIPT_MANAGER_H
#define SCRIPT_MANAGER_H

#include <wx/wx.h>
#include <map>

class ScriptManager {
public:
    static ScriptManager& Get();
    
    bool ExtractScriptToTemp(const wxString& scriptName);
    wxString GetTempScriptPath(const wxString& scriptName) const;

private:
    ScriptManager();
    wxString m_tempDir;
    std::map<wxString, wxString> m_scriptContents;
    
    bool WriteScriptToFile(const wxString& path, const wxString& content);
    bool EnsureTempDirectory();
    void InitializeScripts();
};

#endif // SCRIPT_MANAGER_H