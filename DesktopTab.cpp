#include "DesktopTab.h"
#include <wx/filename.h>
#include <array>
#include <wx/textfile.h>
#include "SecondWindow.h"
#include <wx/file.h>
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <wx/process.h>

// Custom event declaration
wxDEFINE_EVENT(FILE_COPY_COMPLETE_EVENT, wxCommandEvent);

// Custom wxProcess subclass for async Docker commands
class DesktopInstallProcess : public wxProcess {
public:
    DesktopInstallProcess(DesktopTab* parent, const wxString& newEnv, wxButton* button)
        : wxProcess(parent), m_parent(parent), m_newEnv(newEnv), m_button(button) {}

    virtual void OnTerminate(int pid, int status) override {
        if (m_parent) {
            if (status == 0) {
                wxLogDebug("DesktopInstallProcess::OnTerminate - Successfully installed %s", m_newEnv);
                wxString guiFilePath = m_parent->GetGuiFilePath();
                wxFile guiFile;
                if (guiFile.Create(guiFilePath, true) && guiFile.IsOpened()) {
                    guiFile.Write(m_newEnv);
                    guiFile.Close();
                    wxLogDebug("DesktopInstallProcess::OnTerminate - Updated detected_gui.txt with %s", m_newEnv);
                }
                m_parent->UpdateGUILabel(m_newEnv);
                wxMessageBox(
                    wxString::Format("%s installed successfully!", m_newEnv),
                    "Success",
                    wxOK | wxICON_INFORMATION,
                    m_parent
                );
            } else {
                wxLogDebug("DesktopInstallProcess::OnTerminate - Failed to install %s, status: %d", m_newEnv, status);
                wxMessageBox("Failed to install new environment! Check logs.", "Error", wxICON_ERROR, m_parent);
            }
            if (m_button) m_button->Enable();
            delete this; // Clean up process object
        }
    }

private:
    DesktopTab* m_parent;
    wxString m_newEnv;
    wxButton* m_button;
};

DesktopTab::DesktopTab(wxWindow* parent)
    : wxPanel(parent), m_gridSizer(nullptr)
{
    CreateDesktopTab();
    Bind(wxEVT_SIZE, &DesktopTab::OnSize, this);
    Bind(FILE_COPY_COMPLETE_EVENT, &DesktopTab::OnFileCopyComplete, this);
}

wxBitmap DesktopTab::LoadImage(const wxString& imageName) {
    wxFileName path(wxFileName::GetCwd(), imageName);
    path.AppendDir("desktopenvs");
    wxLogDebug("DesktopTab::LoadImage - Loading image from: %s", path.GetFullPath());
    if (path.FileExists()) {
        return wxBitmap(path.GetFullPath(), wxBITMAP_TYPE_PNG);
    }
    wxLogDebug("DesktopTab::LoadImage - Image not found: %s", path.GetFullPath());
    return wxNullBitmap;
}

wxBitmap DesktopTab::ConvertToGrayscale(const wxBitmap& original) {
    wxImage image = original.ConvertToImage();
    if (!image.IsOk()) {
        wxLogDebug("DesktopTab::ConvertToGrayscale - Failed to convert bitmap to image");
        return original;
    }
    image = image.ConvertToGreyscale();
    return wxBitmap(image);
}

