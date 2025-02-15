#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <wx/string.h>

// Struct to hold application settings
struct AppSettings {
    wxString isoPath;         // Path to the ISO file
    wxString workDir;         // Working directory
    wxString projectName;     // Project name
    wxString version;         // Version of the project
    wxString detectedDistro;  // Detected Linux distribution
};

#endif // APPSETTINGS_H