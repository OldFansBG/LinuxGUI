#ifndef CUSTOM_EVENTS_H
#define CUSTOM_EVENTS_H

#include <wx/wx.h>

// Define custom event types
wxDECLARE_EVENT(FILE_COPY_COMPLETE_EVENT, wxCommandEvent); // Existing event
wxDECLARE_EVENT(PYTHON_LOG_UPDATE, wxCommandEvent);        // New event for log updates
wxDECLARE_EVENT(PYTHON_TASK_COMPLETED, wxCommandEvent);    // New event for task completion

#endif // CUSTOM_EVENTS_H