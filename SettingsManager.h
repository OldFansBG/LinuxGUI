#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <wx/string.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>  // Add this line
#include "AppSettings.h"

class SettingsManager {
public:
    SettingsManager();

    bool LoadSettings(AppSettings& settings, const wxString& configPath);
    bool SaveSettings(const AppSettings& settings, const wxString& configPath);  // Single declaration

private:
    rapidjson::Document SettingsToJson(const AppSettings& settings) const;
    AppSettings JsonToSettings(const rapidjson::Document& doc) const;
};

#endif // SETTINGS_MANAGER_H