void DesktopTab::ApplyGrayscaleToCards(const wxString& detectedEnv) {
    wxLogDebug("DesktopTab::ApplyGrayscaleToCards - Applying grayscale for detected env: %s", detectedEnv);
    
    wxWindowList& children = m_scrolledWindow->GetChildren();
    for (wxWindowList::iterator it = children.begin(); it != children.end(); ++it) {
        wxPanel* card = wxDynamicCast(*it, wxPanel);
        if (!card || !card->GetSizer()) continue;
        
        wxString cardName = card->GetName();
        bool isDetectedEnv = (cardName == detectedEnv);
        wxLogDebug("DesktopTab::ApplyGrayscaleToCards - Card: %s, is detected: %d", cardName, isDetectedEnv);
        
        wxWindowList& cardChildren = card->GetChildren();
        for (wxWindowList::iterator childIt = cardChildren.begin(); childIt != cardChildren.end(); ++childIt) {
            wxPanel* imageContainer = wxDynamicCast(*childIt, wxPanel);
            if (!imageContainer || !imageContainer->GetClientData()) continue;
            
            wxBitmap* storedBitmap = static_cast<wxBitmap*>(imageContainer->GetClientData());
            if (storedBitmap && storedBitmap->IsOk()) {
                if (!isDetectedEnv) {
                    *storedBitmap = ConvertToGrayscale(*storedBitmap);
                    wxLogDebug("DesktopTab::ApplyGrayscaleToCards - Applied grayscale to %s", cardName);
                }
                imageContainer->Refresh();
            }
        }
        
        wxWindowList& grandChildren = card->GetChildren();
        for (wxWindowList::iterator gcIt = grandChildren.begin(); gcIt != grandChildren.end(); ++gcIt) {
            wxButton* button = wxDynamicCast(*gcIt, wxButton);
            if (button) {
                if (isDetectedEnv) {
                    button->SetBackgroundColour(wxColour(0, 120, 215));
                } else {
                    button->SetBackgroundColour(wxColour(80, 80, 80));
                }
                button->Refresh();
            }
        }
    }
    m_scrolledWindow->Refresh();
}

void DesktopTab::OnSize(wxSizeEvent& event) {
    wxSize size = event.GetSize();
    wxLogDebug("DesktopTab::OnSize - Window size changed: %d x %d", size.GetWidth(), size.GetHeight());
    RecalculateLayout(size.GetWidth());
    event.Skip();
}

void DesktopTab::RecalculateLayout(int windowWidth) {
    if (!m_gridSizer) return;

    int availableWidth = windowWidth - 40;
    int cardWithSpacing = 220;
    int columns = std::max(1, availableWidth / cardWithSpacing);
    wxLogDebug("DesktopTab::RecalculateLayout - Window width: %d, Columns: %d", windowWidth, columns);

    if (m_gridSizer->GetCols() != columns) {
        m_gridSizer->SetCols(columns);

        for (int i = 0; i < m_gridSizer->GetCols(); i++) {
            if (m_gridSizer->IsColGrowable(i)) {
                m_gridSizer->RemoveGrowableCol(i);
            }
        }

        for (int i = 0; i < columns; i++) {
            m_gridSizer->AddGrowableCol(i, 1);
        }

        Layout();
        Refresh();
    }
}

wxString DesktopTab::GetGuiFilePath() {
    wxString guiFilePath;
    wxWindow* parent = GetParent();
    while (parent) {
        SecondWindow* secondWindow = wxDynamicCast(parent, SecondWindow);
        if (secondWindow) {
            guiFilePath = wxFileName(secondWindow->GetProjectDir(), "detected_gui.txt").GetFullPath();
            wxLogDebug("DesktopTab::GetGuiFilePath - Found SecondWindow, gui path: %s", guiFilePath);
            break;
        }
        parent = parent->GetParent();
    }
    if (guiFilePath.IsEmpty()) {
        guiFilePath = wxFileName(wxGetCwd(), "detected_gui.txt").GetFullPath();
        wxLogDebug("DesktopTab::GetGuiFilePath - No SecondWindow, using cwd: %s", guiFilePath);
    }
    return guiFilePath;
}

