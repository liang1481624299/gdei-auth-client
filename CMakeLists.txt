# CMakeLists.txt — GDEI 校园网认证客户端 / campus auth client
# 核心库（core）+ 命令行（cli）始终构建；Qt 图形界面（gui）按需打开。
# Core CLI always builds; Qt GUI is optional.
cmake_minimum_required(VERSION 3.10)
project(gdei-auth-client CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(GDEI_BUILD_GUI "Build the optional Qt GUI" OFF)

# ---------------------------------------------------------------- core ----
set(CORE_SOURCES
    core/src/encoding.cpp
    core/src/json.cpp
    core/src/config.cpp
    core/src/portal_client.cpp
    core/src/keepalive.cpp
    core/src/http_client.cpp
)

if(WIN32)
    # Windows 原生 WinINet 后端（Win7 起自带）。
    list(APPEND CORE_SOURCES core/src/http_wininet.cpp)
    # 目标系统版本：Windows 7 (0x0601) 起。
    add_definitions(-D_WIN32_WINNT=0x0601)
else()
    # 非 Windows：优先使用 libcurl；找不到时核心仍可编译，HTTP 后端为 Stub。
    find_package(CURL QUIET)
    if(CURL_FOUND)
        add_definitions(-DGDEI_HAVE_CURL)
        list(APPEND CORE_SOURCES core/src/http_curl.cpp)
    endif()
endif()

add_library(gdei_core STATIC ${CORE_SOURCES})
target_include_directories(gdei_core PUBLIC core/include)

if(WIN32)
    target_link_libraries(gdei_core PUBLIC wininet)
else()
    if(CURL_FOUND)
        target_link_libraries(gdei_core PUBLIC CURL::libcurl)
    endif()
endif()

# ------------------------------------------------------------------ cli ----
add_executable(gdei-auth cli/main.cpp)
target_link_libraries(gdei-auth PRIVATE gdei_core)

# ------------------------------------------------------------------ gui ----
if(GDEI_BUILD_GUI)
    set(CMAKE_AUTOMOC ON)
    find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets QUIET)
    find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)

    add_executable(gdei-auth-gui WIN32
        gui/qt/main.cpp
        gui/qt/mainwindow.cpp
    )
    target_include_directories(gdei-auth-gui PRIVATE gui/qt)
    target_link_libraries(gdei-auth-gui PRIVATE gdei_core Qt${QT_VERSION_MAJOR}::Widgets)

    # 老 Windows 建议用 Qt 5.12 LTS（最后一个官方支持 Win7 的大版本）。
endif()

# 安装 / install
install(TARGETS gdei-auth RUNTIME DESTINATION bin)
if(GDEI_BUILD_GUI)
    install(TARGETS gdei-auth-gui RUNTIME DESTINATION bin)
endif()
