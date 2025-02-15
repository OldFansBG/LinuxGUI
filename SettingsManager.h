#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <wx/string.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include "AppSettings.h"

class SettingsManager {
public:
    // Save settings to a specific path
    bool SaveSettings(const AppSettings& settings, const wxString& configPath);

    // Load settings from a specific path
    bool LoadSettings(AppSettings& settings, const wxString& configPath);

private:
    rapidjson::Document SettingsToJson(const AppSettings& settings) const;
    AppSettings JsonToSettings(const rapidjson::Document& doc) const;
};
#endif // SETTINGS_MANAGER_H