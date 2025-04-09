cmake_minimum_required(VERSION 3.14)
project(LinuxISOPro LANGUAGES CXX)

# Add after your project declaration
if(MINGW)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--allow-multiple-definition")
endif()

set(CMAKE_TOOLCHAIN_FILE "C:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
set(VCPKG_TARGET_TRIPLET "x64-mingw-static" CACHE STRING "")

# Compiler Warnings
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-w)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(/W0)
endif()

# Windows Resource Compiler
if(WIN32)
    enable_language(RC)
    set(CMAKE_RC_COMPILER_INIT windres)
endif()

include(FetchContent)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find libcurl
find_package(CURL REQUIRED)

# Build Output
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
if(WIN32)
   foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
       string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
       set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR})
   endforeach()
endif()

# Add Release build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

# wxWidgets
set(wxBUILD_SHARED OFF)
set(wxUSE_GRAPHICS_DIRECT2D ON)
FetchContent_Declare(
  wxWidgets
  GIT_REPOSITORY https://github.com/wxWidgets/wxWidgets.git
  GIT_TAG v3.2.2.1
  GIT_SHALLOW ON
)
FetchContent_MakeAvailable(wxWidgets)

# wxSVG (prebuilt library)
find_library(wxSVG_LIBRARY NAMES wxsvg PATHS /mingw64/lib)
find_path(wxSVG_INCLUDE_DIR NAMES wx/svg/svg.h PATHS /mingw64/include)

if(NOT wxSVG_LIBRARY OR NOT wxSVG_INCLUDE_DIR)
    message(FATAL_ERROR "wxSVG library or headers not found!")
endif()

# LibArchive
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

# yaml-cpp
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

# RapidJSON
find_path(RAPIDJSON_INCLUDE_DIR NAMES rapidjson/document.h
  PATHS "C:/msys64/mingw64/include"
  NO_DEFAULT_PATH
)

if(NOT RAPIDJSON_INCLUDE_DIR)
  message(FATAL_ERROR "RapidJSON not found!
Install it with: pacman -S mingw-w64-x86_64-rapidjson")
endif()

# Source files (as in your file)
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
   FilesystemSelectionDialog.cpp
   SystemTab.cpp
)

# Header files (as in your file)
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

# Resource files
set(RESOURCE_FILES
   "${CMAKE_CURRENT_SOURCE_DIR}/resources/linuxisopro.ico"
   "${CMAKE_CURRENT_SOURCE_DIR}/resources.rc"
   "${CMAKE_CURRENT_SOURCE_DIR}/resources/gear.png"
)

# Platform-specific
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
add_executable(LinuxISOPro WIN32 ${SOURCES} ${HEADERS} ${RESOURCE_FILES})

# Include directories
target_include_directories(LinuxISOPro
   PRIVATE
       ${CMAKE_CURRENT_SOURCE_DIR}
       ${CMAKE_CURRENT_SOURCE_DIR}/src
       ${CMAKE_CURRENT_SOURCE_DIR}/src/theme
       ${libarchive_SOURCE_DIR}/libarchive
       ${libarchive_BINARY_DIR}
       ${RAPIDJSON_INCLUDE_DIR}
       ${wxSVG_INCLUDE_DIR}  # Add wxsvg include directory
)

# Add these before your find_package calls
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "C:/tools/vcpkg/installed/x64-mingw-static")
set(bsoncxx_DIR "C:/tools/vcpkg/installed/x64-mingw-static/share/bsoncxx")
set(mongocxx_DIR "C:/tools/vcpkg/installed/x64-mingw-static/share/mongocxx")

# Find mongocxx
find_package(bsoncxx CONFIG REQUIRED)
find_package(mongocxx CONFIG REQUIRED)

# Link libraries
target_link_libraries(LinuxISOPro
   PRIVATE
       wx::core
       wx::base
       archive_static
       yaml-cpp
       ${PLATFORM_LIBS}
       CURL::libcurl
       $<IF:$<TARGET_EXISTS:mongo::bsoncxx_static>,mongo::bsoncxx_static,mongo::bsoncxx_shared>
       $<IF:$<TARGET_EXISTS:mongo::mongocxx_static>,mongo::mongocxx_static,mongo::mongocxx_shared>
       ${wxSVG_LIBRARY}  # Link wxsvg
)

# Windows-specific settings
if(WIN32)
   target_compile_definitions(LinuxISOPro
       PRIVATE
           WIN32_LEAN_AND_MEAN
           UNICODE
           _UNICODE
           LIBARCHIVE_STATIC
           MONGOCXX_STATIC
           BSONCXX_STATIC
   )

   # Set static runtime
   set_property(TARGET LinuxISOPro PROPERTY
       MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Copy configuration files
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.yaml
              ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/config.yaml COPYONLY)

# Copy scripts
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
   ${RESOURCE_FILES}
   DESTINATION bin
)

# Print build configuration
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Scripts to copy:")
foreach(SCRIPT ${SCRIPT_FILES})
   message(STATUS "  ${SCRIPT}")
endforeach()