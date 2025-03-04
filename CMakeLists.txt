cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
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

include(FetchContent)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find and link libcurl
find_package(CURL REQUIRED)

# Build output configuration
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
if(WIN32)
   foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
       string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
       set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR})
   endforeach()
endif()

# wxWidgets configuration
set(wxBUILD_SHARED OFF)
set(wxUSE_GRAPHICS_DIRECT2D ON)
FetchContent_Declare(
  wxWidgets
  GIT_REPOSITORY https://github.com/wxWidgets/wxWidgets.git
  GIT_TAG v3.2.2.1
  GIT_SHALLOW ON
)
FetchContent_MakeAvailable(wxWidgets)

# LibArchive configuration
set(ENABLE_TAR OFF CACHE BOOL "")
set(ENABLE_CPIO OFF CACHE BOOL "")
set(ENABLE_CAT OFF CACHE BOOL "")
set(ENABLE_XATTR OFF CACHE BOOL "")
set(ENABLE_ACL OFF CACHE BOOL "")
set(ENABLE_TEST OFF CACHE BOOL "")
set(ENABLE_INSTALL OFF CACHE BOOL "")

FetchContent_Declare(
   libarchive
   GIT_REPOSITORY https://github.com/libarchive/libarchive.git
   GIT_TAG v3.7.2
   GIT_SHALLOW ON
)
FetchContent_MakeAvailable(libarchive)

# yaml-cpp configuration
set(YAML_CPP_BUILD_SHARED OFF CACHE BOOL "Build yaml-cpp static library" FORCE)
FetchContent_Declare(
   yaml-cpp
   GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
   GIT_TAG yaml-cpp-0.7.0
   GIT_SHALLOW ON
)
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "disable yaml tests")
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "disable yaml tools")
FetchContent_MakeAvailable(yaml-cpp)

# RapidJSON Configuration (Using system-installed version from MSYS2)
find_path(RAPIDJSON_INCLUDE_DIR NAMES rapidjson/document.h
  PATHS "C:/msys64/mingw64/include"
  NO_DEFAULT_PATH
)

if(NOT RAPIDJSON_INCLUDE_DIR)
  message(FATAL_ERROR "RapidJSON not found! Install it with: pacman -S mingw-w64-x86_64-rapidjson")
endif()

# ---------------------------------------------------------------
# Python 3.9 Configuration (Anaconda Environment)
# ---------------------------------------------------------------
set(Python_FIND_REGISTRY NEVER)  # Ignore Windows registry
set(Python_FIND_STRATEGY LOCATION)  # Prioritize explicit paths
set(Python_ROOT_DIR "C:/Users/PC/anaconda3/envs/myenv")  # Conda environment path
find_package(Python 3.9 EXACT REQUIRED COMPONENTS Development)

# Add Python include and library paths
include_directories(
    ${Python_INCLUDE_DIRS}
    ${Python_ROOT_DIR}/include
)
link_directories(${Python_LIBRARY_DIRS})

# Copy Python files from Anaconda environment
file(COPY "${Python_ROOT_DIR}/python39.dll" DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
file(COPY "${Python_ROOT_DIR}/Lib" DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

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
   SQLTab.cpp
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
       ${libarchive_SOURCE_DIR}/libarchive
       ${libarchive_BINARY_DIR}
       ${Python_INCLUDE_DIRS}
       ${RAPIDJSON_INCLUDE_DIR}
)

# Link libraries
target_link_libraries(LinuxISOPro 
   PRIVATE
       wx::core
       wx::base
       archive_static
       yaml-cpp
       ${PLATFORM_LIBS}
       CURL::libcurl
       ${Python_LIBRARIES}
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