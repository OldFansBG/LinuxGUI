cmake_minimum_required(VERSION 3.16)
project(LinuxISOPro VERSION 1.1.0 LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "LinuxISOPro is a Windows application (it uses the Win32 API and Docker Desktop).")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

option(LINUXISOPRO_WITH_MONGODB "Enable MongoDB Atlas sync (requires mongo-cxx-driver)" ON)

# The legacy sources produce a lot of noise; keep the build output readable.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-w)
elseif(MSVC)
    add_compile_options(/W0)
endif()

enable_language(RC)

# ---------------------------------------------------------------------------
# Dependencies (see README.md for the MSYS2 packages that provide them)
# ---------------------------------------------------------------------------

# wxWidgets: prefer the CMake package config shipped by MSYS2 / vcpkg, fall back to wx-config.
find_package(wxWidgets 3.2 CONFIG QUIET COMPONENTS core base)
if(TARGET wx::core)
    set(WX_LIBRARIES wx::core wx::base)
else()
    find_package(wxWidgets 3.2 REQUIRED COMPONENTS core base)
    include(${wxWidgets_USE_FILE})
    set(WX_LIBRARIES ${wxWidgets_LIBRARIES})
endif()

find_package(LibArchive REQUIRED)
find_package(CURL REQUIRED)

find_package(yaml-cpp CONFIG REQUIRED)
if(TARGET yaml-cpp::yaml-cpp)
    set(YAML_CPP_TARGET yaml-cpp::yaml-cpp)
else()
    set(YAML_CPP_TARGET yaml-cpp)
endif()

find_path(RAPIDJSON_INCLUDE_DIR NAMES rapidjson/document.h)
if(NOT RAPIDJSON_INCLUDE_DIR)
    message(FATAL_ERROR "RapidJSON not found. On MSYS2 UCRT64: pacman -S mingw-w64-ucrt-x86_64-rapidjson")
endif()

# MongoDB is optional: without it the MongoDB panel stores configurations in local JSON files.
if(LINUXISOPRO_WITH_MONGODB)
    find_package(bsoncxx CONFIG QUIET)
    find_package(mongocxx CONFIG QUIET)
    foreach(candidate mongo::mongocxx_static mongo::mongocxx_shared mongo::mongocxx)
        if(TARGET ${candidate})
            set(MONGOCXX_TARGET ${candidate})
            break()
        endif()
    endforeach()
    foreach(candidate mongo::bsoncxx_static mongo::bsoncxx_shared mongo::bsoncxx)
        if(TARGET ${candidate})
            set(BSONCXX_TARGET ${candidate})
            break()
        endif()
    endforeach()
    if(NOT MONGOCXX_TARGET OR NOT BSONCXX_TARGET)
        message(WARNING "mongo-cxx-driver not found: building without MongoDB sync (local storage only).")
        set(LINUXISOPRO_WITH_MONGODB OFF)
    endif()
endif()

# ---------------------------------------------------------------------------
# Application
# ---------------------------------------------------------------------------

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
    ContainerCommand.cpp
    ConfigTab.cpp
    FlatpakStore.cpp
    DesktopTab.cpp
    SettingsManager.cpp
    WinTerminalManager.cpp
    CustomizeTab.cpp
    FilesystemSelectionDialog.cpp
    SystemTab.cpp
    MongoDBPanel.cpp
    resources.rc
)

add_executable(LinuxISOPro WIN32 ${SOURCES})

target_include_directories(LinuxISOPro PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${RAPIDJSON_INCLUDE_DIR}
)

target_compile_definitions(LinuxISOPro PRIVATE
    WIN32_LEAN_AND_MEAN
    UNICODE
    _UNICODE
    WINVER=0x0A00
    _WIN32_WINNT=0x0A00
    LINUXISOPRO_VERSION="${PROJECT_VERSION}"
)

target_link_libraries(LinuxISOPro PRIVATE
    ${WX_LIBRARIES}
    LibArchive::LibArchive
    CURL::libcurl
    ${YAML_CPP_TARGET}
    dwmapi
    uxtheme
)

if(LINUXISOPRO_WITH_MONGODB)
    target_compile_definitions(LinuxISOPro PRIVATE LINUXISOPRO_HAS_MONGODB=1)
    target_link_libraries(LinuxISOPro PRIVATE ${MONGOCXX_TARGET} ${BSONCXX_TARGET})
endif()

# Runtime files the application loads from its own directory.
add_custom_command(TARGET LinuxISOPro POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/config.yaml
            ${CMAKE_CURRENT_SOURCE_DIR}/themes.json
            $<TARGET_FILE_DIR:LinuxISOPro>
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/resources
            $<TARGET_FILE_DIR:LinuxISOPro>/resources
    COMMENT "Copying config.yaml, themes.json and resources/ next to the executable"
)

install(TARGETS LinuxISOPro RUNTIME DESTINATION .)
install(FILES config.yaml themes.json DESTINATION .)
install(DIRECTORY resources DESTINATION .)

message(STATUS "LinuxISOPro ${PROJECT_VERSION} - build type: ${CMAKE_BUILD_TYPE}, MongoDB sync: ${LINUXISOPRO_WITH_MONGODB}")
