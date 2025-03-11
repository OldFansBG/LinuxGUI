cmake_minimum_required(VERSION 3.25 FATAL_ERROR)
project(LinuxISOPro LANGUAGES CXX RC)

# Disable all warnings (Compiler-specific)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-w)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(/W0)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Conan integration ---
# Expects conanfile.txt to exist in the source directory
if(EXISTS "${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
    include("${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
    conan_basic_setup(TARGETS)
else()
    message(FATAL_ERROR "The file conanbuildinfo.cmake doesn't exist, you have to run conan install first")
endif()

# Find MongoDB package
find_package(mongo-cxx-driver REQUIRED)

# --- Output directory configuration ---
if(CMAKE_BUILD_TYPE)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}")
else()
  foreach(config ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER "${config}" config_upper)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${config_upper} "${CMAKE_BINARY_DIR}/${config}")
  endforeach()
endif()

# --- FetchContent for wxWidgets, libarchive, and yaml-cpp ---
include(FetchContent)

# wxWidgets
set(wxBUILD_SHARED OFF CACHE BOOL "Build wxWidgets as static libraries")
set(wxUSE_GRAPHICS_DIRECT2D ON CACHE BOOL "Use Direct2D for graphics")
FetchContent_Declare(
  wxWidgets
  GIT_REPOSITORY https://github.com/wxWidgets/wxWidgets.git
  GIT_TAG v3.2.2.1
  GIT_SHALLOW ON
)
FetchContent_MakeAvailable(wxWidgets)

# LibArchive
set(ENABLE_TAR OFF CACHE BOOL "Disable tar support" FORCE)
set(ENABLE_CPIO OFF CACHE BOOL "Disable cpio support" FORCE)
set(ENABLE_CAT OFF CACHE BOOL "Disable cat support" FORCE)
set(ENABLE_XATTR OFF CACHE BOOL "Disable xattr support" FORCE)
set(ENABLE_ACL OFF CACHE BOOL "Disable ACL support" FORCE)
set(ENABLE_TEST OFF CACHE BOOL "Disable tests" FORCE)
set(ENABLE_INSTALL OFF CACHE BOOL "Disable install target" FORCE)
FetchContent_Declare(
   libarchive
   GIT_REPOSITORY https://github.com/libarchive/libarchive.git
   GIT_TAG v3.7.2
   GIT_SHALLOW ON
)
FetchContent_MakeAvailable(libarchive)

# yaml-cpp
set(YAML_CPP_BUILD_SHARED OFF CACHE BOOL "Build yaml-cpp static library" FORCE)
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "Disable yaml tests" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "Disable yaml tools" FORCE)
FetchContent_Declare(
   yaml-cpp
   GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
   GIT_TAG 0.8.0
   GIT_SHALLOW ON
)
FetchContent_MakeAvailable(yaml-cpp)

# --- Find Packages ---

# Explicitly find CURL
find_package(CURL)  # Don't use REQUIRED here

if(CURL_FOUND)
    message(STATUS "Found CURL: ${CURL_LIBRARIES}")
else()
    message(WARNING "CURL not found. Ensure it's installed via Conan.")
endif()

# RapidJSON (via Conan or manual)
if(NOT TARGET rapidjson)
  find_path(RAPIDJSON_INCLUDE_DIR NAMES rapidjson/document.h
    PATHS "C:/msys64/mingw64/include"
    NO_DEFAULT_PATH
  )
  if(NOT RAPIDJSON_INCLUDE_DIR)
    message(FATAL_ERROR "RapidJSON not found! Install via Conan or MSYS2.")
  endif()
endif()

# --- Source, Header, and Resource Files ---
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

set(RESOURCE_FILES
   "${CMAKE_CURRENT_SOURCE_DIR}/resources/linuxisopro.ico"
   "${CMAKE_CURRENT_SOURCE_DIR}/resources.rc"
   "${CMAKE_CURRENT_SOURCE_DIR}/resources/gear.png"
)

# --- Platform-Specific Libraries and Definitions ---
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

# --- Create Executable ---
add_executable(LinuxISOPro WIN32 ${SOURCES} ${HEADERS} ${RESOURCE_FILES})

# --- Include Directories ---
target_include_directories(LinuxISOPro
   PRIVATE
       ${CMAKE_CURRENT_SOURCE_DIR}
       ${CMAKE_CURRENT_SOURCE_DIR}/src
       ${CMAKE_CURRENT_SOURCE_DIR}/src/theme
       ${libarchive_SOURCE_DIR}/libarchive
       ${libarchive_BINARY_DIR}
       ${RAPIDJSON_INCLUDE_DIR}
)

# --- Link Libraries ---
target_link_libraries(LinuxISOPro
    PRIVATE
        wx::core
        wx::base
        archive_static
        yaml-cpp
        ${PLATFORM_LIBS}
        $<$<BOOL:${CURL_FOUND}>:CURL::libcurl>
        mongo::mongocxx_static
        mongo::bsoncxx_static
)

# --- Windows-Specific Compile Definitions ---
if(WIN32)
   target_compile_definitions(LinuxISOPro
       PRIVATE
           WIN32_LEAN_AND_MEAN
           UNICODE
           _UNICODE
           LIBARCHIVE_STATIC
   )
   set_property(TARGET LinuxISOPro PROPERTY
       MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# --- Copy Files to Output Directory ---
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.yaml
              ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/config.yaml COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/themes.json
              ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/themes.json COPYONLY)
file(COPY ${RESOURCE_FILES} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# --- Installation ---
install(TARGETS LinuxISOPro RUNTIME DESTINATION bin)
install(FILES
   ${CMAKE_CURRENT_SOURCE_DIR}/config.yaml
   ${CMAKE_CURRENT_SOURCE_DIR}/themes.json
   DESTINATION bin
)

# --- Build Information ---
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Install Prefix: ${CMAKE_INSTALL_PREFIX}")