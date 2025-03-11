cmake_minimum_required(VERSION 3.17 FATAL_ERROR)

# Set CMP0091 policy to NEW
if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()

project(LinuxISOPro LANGUAGES CXX)

# Disable all warnings
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-w)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(/W0)
endif()

# Enable RC (Resource Compiler) for Windows
if(WIN32)
    enable_language(RC)
    set(CMAKE_RC_COMPILER_INIT windres)  # Use windres for MinGW
endif()

# Include Conan-generated files
include(${CMAKE_BINARY_DIR}/generators/conan_toolchain.cmake)

# Find the packages
find_package(CURL REQUIRED)
find_package(wxWidgets REQUIRED)
find_package(LibArchive REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(rapidjson REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build output configuration
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
if(WIN32)
   foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
       string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
       set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR})
   endforeach()
endif()

# Source files
set(SOURCES
   main.cpp
   MainFrame.cpp 
   CustomTitleBar.cpp
   CustomStatusBar.cpp
   ISOReader.cpp
   OSDetector.cpp
   SecondWindow.cpp
   LinuxTerminalPanel.cpp
   SettingsDialog.cpp
   ThemeConfig.cpp
   ContainerManager.cpp
   ScriptManager.cpp
   ScriptContents.cpp
   ConfigTab.cpp
   FlatpakStore.cpp
   DesktopTab.cpp
   SettingsManager.cpp
   WinTerminalManager.cpp
   CustomizeTab.cpp
)

# Header files
set(HEADERS
   MainFrame.h
   CustomTitleBar.h
   CustomStatusBar.h
   ISOReader.h
   OSDetector.h
   SecondWindow.h
   LinuxTerminalPanel.h
   SettingsDialog.h
   ContainerManager.h
   SettingsManager.h
)

# Resource files (including the icon and RC file)
set(RESOURCE_FILES
   "${CMAKE_CURRENT_SOURCE_DIR}/resources/linuxisopro.ico"  # Explicitly include the icon
   "${CMAKE_CURRENT_SOURCE_DIR}/resources.rc"               # Include the RC file
   "${CMAKE_CURRENT_SOURCE_DIR}/resources/gear.png"         # Assuming this exists from your code
   # Add other PNGs or ICOs here if needed
)

# Platform-specific configurations
if(WIN32)
   set(PLATFORM_LIBS
       dwmapi
       uxtheme
       d2d1
       dwrite
   )
   
   add_compile_definitions(
       WINVER=0x0A00
       _WIN32_WINNT=0x0A00
   )
endif()

# Create executable as a Windows GUI application
add_executable(LinuxISOPro WIN32 ${SOURCES} ${HEADERS} ${RESOURCE_FILES})

# Include directories
target_include_directories(LinuxISOPro 
   PRIVATE
       ${CMAKE_CURRENT_SOURCE_DIR}
       ${CMAKE_CURRENT_SOURCE_DIR}/src
       ${CMAKE_CURRENT_SOURCE_DIR}/src/theme
       ${RAPIDJSON_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(LinuxISOPro 
   PRIVATE
       CURL::libcurl
       wxWidgets::wxWidgets
       LibArchive::LibArchive
       yaml-cpp::yaml-cpp
       ${PLATFORM_LIBS}
)

# Windows-specific settings
if(WIN32)
   target_compile_definitions(LinuxISOPro 
       PRIVATE
           WIN32_LEAN_AND_MEAN
           UNICODE
           _UNICODE
           LIBARCHIVE_STATIC
   )

   # Set static runtime
   set_property(TARGET LinuxISOPro PROPERTY
       MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Copy configuration files to build directory
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.yaml
              ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/config.yaml COPYONLY)

# Copy scripts to build directory with executable permissions
foreach(SCRIPT ${SCRIPT_FILES})
   configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${SCRIPT}
                 ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${SCRIPT}
                 COPYONLY)
   if(UNIX)
       execute_process(
           COMMAND chmod +x ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${SCRIPT}
       )
   endif()
endforeach()

# Copy Python script
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/docker_operations.py
              ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/docker_operations.py COPYONLY)

# Copy resource files
file(COPY ${RESOURCE_FILES} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# Copy themes file
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/themes.json
              ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/themes.json
              COPYONLY)

# Install configuration
install(TARGETS LinuxISOPro
   RUNTIME DESTINATION bin
)

install(FILES 
   ${CMAKE_CURRENT_SOURCE_DIR}/config.yaml
   ${CMAKE_CURRENT_SOURCE_DIR}/themes.json
   ${SCRIPT_FILES}
   ${RESOURCE_FILES}  # Includes linuxisopro.ico
   DESTINATION bin
)

# Print build configuration information
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Scripts to copy:")
foreach(SCRIPT ${SCRIPT_FILES})
   message(STATUS "  ${SCRIPT}")
endforeach()

find_path(RAPIDJSON_INCLUDE_DIR rapidjson/document.h)
target_include_directories(LinuxISOPro PRIVATE ${RAPIDJSON_INCLUDE_DIR})