#include "MainFrame.h"
#include <wx/wx.h>

class ISOAnalyzerApp : public wxApp {
public:
    bool OnInit() {
        wxInitAllImageHandlers();

        MainFrame* frame = new MainFrame("ISO Analyzer");
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(ISOAnalyzerApp);