#ifndef CUSTOMIZETAB_H
#define CUSTOMIZETAB_H

#include <wx/wx.h>

class CustomizeTab : public wxPanel {
public:
    CustomizeTab(wxWindow* parent);
    void CreateContent();  // Method to create the tab content

private:
    DECLARE_EVENT_TABLE()
};

#endif // CUSTOMIZETAB_H