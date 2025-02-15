#include "SettingsManager.h"
#include <wx/file.h>
#include <wx/log.h>

bool SettingsManager::SaveSettings(const AppSettings& settings, const wxString& configPath) {
    rapidjson::Document doc = SettingsToJson(settings);
    
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    wxFile file(configPath, wxFile::write);
    if (!file.IsOpened()) {
        wxLogError("Failed to open settings file: %s", configPath);
        return false;
    }
    
    if (!file.Write(buffer.GetString(), buffer.GetSize())) {
        wxLogError("Failed to write settings to: %s", configPath);
        return false;
    }
    
    wxLogMessage("Settings saved to: %s", configPath);
    return true;
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

    doc.AddMember("iso_path", 
        rapidjson::Value(settings.isoPath.ToUTF8().data(), allocator), 
        allocator);
    doc.AddMember("work_dir", 
        rapidjson::Value(settings.workDir.ToUTF8().data(), allocator), 
        allocator);
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