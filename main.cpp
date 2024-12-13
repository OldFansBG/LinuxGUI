#include "MainFrame.h"
#include <wx/wx.h>
#include "ThemeSystem.h"

class ISOAnalyzerApp : public wxApp {
public:
    virtual bool OnInit() {
        wxInitAllImageHandlers();
        
        // Initialize theme system with system preference
        ThemeSystem::Get().DetectSystemTheme();
        
        MainFrame* frame = new MainFrame("ISO Analyzer");
        frame->Show(true);
        return true;
    }
    
    virtual int OnExit() {
        // Save theme preferences before exit
        ThemeSystem::Get().SaveThemePreference();
        return wxApp::OnExit();
    }
};

wxIMPLEMENT_APP(ISOAnalyzerApp);