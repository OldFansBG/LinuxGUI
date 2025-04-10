#ifndef WINDOW_IDS_H
#define WINDOW_IDS_H

// It's generally good practice to start custom IDs above wxID_HIGHEST
// or use wxNewId() / wxID_ANY, but for simplicity, we'll keep the existing enum style.
// Just be mindful of potential clashes with wxWidgets' internal IDs if you add many more.

enum
{
    // --- MainFrame IDs ---
    ID_BROWSE_ISO = 1, // Button to browse for ISO file
    ID_BROWSE_WORKDIR, // Button to browse for working directory
    ID_DETECT,         // Button to detect OS/Distro
    ID_CANCEL,         // Button to cancel/exit application (might be generic)
    ID_SETTINGS,       // Button to open settings dialog
    ID_NEXT,           // Button to proceed from MainFrame to SecondWindow

    // --- SecondWindow Tab/View IDs ---
    ID_TERMINAL_TAB = 1001, // Button to switch to Terminal view
    ID_SQL_TAB,             // Button to switch to Config view (formerly SQL)

    // --- SecondWindow Button IDs ---
    ID_NEXT_BUTTON,    // Button in SecondWindow to trigger "Create ISO"
    ID_MONGODB_BUTTON, // Button in SecondWindow top bar to show MongoDB panel

    // --- MongoDBPanel Communication ID ---
    // ID used for the event sent *from* MongoDBPanel *to* SecondWindow when its internal close button is clicked.
    ID_MONGODB_PANEL_CLOSE,

    // --- ConfigTab Sub-Tab IDs (Ensure these don't clash) ---
    // These were previously defined in ConfigTab.h, centralizing them here
    ID_CONFIG_DESKTOP = 2001,
    ID_CONFIG_APPS,
    ID_CONFIG_SYSTEM,
    ID_CONFIG_CUSTOMIZE,

    // --- CustomTitleBar Button IDs (Ensure these don't clash) ---
    // These were previously defined in CustomTitleBar.h
    ID_MINIMIZE = 3001,
    ID_CLOSE_WINDOW,

    // Add other specific IDs for widgets as needed...
    // Example: ID_HOSTNAME_INPUT, ID_LANGUAGE_CHOICE etc. if needed for event handling

    // --- FlatpakStore Timer IDs (from FlatpakStore.h) ---
    ID_BATCH_TIMER = 4001,
    ID_LAYOUT_TIMER,
    ID_SHOW_INSTALLED_BUTTON // Button within FlatpakStore

    // Add other IDs...
};

#endif // WINDOW_IDS_H