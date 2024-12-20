cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
project(iso_analyzer LANGUAGES CXX)
include(FetchContent)
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

# Theme system sources
# Remove theme-related sources
set(SOURCES
    main.cpp
    MainFrame.cpp 
    CustomTitleBar.cpp
    CustomStatusBar.cpp
    ISOReader.cpp
    OSDetector.cpp
    SecondWindow.cpp
    WindowsCmdPanel.cpp
    LinuxTerminalPanel.cpp
    SettingsDialog.cpp
    ISOExtractor.cpp
    DockerTransfer.cpp
    gear_icon.cpp
    ThemeConfig.cpp
)

# Header files
set(HEADERS
    MainFrame.h
    CustomTitleBar.h
    CustomStatusBar.h
    ISOReader.h
    OSDetector.h
    SecondWindow.h
    WindowsCmdPanel.h
    LinuxTerminalPanel.h
    SettingsDialog.h
    ISOExtractor.h
    DockerTransfer.h
    MyButton.h
    gear_icon.h
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

# Create executable
add_executable(iso_analyzer WIN32 ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(iso_analyzer 
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/theme
        ${libarchive_SOURCE_DIR}/libarchive
        ${libarchive_BINARY_DIR}
)

# Link libraries
target_link_libraries(iso_analyzer 
    PRIVATE
        wx::core
        wx::base
        archive_static
        yaml-cpp
        ${PLATFORM_LIBS}
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

# Copy resource files
file(GLOB RESOURCE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/*.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/*.ico"
)
file(COPY ${RESOURCE_FILES} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# Install configuration
install(TARGETS iso_analyzer
    RUNTIME DESTINATION bin
)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/themes.json
               ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/themes.json
               COPYONLY)
install(FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/config.yaml
    ${RESOURCE_FILES}
    DESTINATION bin
)