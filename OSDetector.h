#ifndef OSDETECTOR_H
#define OSDETECTOR_H

#include <wx/wx.h>

class OSDetector {
public:
    enum class OS {
        Windows,
        MacOS,
        Linux,
        Unknown
    };

    OS GetCurrentOS();
    wxString GetOSName(OS os);
};

#endif // OSDETECTOR_H