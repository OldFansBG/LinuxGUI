#include "SettingsManager.h"
#include <wx/file.h>
#include <wx/log.h>
#include <wx/filename.h>  // Needed for wxFileName

// In SettingsManager.cpp

bool SettingsManager::SaveSettings(const AppSettings& settings, const wxString& configPath) {
    rapidjson::Document doc = SettingsToJson(settings);
    
    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);  // Now valid
    writer.SetIndent(' ', 4);
    doc.Accept(writer);

    wxFile file(configPath, wxFile::write);
    if (!file.IsOpened()) return false;
    
    return file.Write(buffer.GetString(), buffer.GetSize());
}

// Add to SettingsManager.cpp
SettingsManager::SettingsManager() {
    // Constructor can be empty if no initialization is needed
    // Or add initialization logic here if required
}

bool SettingsManager::LoadSettings(AppSettings& settings, const wxString& configPath) {
    if (!wxFileExists(configPath)) {
        wxLogMessage("No settings file found at: %s", configPath);
        return false;
    }

    wxFile file(configPath);
    if (!file.IsOpened()) return false;

    wxString jsonStr;
    file.ReadAll(&jsonStr);
    
    rapidjson::Document doc;
    doc.Parse(jsonStr.ToUTF8().data());
    if (doc.HasParseError()) return false;

    settings = JsonToSettings(doc);
    return true;
}

rapidjson::Document SettingsManager::SettingsToJson(const AppSettings& settings) const {
    rapidjson::Document doc;
    doc.SetObject();
    auto& allocator = doc.GetAllocator();

    // Save ISO path
    doc.AddMember("iso_path", 
        rapidjson::Value(settings.isoPath.ToUTF8().data(), allocator), 
        allocator);

    // Save working directory (last folder name only)
    wxString workDir = settings.workDir;
    wxString workDirName;

    if (!workDir.IsEmpty()) {
        // Ensure the path ends with a separator to force directory parsing
        if (!workDir.EndsWith(wxFileName::GetPathSeparator())) {
            workDir += wxFileName::GetPathSeparator();
        }
        
        wxFileName fn(workDir);
        fn.Normalize(wxPATH_NORM_LONG | wxPATH_NORM_DOTS);  // Normalize the path

        const wxArrayString& dirs = fn.GetDirs();
        if (!dirs.IsEmpty()) {
            workDirName = dirs.Last();  // Get the last folder (e.g., "MyProject")
        } else {
            // Handle root paths like "C:\" or "/"
            workDirName = fn.GetFullName();  // Returns "C:" for "C:\"
        }
    }

    doc.AddMember("work_dir", 
        rapidjson::Value(workDirName.ToUTF8().data(), allocator), 
        allocator);

    // Save other fields
    doc.AddMember("project_name", 
        rapidjson::Value(settings.projectName.ToUTF8().data(), allocator), 
        allocator);
    doc.AddMember("version", 
        rapidjson::Value(settings.version.ToUTF8().data(), allocator), 
        allocator);
    doc.AddMember("detected_distro", 
        rapidjson::Value(settings.detectedDistro.ToUTF8().data(), allocator), 
        allocator);

    return doc;
}

AppSettings SettingsManager::JsonToSettings(const rapidjson::Document& doc) const {
    AppSettings settings;
    
    if (doc.HasMember("iso_path")) 
        settings.isoPath = wxString::FromUTF8(doc["iso_path"].GetString());
    if (doc.HasMember("work_dir")) 
        settings.workDir = wxString::FromUTF8(doc["work_dir"].GetString());
    if (doc.HasMember("project_name")) 
        settings.projectName = wxString::FromUTF8(doc["project_name"].GetString());
    if (doc.HasMember("version")) 
        settings.version = wxString::FromUTF8(doc["version"].GetString());
    if (doc.HasMember("detected_distro")) 
        settings.detectedDistro = wxString::FromUTF8(doc["detected_distro"].GetString());

    return settings;
}