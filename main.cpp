#include "MainFrame.h"
#include <wx/wx.h>
#include "ThemeSystem.h"

class ISOAnalyzerApp : public wxApp {
public:
    bool ISOAnalyzerApp::OnInit() {
        wxInitAllImageHandlers();

        // Initialize and detect system theme before creating main frame
        ThemeSystem::Get().DetectSystemTheme();
        ThemeSystem::Get().LoadThemePreference();  // Load any saved theme preferences

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