#include "OSDetector.h" // Включване на хедър файла за класа OSDetector

OSDetector::OS OSDetector::GetCurrentOS()
{                        // Имплементация на метода за определяне на текущата операционна система
#if defined(__WINDOWS__) // Проверка дали компилираме за Windows
    return OS::Windows;  // Връщаме Windows като резултат
#elif defined(__APPLE__) // Проверка дали компилираме за Apple (MacOS)
    return OS::MacOS; // Връщаме MacOS като резултат
#elif defined(__LINUX__) // Проверка дали компилираме за Linux
    return OS::Linux; // Връщаме Linux като резултат
#else                    // Ако нито една от горните проверки не е успешна
    return OS::Unknown; // Връщаме неизвестна операционна система
#endif
}

wxString OSDetector::GetOSName(OS os)
{ // Имплементация на метода за получаване на името на операционната система като текст
    switch (os)
    { // Използване на switch за различните операционни системи
    case OS::Windows:
        return "Windows"; // Връщаме "Windows" за Windows
    case OS::MacOS:
        return "macOS"; // Връщаме "macOS" за MacOS
    case OS::Linux:
        return "Linux"; // Връщаме "Linux" за Linux
    default:
        return "Unknown OS"; // Връщаме "Unknown OS" за всички останали случаи
    }
}