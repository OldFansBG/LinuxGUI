#include <wx/wx.h>

class MyFrame : public wxFrame {
public:
    MyFrame() : wxFrame(nullptr, wxID_ANY, "Transparent Button") {
        // Create a transparent button
        wxButton* btn = new wxButton(this, wxID_ANY, "Click me!");
        btn->SetBackgroundColour(wxColour(0, 0, 0, 0)); // Set the button's background to be fully transparent
        btn->SetForegroundColour(*wxWHITE); // Set the text color to white

        // Bind a click event handler to the button
        btn->Bind(wxEVT_BUTTON, &MyFrame::OnButtonClick, this);

        // Add the button to the frame
        wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
        sizer->Add(btn, 0, wxALIGN_CENTER | wxTOP, 20);
        SetSizer(sizer);
    }

    void OnButtonClick(wxCommandEvent& event) {
        // Handle the button click event
        wxMessageBox("You clicked the button!");
    }
};

class MyApp : public wxApp {
public:
    virtual bool OnInit() {
        MyFrame* frame = new MyFrame();
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(MyApp);