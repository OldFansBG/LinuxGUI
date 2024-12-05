#include "OSDetector.h"

OSDetector::OS OSDetector::GetCurrentOS() {
    #if defined(__WINDOWS__)
        return OS::Windows;
    #elif defined(__APPLE__)
        return OS::MacOS;
    #elif defined(__LINUX__)
        return OS::Linux;
    #else
        return OS::Unknown;
    #endif
}

wxString OSDetector::GetOSName(OS os) {
    switch (os) {
        case OS::Windows: return "Windows";
        case OS::MacOS: return "macOS";
        case OS::Linux: return "Linux";
        default: return "Unknown OS";
    }
}