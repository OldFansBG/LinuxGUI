cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
project(iso_analyzer LANGUAGES CXX)

# Disable all warnings
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-w)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(/W0)
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
# Force CMake to use the correct Python version
set(Python_FIND_REGISTRY NEVER)  # Ignore Windows registry
set(Python_FIND_STRATEGY LOCATION)  # Prioritize explicit paths
set(Python_ROOT_DIR "C:/Users/PC/anaconda3/envs/myenv")  # Conda environment path
find_package(Python 3.9 EXACT REQUIRED COMPONENTS Development)  # Match your Python version

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
   MyButton.h
   ContainerManager.h
   SettingsManager.h
)

# Scripts and resources to copy
set(SCRIPT_FILES
   setup_output.sh
   setup_chroot.sh
   create_iso.sh
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
add_executable(iso_analyzer WIN32 ${SOURCES} ${HEADERS} ${RESOURCE_FILES})

# Include directories
target_include_directories(iso_analyzer 
   PRIVATE
       ${CMAKE_CURRENT_SOURCE_DIR}
       ${CMAKE_CURRENT_SOURCE_DIR}/src
       ${CMAKE_CURRENT_SOURCE_DIR}/src/theme
       ${libarchive_SOURCE_DIR}/libarchive
       ${libarchive_BINARY_DIR}
       ${Python_INCLUDE_DIRS}
       ${RAPIDJSON_INCLUDE_DIR}  # Use system-installed RapidJSON
)

# Link libraries
target_link_libraries(iso_analyzer 
   PRIVATE
       wx::core
       wx::base
       archive_static
       yaml-cpp
       ${PLATFORM_LIBS}
       CURL::libcurl  # Link libcurl
       ${Python_LIBRARIES}  # Link Python
)

# Windows-specific settings
if(WIN32)
   target_compile_definitions(iso_analyzer 
       PRIVATE
           WIN32_LEAN_AND_MEAN
           UNICODE
           _UNICODE
           LIBARCHIVE_STATIC
   )

   # Set static runtime
   set_property(TARGET iso_analyzer PROPERTY
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
file(GLOB RESOURCE_FILES
   "${CMAKE_CURRENT_SOURCE_DIR}/resources/*.png"
   "${CMAKE_CURRENT_SOURCE_DIR}/resources/*.ico"
)
file(COPY ${RESOURCE_FILES} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# Copy themes file
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/themes.json
              ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/themes.json
              COPYONLY)

# Install configuration
install(TARGETS iso_analyzer
   RUNTIME DESTINATION bin
)

install(FILES 
   ${CMAKE_CURRENT_SOURCE_DIR}/config.yaml
   ${CMAKE_CURRENT_SOURCE_DIR}/themes.json
   ${SCRIPT_FILES}
   ${RESOURCE_FILES}
   DESTINATION bin
)

# Print build configuration information
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Scripts to copy:")
foreach(SCRIPT ${SCRIPT_FILES})
   message(STATUS "  ${SCRIPT}")
endforeach()