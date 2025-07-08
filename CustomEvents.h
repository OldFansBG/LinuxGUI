#ifndef CUSTOM_EVENTS_H // Предпазител от многократно включване на хедър файла
#define CUSTOM_EVENTS_H // Дефиниране на предпазителя за хедър файла

#include <wx/wx.h> // Включване на основната wxWidgets библиотека

// Дефиниране на типове за персонализирани събития
wxDECLARE_EVENT(FILE_COPY_COMPLETE_EVENT, wxCommandEvent); // Събитие за завършено копиране на файл
wxDECLARE_EVENT(PYTHON_LOG_UPDATE, wxCommandEvent);        // Ново събитие за актуализации на логовете от Python
wxDECLARE_EVENT(PYTHON_TASK_COMPLETED, wxCommandEvent);    // Ново събитие за завършване на Python задача

#endif // CUSTOM_EVENTS_H // Край на предпазителя за хедър файла