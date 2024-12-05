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
message(STATUS "Fetching wxWidgets...")
FetchContent_Declare(
   wxWidgets
   GIT_REPOSITORY https://github.com/wxWidgets/wxWidgets.git
   GIT_TAG v3.2.2.1
   GIT_SHALLOW ON
)
FetchContent_MakeAvailable(wxWidgets)

# LibArchive configuration
message(STATUS "Configuring libarchive...")
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

# PCRE configuration
message(STATUS "Fetching PCRE...")
FetchContent_Declare(
    pcre
    GIT_REPOSITORY https://github.com/PCRE2Project/pcre2.git
    GIT_TAG pcre2-10.42
    GIT_SHALLOW ON
)

set(PCRE2_BUILD_TESTS OFF CACHE BOOL "")
set(PCRE2_BUILD_PCRE2_8 ON CACHE BOOL "")
set(PCRE2_BUILD_PCRE2_16 OFF CACHE BOOL "")
set(PCRE2_BUILD_PCRE2_32 OFF CACHE BOOL "")
FetchContent_MakeAvailable(pcre)

# yaml-cpp configuration
message(STATUS "Fetching yaml-cpp...")
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

# Source files
set(SRCS
    main.cpp
    MainFrame.cpp
    ISOReader.cpp
    OSDetector.cpp
    SecondWindow.cpp
    WindowsCmdPanel.cpp
    LinuxTerminalPanel.cpp
    CustomTitleBar.cpp
    CustomStatusBar.cpp
    SettingsDialog.cpp
    ISOExtractor.cpp
)

# Header files
set(HEADERS
    ISOReader.h
    MainFrame.h
    OSDetector.h
    SecondWindow.h
    WindowsCmdPanel.h
    LinuxTerminalPanel.h
    CustomTitleBar.h
    CustomStatusBar.h
    SettingsDialog.h 
    ISOExtractor.h
    ThemeColors.h
    MyButton.h
    DockerTransfer.cpp
)

# Configuration files
set(CONFIG_FILES
    config.yaml
    minimize.png
    maximize.png
    close.png
    gear.png
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
elseif(UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED IMPORTED_TARGET gtk+-3.0)
    pkg_check_modules(VTE REQUIRED IMPORTED_TARGET vte-2.91)
    find_package(X11 REQUIRED)
    
    set(PLATFORM_LIBS
        PkgConfig::GTK3
        PkgConfig::VTE
        ${X11_LIBRARIES}
    )
    
    add_compile_definitions(__WXGTK__)
else()
    set(PLATFORM_LIBS "")
endif()

# Create executable
if(APPLE)
    add_executable(iso_analyzer MACOSX_BUNDLE ${SRCS} ${HEADERS})
    set_target_properties(iso_analyzer PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist)
else()
    add_executable(iso_analyzer WIN32 ${SRCS} ${HEADERS})
endif()

# Include directories
target_include_directories(iso_analyzer 
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
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
        pcre2-8
        ${PLATFORM_LIBS}
)

# Copy configuration files
foreach(CONFIG_FILE ${CONFIG_FILES})
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/${CONFIG_FILE}
        ${CMAKE_BINARY_DIR}/${CONFIG_FILE}
        COPYONLY
    )
endforeach()

# Install configuration
install(FILES ${CONFIG_FILES} 
        DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

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
    set_property(TARGET yaml-cpp PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Unix-specific settings
if(UNIX AND NOT APPLE)
    target_include_directories(iso_analyzer PRIVATE ${X11_INCLUDE_DIR})
endif()