void DesktopTab::CreateDesktopTab() {
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    m_scrolledWindow = new wxScrolledWindow(this, wxID_ANY);
    m_scrolledWindow->SetScrollRate(0, 10);
    m_scrolledWindow->SetBackgroundColour(wxColour(18, 18, 18));

    wxBoxSizer* scrolledSizer = new wxBoxSizer(wxVERTICAL);
    m_scrolledWindow->SetSizer(scrolledSizer);

    wxBoxSizer* headerSizer = new wxBoxSizer(wxHORIZONTAL);
    m_textDisplay = new wxStaticText(m_scrolledWindow, wxID_ANY, "GUI Environment: Not detected");
    m_textDisplay->SetForegroundColour(*wxWHITE);
    m_textDisplay->SetFont(wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    headerSizer->Add(m_textDisplay, 1, wxALIGN_LEFT | wxALL, 10);

    m_reloadButton = new wxButton(m_scrolledWindow, wxID_ANY, "Reload UI", 
                                 wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    m_reloadButton->SetForegroundColour(*wxWHITE);
    m_reloadButton->SetBackgroundColour(wxColour(60, 60, 60));
    headerSizer->Add(m_reloadButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);

    m_reloadButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        UpdateTextFromFile();
    });

    scrolledSizer->Add(headerSizer, 0, wxEXPAND);

    const std::array<wxString, 9> environments = {
        "GNOME", "KDE Plasma", "XFCE",
        "Cinnamon", "MATE", "LXQt",
        "Budgie", "Deepin", "Pantheon"
    };

    const std::array<wxString, 9> imageFiles = {
        "GNOME.png", "KDE.png", "Xfce.png",
        "Cinnamon.png", "MATE.png", "LXQt.png",
        "Budgie.png", "Deepin.png", "Pantheon.png"
    };

    const std::array<wxString, 9> packageNames = {
        "gnome-session", "plasma-desktop", "xfce4",
        "cinnamon", "mate-desktop", "lxqt",
        "budgie-desktop", "deepin-desktop-environment", "pantheon-desktop"
    };

    m_gridSizer = new wxFlexGridSizer(3, 20, 20);
    m_gridSizer->SetFlexibleDirection(wxBOTH);

    for (size_t i = 0; i < environments.size(); ++i) {
        wxPanel* card = new wxPanel(m_scrolledWindow);
        card->SetBackgroundColour(wxColour(32, 32, 32));
        card->SetMinSize(wxSize(200, 300));
        card->SetName(environments[i]);

        wxBoxSizer* cardSizer = new wxBoxSizer(wxVERTICAL);

        wxPanel* imageContainer = new wxPanel(card, wxID_ANY);
        imageContainer->SetMinSize(wxSize(180, 240));
        imageContainer->SetBackgroundColour(wxColour(32, 32, 32));

        wxBitmap bitmap = LoadImage(imageFiles[i]);
        if (bitmap.IsOk()) {
            imageContainer->SetClientData(new wxBitmap(bitmap));
            
            imageContainer->Bind(wxEVT_PAINT, [this](wxPaintEvent& evt) {
                wxPanel* panel = dynamic_cast<wxPanel*>(evt.GetEventObject());
                if (!panel) return;
                
                wxPaintDC dc(panel);
                wxSize panelSize = panel->GetSize();
                
                wxBitmap* bmpPtr = static_cast<wxBitmap*>(panel->GetClientData());
                if (!bmpPtr || !bmpPtr->IsOk()) return;
                
                wxBitmap bmpToDraw = *bmpPtr;
                wxSize bmpSize = bmpToDraw.GetSize();

                double scale = std::min(
                    static_cast<double>(panelSize.GetWidth()) / bmpSize.GetWidth(),
                    static_cast<double>(panelSize.GetHeight()) / bmpSize.GetHeight()
                );

                int newWidth = bmpSize.GetWidth() * scale;
                int newHeight = bmpSize.GetHeight() * scale;

                int x = (panelSize.GetWidth() - newWidth) / 2;
                int y = (panelSize.GetHeight() - newHeight) / 2;

                wxImage img = bmpToDraw.ConvertToImage();
                dc.DrawBitmap(
                    wxBitmap(img.Scale(newWidth, newHeight, wxIMAGE_QUALITY_HIGH)),
                    x, y
                );
            });
        }

        cardSizer->Add(imageContainer, 1, wxEXPAND | wxALL, 10);

        wxStaticText* title = new wxStaticText(card, wxID_ANY, environments[i]);
        title->SetForegroundColour(*wxWHITE);
        title->SetFont(wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
        cardSizer->Add(title, 0, wxALIGN_LEFT | wxLEFT | wxRIGHT, 10);

        wxStaticText* info = new wxStaticText(card, wxID_ANY, "0/100 Achievements");
        info->SetForegroundColour(wxColour(180, 180, 180));
        info->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        cardSizer->Add(info, 0, wxALIGN_LEFT | wxLEFT | wxRIGHT | wxTOP, 10);

        wxButton* installButton = new wxButton(card, wxID_ANY, "Install");
        installButton->SetMinSize(wxSize(80, 30));
        installButton->SetBackgroundColour(wxColour(0, 120, 215));
        installButton->SetForegroundColour(*wxWHITE);
        cardSizer->Add(installButton, 0, wxALIGN_LEFT | wxALL, 10);

        installButton->Bind(wxEVT_BUTTON, [this, environments, packageNames, i](wxCommandEvent& event) {
            wxString newEnv = environments[i];
            wxString newPackage = packageNames[i];
            wxLogDebug("DesktopTab::InstallButton - Installing %s (package: %s)", newEnv, newPackage);

            wxWindow* parent = GetParent();
            SecondWindow* secondWindow = nullptr;
            wxString containerIdPath;
            wxString projectDir;

            while (parent) {
                secondWindow = wxDynamicCast(parent, SecondWindow);
                if (secondWindow) {
                    containerIdPath = wxFileName(secondWindow->GetProjectDir(), "container_id.txt").GetFullPath();
                    projectDir = secondWindow->GetProjectDir();
                    wxLogDebug("DesktopTab::InstallButton - Found SecondWindow, project dir: %s", projectDir);
                    break;
                }
                parent = parent->GetParent();
            }

            if (!secondWindow || containerIdPath.IsEmpty()) {
                wxLogDebug("DesktopTab::InstallButton - Failed to find SecondWindow or container ID path");
                wxMessageBox("Failed to find container ID!", "Error", wxICON_ERROR);
                return;
            }

            wxFile file;
            wxString containerId;
            if (file.Open(containerIdPath)) {
                file.ReadAll(&containerId);
                containerId.Trim();
                wxLogDebug("DesktopTab::InstallButton - Container ID: %s", containerId);
            } else {
                wxLogDebug("DesktopTab::InstallButton - Failed to read container_id.txt: %s", containerIdPath);
                wxMessageBox("Failed to read container ID!", "Error", wxICON_ERROR);
                return;
            }

            wxString currentEnv;
            wxString guiFilePath = wxFileName(projectDir, "detected_gui.txt").GetFullPath();
            if (wxFileExists(guiFilePath)) {
                wxFile guiFile(guiFilePath);
                guiFile.ReadAll(&currentEnv); // Fixed variable name
                currentEnv.Trim();
                wxLogDebug("DesktopTab::InstallButton - Current environment: %s", currentEnv);
            } else {
                currentEnv = "Not detected";
                wxLogDebug("DesktopTab::InstallButton - No current environment detected");
            }

            if (currentEnv == newEnv) {
                wxLogDebug("DesktopTab::InstallButton - %s is already installed", newEnv);
                wxMessageBox("This desktop environment is already installed!", "Info", wxOK | wxICON_INFORMATION);
                return;
            }

            wxString currentPackage;
            for (size_t j = 0; j < environments.size(); ++j) {
                if (environments[j] == currentEnv) {
                    currentPackage = packageNames[j];
                    break;
                }
            }

            int confirm = wxMessageBox(
                wxString::Format("Uninstall %s and install %s?", currentEnv, newEnv),
                "Confirm Desktop Environment Change",
                wxYES_NO | wxICON_QUESTION
            );
            if (confirm != wxYES) {
                wxLogDebug("DesktopTab::InstallButton - User cancelled installation");
                return;
            }

            wxButton* btn = wxDynamicCast(event.GetEventObject(), wxButton);
            if (btn) btn->Disable();
            wxLogDebug("DesktopTab::InstallButton - Disabled install button for %s", newEnv);

            // Updated command with purge and session configuration
            wxString command;
            if (currentEnv != "Not detected" && !currentPackage.IsEmpty()) {
                command = wxString::Format(
                    "docker exec %s /bin/bash -c \"chroot /root/custom_iso/squashfs-root /bin/bash -c 'apt-get update && apt-get purge -y %s && apt-get autoremove -y && apt-get install -y %s && echo -e \\\"[Seat:*]\\\\nuser-session=xfce\\\" > /etc/lightdm/lightdm.conf'\"",
                    containerId, currentPackage, newPackage
                );
            } else {
                command = wxString::Format(
                    "docker exec %s /bin/bash -c \"chroot /root/custom_iso/squashfs-root /bin/bash -c 'apt-get update && apt-get install -y %s && echo -e \\\"[Seat:*]\\\\nuser-session=xfce\\\" > /etc/lightdm/lightdm.conf'\"",
                    containerId, newPackage
                );
            }
            wxLogDebug("DesktopTab::InstallButton - Executing command: %s", command);

            DesktopInstallProcess* proc = new DesktopInstallProcess(this, newEnv, btn);
            long pid = wxExecute(command, wxEXEC_ASYNC | wxEXEC_HIDE_CONSOLE, proc);
            if (pid == 0) {
                wxLogDebug("DesktopTab::InstallButton - Failed to execute command for %s", newEnv);
                wxMessageBox("Failed to start installation process!", "Error", wxICON_ERROR);
                delete proc;
                if (btn) btn->Enable();
            } else {
                wxLogDebug("DesktopTab::InstallButton - Started installation process for %s, PID: %ld", newEnv, pid);
            }
        });

        card->SetSizer(cardSizer);
        m_gridSizer->Add(card, 1, wxEXPAND);
    }

    scrolledSizer->Add(m_gridSizer, 1, wxEXPAND | wxALL, 20);
    mainSizer->Add(m_scrolledWindow, 1, wxEXPAND);
    SetSizer(mainSizer);

    CallAfter(&DesktopTab::RecalculateLayout, GetSize().GetWidth());
    wxLogDebug("DesktopTab::CreateDesktopTab - Desktop tab created and laid out");
}

void DesktopTab::UpdateGUILabel(const wxString& guiName) {
    wxLogDebug("DesktopTab::UpdateGUILabel - Setting label with GUI name: %s", guiName);
    wxString displayText = "GUI Environment: " + guiName;
    m_textDisplay->SetLabel(displayText);
    ApplyGrayscaleToCards(guiName);
    m_textDisplay->Refresh();
    m_scrolledWindow->Layout();
    Refresh(true);
    Update();
    GetParent()->Refresh(true);
    GetParent()->Update();
    wxLogDebug("DesktopTab::UpdateGUILabel - UI updated with %s", guiName);
}

void DesktopTab::OnFileCopyComplete(wxCommandEvent& event) {
    wxString guiName = event.GetString();
    wxLogDebug("DesktopTab::OnFileCopyComplete - Event received with GUI name: %s", guiName);
    
    guiName.Trim(true).Trim(false);
    UpdateGUILabel(guiName.IsEmpty() ? "Not detected" : guiName);
}

void DesktopTab::UpdateTextFromFile() {
    wxLogDebug("DesktopTab::UpdateTextFromFile - Reload UI button clicked");
    
    wxString guiFilePath = GetGuiFilePath();
    wxLogDebug("DesktopTab::UpdateTextFromFile - Looking for file: %s", guiFilePath);
    
    if (wxFileExists(guiFilePath)) {
        wxFile file(guiFilePath);
        wxString content;
        if (file.ReadAll(&content)) {
            content.Trim(true).Trim(false);
            wxLogDebug("DesktopTab::UpdateTextFromFile - Read content: %s", content);
            UpdateGUILabel(content);
        } else {
            wxLogDebug("DesktopTab::UpdateTextFromFile - Failed to read file contents");
            UpdateGUILabel("Not detected");
        }
    } else {
        wxLogDebug("DesktopTab::UpdateTextFromFile - File not found: %s", guiFilePath);
        UpdateGUILabel("Not detected");
    }
}